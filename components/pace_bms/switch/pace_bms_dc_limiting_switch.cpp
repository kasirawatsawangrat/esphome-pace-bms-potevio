#include "pace_bms_dc_limiting_switch.h"

namespace esphome::pace_bms_base {
static const char* const TAG = "pace_bms.dc_switch";

void PaceBmsDcLimitingSwitch::setup() {
  if (this->parent_ == nullptr || this->parent_->get_protocol_commandset() != 0x25 ||
      !PaceBmsDcProtocol::IsLimitingParameter(this->parameter_)) {
    ESP_LOGE(TAG, "DC limiting switch requires a v25 parent and supported limiting parameter");
    this->status_set_error();
    return;
  }
  // Never restore, publish a guessed state, or write at startup.
  this->parent_->register_dc_parameter_callback_v25(this->parameter_, [this](float raw) {
    if (raw != 0.0f && raw != 1.0f) return;
    this->seen_readback_ = true;
    const bool enabled = raw == 0.0f;
    this->parent_->queue_sensor_update([this, enabled]() { this->publish_state(enabled); });
  });
}

void PaceBmsDcLimitingSwitch::write_state(bool enabled) {
  if (!this->seen_readback_) {
    ESP_LOGE(TAG, "Wait for a successful DC limiting parameter %u read before writing", this->parameter_);
    return;
  }
  // User-facing ON enables limiting; the vendor uses inverted numeric values.
  // The master publishes only validated F5 readback, not this requested state.
  this->parent_->write_dc_parameter_v25(this->parameter_, enabled ? 0.0f : 1.0f);
}

void PaceBmsDcLimitingSwitch::dump_config() {
  LOG_SWITCH("  ", "DC Current Limiting", this);
  ESP_LOGCONFIG(TAG, "  Parameter: 0x%02X; ON = limiting enabled", this->parameter_);
}
}
