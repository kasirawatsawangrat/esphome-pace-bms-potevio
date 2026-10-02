import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import switch
from esphome.const import (
    CONF_ID,
    CONF_DEVICE_ID,
    CONF_RESTORE_MODE,
    CONF_INVERTED,
    ENTITY_CATEGORY_CONFIG,
)
from .. import pace_bms_base_ns, CONF_PACE_BMS_ID, PaceBmsBase
from ..pace_bms_globals import inherit_device_id

CODEOWNERS = ["@nkinnan"]
DEPENDENCIES = ["pace_bms"]

PaceBmsSwitch = pace_bms_base_ns.class_("PaceBmsSwitch", cg.Component)
PaceBmsSwitchImplementation = pace_bms_base_ns.class_("PaceBmsSwitchImplementation", cg.Component, switch.Switch)
PaceBmsDcLimitingSwitch = pace_bms_base_ns.class_("PaceBmsDcLimitingSwitch", switch.Switch, cg.Component)
DC_SWITCHES = {"discharge_current_limiting": 0x02, "charge_current_limiting": 0x03}


def validate_dc_switch_options(config):
    if not isinstance(config, dict):
        return config  # Let the standard entity schema validate shorthand/errors.
    if str(config.get(CONF_RESTORE_MODE, "DISABLED")).upper() != "DISABLED":
        raise cv.Invalid("DC limiting switches require restore_mode: DISABLED; no startup writes are allowed.")
    if config.get(CONF_INVERTED, False):
        raise cv.Invalid("DC limiting switches cannot be inverted: ON must mean limiting enabled.")
    return config

CONF_BUZZER_ALARM           = "buzzer_alarm"
CONF_LED_ALARM              = "led_alarm"
CONF_CHARGE_CURRENT_LIMITER = "charge_current_limiter"
CONF_CHARGE_MOSFET          = "charge_mosfet"
CONF_DISCHARGE_MOSFET       = "discharge_mosfet"

CONFIG_SCHEMA = cv.All(
    inherit_device_id,
    cv.Schema({
        cv.GenerateID(): cv.declare_id(PaceBmsSwitch),
        cv.GenerateID(CONF_PACE_BMS_ID): cv.use_id(PaceBmsBase),
        cv.Optional(CONF_DEVICE_ID): cv.sub_device_id,

        **{
            cv.Optional(key): cv.All(
                validate_dc_switch_options,
                switch.switch_schema(PaceBmsDcLimitingSwitch,
                                     default_restore_mode="DISABLED",
                                     entity_category=ENTITY_CATEGORY_CONFIG).extend(cv.COMPONENT_SCHEMA),
            )
            for key in DC_SWITCHES
        },

        cv.Optional(CONF_BUZZER_ALARM): switch.switch_schema(PaceBmsSwitchImplementation, default_restore_mode="DISABLED"),
        cv.Optional(CONF_LED_ALARM): switch.switch_schema(PaceBmsSwitchImplementation, default_restore_mode="DISABLED"),
        cv.Optional(CONF_CHARGE_CURRENT_LIMITER): switch.switch_schema(PaceBmsSwitchImplementation, default_restore_mode="DISABLED"),
        cv.Optional(CONF_CHARGE_MOSFET): switch.switch_schema(PaceBmsSwitchImplementation, default_restore_mode="DISABLED"),
        cv.Optional(CONF_DISCHARGE_MOSFET): switch.switch_schema(PaceBmsSwitchImplementation, default_restore_mode="DISABLED"),
    })
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    parent = await cg.get_variable(config[CONF_PACE_BMS_ID])
    cg.add(var.set_parent(parent))

    for key, parameter in DC_SWITCHES.items():
        if conf := config.get(key):
            entity = await switch.new_switch(conf)
            await cg.register_component(entity, conf)
            cg.add(entity.set_parent(parent))
            cg.add(entity.set_parameter(parameter))

    if buzzer_alarm_config := config.get(CONF_BUZZER_ALARM):
        sens = await switch.new_switch(buzzer_alarm_config)
        cg.add(var.set_buzzer_alarm_switch(sens))

    if led_alarm_config := config.get(CONF_LED_ALARM):
        sens = await switch.new_switch(led_alarm_config)
        cg.add(var.set_led_alarm_switch(sens))

    if charge_current_limiter_config := config.get(CONF_CHARGE_CURRENT_LIMITER):
        sens = await switch.new_switch(charge_current_limiter_config)
        cg.add(var.set_charge_current_limiter_switch(sens))

    if charge_mosfet_config := config.get(CONF_CHARGE_MOSFET):
        sens = await switch.new_switch(charge_mosfet_config)
        cg.add(var.set_charge_mosfet_switch(sens))

    if discharge_mosfet_config := config.get(CONF_DISCHARGE_MOSFET):
        sens = await switch.new_switch(discharge_mosfet_config)
        cg.add(var.set_discharge_mosfet_switch(sens))
