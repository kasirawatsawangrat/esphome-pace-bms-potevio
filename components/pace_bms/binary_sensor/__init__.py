import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import CONF_DEVICE_ID, DEVICE_CLASS_CONNECTIVITY, ENTITY_CATEGORY_DIAGNOSTIC
from .. import pace_bms_base_ns, CONF_PACE_BMS_ID, PaceBmsBase
from ..pace_bms_globals import inherit_device_id

CODEOWNERS = ["@nkinnan"]
DEPENDENCIES = ["pace_bms"]

PaceBmsOnlineBinarySensor = pace_bms_base_ns.class_(
    "PaceBmsOnlineBinarySensor", binary_sensor.BinarySensor, cg.Component
)
CONF_ONLINE = "online"
CONF_OFFLINE_TIMEOUT = "offline_timeout"

CONFIG_SCHEMA = cv.All(
    inherit_device_id,
    cv.Schema({
        cv.GenerateID(CONF_PACE_BMS_ID): cv.use_id(PaceBmsBase),
        cv.Optional(CONF_DEVICE_ID): cv.sub_device_id,
        cv.Optional(CONF_ONLINE): binary_sensor.binary_sensor_schema(
            PaceBmsOnlineBinarySensor,
            device_class=DEVICE_CLASS_CONNECTIVITY,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ).extend(cv.COMPONENT_SCHEMA).extend({
            cv.Optional(CONF_OFFLINE_TIMEOUT, default="15s"): cv.All(
                cv.positive_time_period_milliseconds,
                cv.Range(min=cv.TimePeriod(milliseconds=1),
                         max=cv.TimePeriod(milliseconds=0x7FFFFFFF)),
            ),
        }),
    })
)

async def to_code(config):
    parent = await cg.get_variable(config[CONF_PACE_BMS_ID])
    if conf := config.get(CONF_ONLINE):
        entity = await binary_sensor.new_binary_sensor(conf)
        await cg.register_component(entity, conf)
        cg.add(entity.set_parent(parent))
        cg.add(entity.set_offline_timeout(conf[CONF_OFFLINE_TIMEOUT].total_milliseconds))
