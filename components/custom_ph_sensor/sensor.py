import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import CONF_ID

custom_ph_sensor_ns = cg.esphome_ns.namespace("custom_ph_sensor")

# Declare the C++ class without pulling in register_service
PhSensor = custom_ph_sensor_ns.class_("PhSensor", cg.PollingComponent, sensor.Sensor)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(PhSensor),
    cv.Required("ads_sensor"): cv.use_id(sensor.Sensor),
    cv.Required("water_temperature"): cv.use_id(sensor.Sensor)
}).extend(sensor.sensor_schema(PhSensor))

async def to_code(config):
    ads_sensor = await cg.get_variable(config["ads_sensor"])
    water_temperature = await cg.get_variable(config["water_temperature"])
    var = cg.new_Pvariable(config[CONF_ID], ads_sensor, water_temperature)
    await cg.register_component(var, config)
    await sensor.register_sensor(var, config)


