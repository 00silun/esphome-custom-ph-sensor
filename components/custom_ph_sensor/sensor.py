import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import CONF_ID

# Create a namespace matching your C++ namespace structure.
esp_home_ns = cg.esphome_ns.namespace('esp_home')
custom_ph_sensor_ns = esp_home_ns.namespace('custom_ph_sensor')

# Declare your C++ class.
PhSensor = custom_ph_sensor_ns.class_('PhSensor', cg.PollingComponent, sensor.Sensor)

# Define the configuration schema; now only requiring the ADS sensor dependency.
CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(PhSensor),
    cv.Required("ads_sensor"): cv.use_id(sensor.Sensor),
}).extend(sensor.sensor_schema(PhSensor))



import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import CONF_ID

custom_ph_sensor_ns = cg.esphome_ns.namespace("custom_ph_sensor")

# Declare your C++ class 
PhSensor = custom_ph_sensor_ns.class_("PhSensor", cg.PollingComponent, sensor.Sensor)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(PhSensor),
    cv.Required("ads_sensor"): cv.use_id(sensor.Sensor),
    cv.Required("water_temperature"): cv.use_id(sensor.Sensor)
    
}).extend(sensor.sensor_schema(pHSensor))

async def to_code(config):
    ads_sensor = await cg.get_variable(config["ads_sensor"])
    water_temperature = await cg.get_variable(config["water_temperature"])
    var = cg.new_Pvariable(config[CONF_ID], ads_sensor, water_temperature)
    await cg.register_component(var, config)
    await sensor.register_sensor(var, config)
    
    
    
    
    
    