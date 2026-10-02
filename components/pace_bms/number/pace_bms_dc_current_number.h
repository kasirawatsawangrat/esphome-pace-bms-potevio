#pragma once

#include "esphome/core/component.h"
#include "esphome/components/number/number.h"
#include "../pace_bms_component_base.h"

namespace esphome::pace_bms_base {
class PaceBmsDcCurrentNumber : public number::Number, public Component {
 public:
  void set_parent(PaceBmsBase* parent) { parent_ = parent; }
  void set_parameter(uint8_t parameter) { parameter_ = parameter; }
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

 protected:
  void control(float value) override;
  PaceBmsBase* parent_{nullptr};
  // Default preserves the v4 Equalized Charging Current configuration.
  uint8_t parameter_{PaceBmsDcProtocol::EQUALIZED_CHARGING_CURRENT};
  bool seen_readback_{false};
};
}
