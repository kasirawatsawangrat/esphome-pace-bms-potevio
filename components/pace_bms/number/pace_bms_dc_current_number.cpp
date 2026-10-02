#include "pace_bms_dc_current_number.h"
#include <cmath>

namespace esphome::pace_bms_base {
static const char* const TAG = "pace_bms.dc_number";

void PaceBmsDcCurrentNumber::setup() {
  if (this->parent_ == nullptr || this->parent_->get_protocol_commandset() != 0x25 ||
      !PaceBmsDcProtocol::IsSupportedParameter(this->parameter_) ||
      PaceBmsDcProtocol::IsLimitingParameter(this->parameter_)) {
    ESP_LOGE(TAG, "DC number requires a v25 parent and supported numeric parameter");
    this->status_set_error();
    return;
  }
  this->parent_->register_dc_parameter_callback_v25(this->parameter_,
      [this](float value) {
        this->seen_readback_ = true;
        this->parent_->queue_sensor_update([this, value]() { this->publish_state(value); });
      });
}

void PaceBmsDcCurrentNumber::control(float value) {
  if (!this->seen_readback_) {
    ESP_LOGE(TAG, "Wait for a successful DC parameter %u read before writing", this->parameter_);
    return;
  }
  const float scaled = value * PaceBmsDcProtocol::ValueScale(this->parameter_);
  if (!std::isfinite(value) || value < this->traits.get_min_value() ||
      value > this->traits.get_max_value() || value < 0.0f ||
      scaled > PaceBmsDcProtocol::MaxWriteRaw(this->parameter_) ||
      std::fabs(scaled - std::round(scaled)) > 0.005f) {
    ESP_LOGE(TAG, "DC number is outside configured limits or not on a valid step");
    return;
  }
  // No optimistic publish, restore, boot-time write, or automatic retry.
  this->parent_->write_dc_parameter_v25(this->parameter_, value);
}

void PaceBmsDcCurrentNumber::dump_config() {
  LOG_NUMBER("  ", "DC Setting", this);
  ESP_LOGCONFIG(TAG, "  Parameter: 0x%02X", this->parameter_);
}
}
