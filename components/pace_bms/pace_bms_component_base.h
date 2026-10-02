#pragma once

#include <functional>
#include <queue>
#include <list>
#include <string>
#include <vector>
#include <span>
#include <optional>
#include <map>

#include "esphome/core/component.h"
#include "esphome/core/log.h"

#include "pace_bms_protocol_v25.h"
#include "pace_bms_protocol_v20.h"
#include "pace_bms_dc_protocol.h"

namespace esphome {
namespace pace_bms_base {

enum BmsType : uint8_t {
	BMS_TYPE_MASTER = 0,
	BMS_TYPE_SLAVE = 1,
};

class PaceBmsBase {
public:
	// called by the codegen to set our YAML property values
	void set_id_name(std::string id_name) { this->id_name_ = id_name; }
	void set_bms_type(BmsType bms_type) { this->bms_type_ = bms_type; }
	void set_address(uint8_t address) { this->address_ = address; }

	// make available
	std::string get_id_name() const { return this->id_name_; }
	BmsType get_bms_type() const { return this->bms_type_; }
	uint8_t get_address() const { return this->address_; }

	// Multiple sensor/number/switch subscribers share one F5 read per pack and parameter.
	void register_dc_parameter_callback_v25(uint8_t parameter, std::function<void(float)> callback) {
		if (PaceBmsDcProtocol::IsSupportedParameter(parameter))
			dc_parameter_callbacks_[parameter].push_back(std::move(callback));
	}
	bool needs_dc_parameter_v25(uint8_t parameter) const {
		auto found = dc_parameter_callbacks_.find(parameter);
		return found != dc_parameter_callbacks_.end() && !found->second.empty();
	}
	bool has_dc_parameter_value_v25(uint8_t parameter) const { return dc_parameter_values_.count(parameter) != 0; }
	void publish_dc_parameter_v25(uint8_t parameter, uint16_t raw) {
		dc_parameter_values_[parameter] = raw;
		auto found = dc_parameter_callbacks_.find(parameter);
		if (found != dc_parameter_callbacks_.end())
			for (const auto& callback : found->second) callback(raw / PaceBmsDcProtocol::ValueScale(parameter));
	}
	virtual void write_equalized_charging_current_v25(float amperes) = 0;
	virtual void write_dc_parameter_v25(uint8_t parameter, float value) = 0;
	void register_online_callback(std::function<void(bool)> callback, uint32_t offline_timeout_ms, uint32_t now = 0) {
		online_callbacks_.push_back(std::move(callback));
		offline_timeout_ms_ = offline_timeout_ms;
		online_monitor_started_ms_ = now;
	}
	bool needs_online_probe() const { return !online_callbacks_.empty(); }
	void notify_online_probe_success(uint32_t now) {
		last_online_success_ms_ = now;
		has_online_success_ = true;
		if (!has_online_state_ || !online_state_) publish_online_state_(true);
	}
	void notify_online_probe_failure(uint32_t now) {
		if (!has_online_success_ || now - last_online_success_ms_ >= offline_timeout_ms_)
			if (!has_online_state_ || online_state_) publish_online_state_(false);
	}

	void check_online_timeout(uint32_t now) {
		if (!needs_online_probe()) return;
		const uint32_t reference = has_online_success_ ? last_online_success_ms_ : online_monitor_started_ms_;
		if (now - reference >= offline_timeout_ms_)
			if (!has_online_state_ || online_state_) publish_online_state_(false);
	}

protected:
	// config values set in YAML
	std::string id_name_;
	BmsType bms_type_;
	uint8_t address_{ 0 };
	std::map<uint8_t, std::vector<std::function<void(float)>>> dc_parameter_callbacks_;
	std::map<uint8_t, uint16_t> dc_parameter_values_;
	std::vector<std::function<void(bool)>> online_callbacks_;
	uint32_t offline_timeout_ms_{15000};
	uint32_t last_online_success_ms_{0};
	uint32_t online_monitor_started_ms_{0};
	bool has_online_success_{false};
	bool has_online_state_{false};
	bool online_state_{false};
	void publish_online_state_(bool online) {
		has_online_state_ = true;
		online_state_ = online;
		for (const auto& callback : online_callbacks_) callback(online);
	}

public:
	// make accessible to sensors
	virtual int get_protocol_commandset() = 0;
	// we don't push all updates in a single loop, that'd stall the ESP out
	virtual void queue_sensor_update(std::function<void()> update) = 0;

	// currently the master will dispatch BMS updates to slaves (or itself) through these two access points, could probably use an improved / cleaner 
	// design such as having slaves (or itself) process the payloads internally via a method such as "notify_analog_information" for example
	virtual std::vector<std::function<void(PaceBmsProtocolV25::AnalogInformation&)>> get_analog_information_callbacks_v25() = 0;
	virtual std::vector<std::function<void(PaceBmsProtocolV25::StatusInformation&)>> get_status_information_callbacks_v25() = 0;

	// child sensors call these to register for notification upon reciept of various types of data from the BMS, and the 
	//     callbacks lists not being empty is what prompts update() to queue command_items for BMS communication in order to 
	//     periodically gather these updates for fan-out to the sensors the first place
	virtual void register_bms_count_callback_v25(std::function<void(uint8_t&)> callback) = 0;
	virtual void register_payload_count_callback_v25(std::function<void(uint8_t&)> callback) = 0;
	virtual void register_analog_information_callback_v25(std::function<void(PaceBmsProtocolV25::AnalogInformation&)> callback) = 0;
	virtual void register_status_information_callback_v25(std::function<void(PaceBmsProtocolV25::StatusInformation&)> callback) = 0;
	virtual void register_hardware_version_callback_v25(std::function<void(std::string&)> callback) = 0;
	virtual void register_serial_number_callback_v25(std::function<void(std::string&) > callback) = 0;
	virtual void register_protocols_callback_v25(std::function<void(PaceBmsProtocolV25::Protocols&) > callback) = 0;
	virtual void register_cell_over_voltage_configuration_callback_v25(std::function<void(PaceBmsProtocolV25::CellOverVoltageConfiguration&)> callback) = 0;
	virtual void register_pack_over_voltage_configuration_callback_v25(std::function<void(PaceBmsProtocolV25::PackOverVoltageConfiguration&)> callback) = 0;
	virtual void register_cell_under_voltage_configuration_callback_v25(std::function<void(PaceBmsProtocolV25::CellUnderVoltageConfiguration&)> callback) = 0;
	virtual void register_pack_under_voltage_configuration_callback_v25(std::function<void(PaceBmsProtocolV25::PackUnderVoltageConfiguration&)> callback) = 0;
	virtual void register_charge_over_current_configuration_callback_v25(std::function<void(PaceBmsProtocolV25::ChargeOverCurrentConfiguration&)> callback) = 0;
	virtual void register_discharge_over_current1_configuration_callback_v25(std::function<void(PaceBmsProtocolV25::DischargeOverCurrent1Configuration&)> callback) = 0;
	virtual void register_discharge_over_current2_configuration_callback_v25(std::function<void(PaceBmsProtocolV25::DischargeOverCurrent2Configuration&)> callback) = 0;
	virtual void register_short_circuit_protection_configuration_callback_v25(std::function<void(PaceBmsProtocolV25::ShortCircuitProtectionConfiguration&)> callback) = 0;
	virtual void register_cell_balancing_configuration_callback_v25(std::function<void(PaceBmsProtocolV25::CellBalancingConfiguration&)> callback) = 0;
	virtual void register_sleep_configuration_callback_v25(std::function<void(PaceBmsProtocolV25::SleepConfiguration&)> callback) = 0;
	virtual void register_full_charge_low_charge_configuration_callback_v25(std::function<void(PaceBmsProtocolV25::FullChargeLowChargeConfiguration&)> callback) = 0;
	virtual void register_charge_and_discharge_over_temperature_configuration_callback_v25(std::function<void(PaceBmsProtocolV25::ChargeAndDischargeOverTemperatureConfiguration&)> callback) = 0;
	virtual void register_charge_and_discharge_under_temperature_configuration_callback_v25(std::function<void(PaceBmsProtocolV25::ChargeAndDischargeUnderTemperatureConfiguration&)> callback) = 0;
	virtual void register_mosfet_over_temperature_configuration_callback_v25(std::function<void(PaceBmsProtocolV25::MosfetOverTemperatureConfiguration&)> callback) = 0;
	virtual void register_environment_over_under_temperature_configuration_callback_v25(std::function<void(PaceBmsProtocolV25::EnvironmentOverUnderTemperatureConfiguration&)> callback) = 0;
	virtual void register_system_datetime_callback_v25(std::function<void(PaceBmsProtocolV25::DateTime&)> callback) = 0;

	virtual void register_analog_information_callback_v20(std::function<void(PaceBmsProtocolV20::AnalogInformation&)> callback) = 0;
	virtual void register_status_information_callback_v20(std::function<void(PaceBmsProtocolV20::StatusInformation&)> callback) = 0;
	virtual void register_hardware_version_callback_v20(std::function<void(std::string&)> callback) = 0;
	virtual void register_serial_number_callback_v20(std::function<void(std::string&) > callback) = 0;
	virtual void register_system_datetime_callback_v20(std::function<void(PaceBmsProtocolV20::DateTime&)> callback) = 0;

	// child sensors call these to schedule new values be written out to the hardware
	virtual void write_switch_state_v25(PaceBmsProtocolV25::SwitchCommand state) = 0;
	virtual void write_mosfet_state_v25(PaceBmsProtocolV25::MosfetType type, PaceBmsProtocolV25::MosfetState state) = 0;
	virtual void write_shutdown_v25() = 0;
	virtual void write_protocols_v25(PaceBmsProtocolV25::Protocols& protocols) = 0;
	virtual void write_cell_over_voltage_configuration_v25(PaceBmsProtocolV25::CellOverVoltageConfiguration& config) = 0;
	virtual void write_pack_over_voltage_configuration_v25(PaceBmsProtocolV25::PackOverVoltageConfiguration& config) = 0;
	virtual void write_cell_under_voltage_configuration_v25(PaceBmsProtocolV25::CellUnderVoltageConfiguration& config) = 0;
	virtual void write_pack_under_voltage_configuration_v25(PaceBmsProtocolV25::PackUnderVoltageConfiguration& config) = 0;
	virtual void write_charge_over_current_configuration_v25(PaceBmsProtocolV25::ChargeOverCurrentConfiguration& config) = 0;
	virtual void write_discharge_over_current1_configuration_v25(PaceBmsProtocolV25::DischargeOverCurrent1Configuration& config) = 0;
	virtual void write_discharge_over_current2_configuration_v25(PaceBmsProtocolV25::DischargeOverCurrent2Configuration& config) = 0;
	virtual void write_short_circuit_protection_configuration_v25(PaceBmsProtocolV25::ShortCircuitProtectionConfiguration& config) = 0;
	virtual void write_cell_balancing_configuration_v25(PaceBmsProtocolV25::CellBalancingConfiguration& config) = 0;
	virtual void write_sleep_configuration_v25(PaceBmsProtocolV25::SleepConfiguration& config) = 0;
	virtual void write_full_charge_low_charge_configuration_v25(PaceBmsProtocolV25::FullChargeLowChargeConfiguration& config) = 0;
	virtual void write_charge_and_discharge_over_temperature_configuration_v25(PaceBmsProtocolV25::ChargeAndDischargeOverTemperatureConfiguration& config) = 0;
	virtual void write_charge_and_discharge_under_temperature_configuration_v25(PaceBmsProtocolV25::ChargeAndDischargeUnderTemperatureConfiguration& config) = 0;
	virtual void write_mosfet_over_temperature_configuration_v25(PaceBmsProtocolV25::MosfetOverTemperatureConfiguration& config) = 0;
	virtual void write_environment_over_under_temperature_configuration_v25(PaceBmsProtocolV25::EnvironmentOverUnderTemperatureConfiguration& config) = 0;
	virtual void write_system_datetime_v25(PaceBmsProtocolV25::DateTime& dt) = 0;

	virtual void write_shutdown_v20() = 0;
	virtual void write_system_datetime_v20(PaceBmsProtocolV20::DateTime& dt) = 0;
};

}  // namespace pace_bms_base
}  // namespace esphome
