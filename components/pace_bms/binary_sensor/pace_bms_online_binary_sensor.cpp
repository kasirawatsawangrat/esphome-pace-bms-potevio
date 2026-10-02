#include "pace_bms_online_binary_sensor.h"

namespace esphome::pace_bms_base {
static const char* const TAG = "pace_bms.online";

void PaceBmsOnlineBinarySensor::setup() {
  if (this->parent_ == nullptr || this->parent_->get_protocol_commandset() != 0x25) {
    ESP_LOGE(TAG, "Online binary sensor requires a v25 parent");
    this->status_set_error();
    return;
  }
  this->parent_->register_online_callback(
      [this](bool online) {
        this->parent_->queue_sensor_update([this, online]() { this->publish_state(online); });
      },
      this->offline_timeout_ms_, millis());
}

void PaceBmsOnlineBinarySensor::dump_config() {
  LOG_BINARY_SENSOR("  ", "PACE BMS Online", this);
  ESP_LOGCONFIG(TAG, "  Offline timeout: %u ms", static_cast<unsigned>(this->offline_timeout_ms_));
}
}
