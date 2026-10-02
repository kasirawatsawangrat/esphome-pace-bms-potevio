#pragma once

#include "esphome/core/component.h"
#include "esphome/components/switch/switch.h"
#include "../pace_bms_component_base.h"

namespace esphome::pace_bms_base {
class PaceBmsDcLimitingSwitch : public switch_::Switch, public Component {
 public:
  void set_parent(PaceBmsBase* parent) { parent_ = parent; }
  void set_parameter(uint8_t parameter) { parameter_ = parameter; }
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

 protected:
  void write_state(bool enabled) override;
  PaceBmsBase* parent_{nullptr};
  uint8_t parameter_{0};
  bool seen_readback_{false};
};
}
