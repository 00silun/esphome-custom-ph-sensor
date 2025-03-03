import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import CONF_ID

# Create a namespace matching your C++ namespace.
custom_ph_sensor_ns = cg.esphome_ns.namespace("custom_ph_sensor")

# Declare your C++ class.
PhSensor = custom_ph_sensor_ns.class_("PhSensor", cg.PollingComponent, sensor.Sensor)

CONFIG_SCHEMA = cv.Schema({
  cv.GenerateID(): cv.declare_id(PhSensor),
  cv.Required("ads_sensor"): cv.use_id(sensor.Sensor),
  cv.Required("water_temperature"): cv.use_id(sensor.Sensor)
}).extend(sensor.sensor_schema(PhSensor))

# Provide a dummy (nonempty) service schema as a raw C++ expression.
SERVICE_SCHEMA = cg.RawExpression("({\"dummy\": 0})")

# Helper: Create a plain function (not a lambda) that calls the desired method.
def make_calibrate_callback(instance, func):
    def callback(args):
        func()
        return None  # Must return something (here, None)
    return callback

async def to_code(config):
    ads_sensor = await cg.get_variable(config["ads_sensor"])
    water_temperature = await cg.get_variable(config["water_temperature"])
    var = cg.new_Pvariable(config[CONF_ID], ads_sensor, water_temperature)
    await cg.register_component(var, config)
    await sensor.register_sensor(var, config)

    # Create callback functions using the helper.
    calibrate_ph7_cb = make_calibrate_callback(var, var.calibrate_neutral)
    calibrate_ph4_cb = make_calibrate_callback(var, var.calibrate_acid)

    # Register the services with the nonempty dummy schema and the plain callbacks.
    cg.add(var.register_service("calibrate_ph7", SERVICE_SCHEMA, calibrate_ph7_cb))
    cg.add(var.register_service("calibrate_ph4", SERVICE_SCHEMA, calibrate_ph4_cb))
