#pragma once

#include "esphome/core/component.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "../pace_bms_component_base.h"

namespace esphome::pace_bms_base {
class PaceBmsOnlineBinarySensor : public binary_sensor::BinarySensor, public Component {
 public:
  void set_parent(PaceBmsBase* parent) { parent_ = parent; }
  void set_offline_timeout(uint32_t milliseconds) { offline_timeout_ms_ = milliseconds; }
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

 protected:
  PaceBmsBase* parent_{nullptr};
  uint32_t offline_timeout_ms_{15000};
};
}
