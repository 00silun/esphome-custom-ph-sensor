#pragma once
#include "esphome.h"
#include "esphome/components/ads1115/ads1115.h"
#include <cmath>  // For std::isfinite

namespace esphome {
namespace custom_ph_sensor {

class PhSensor : public esphome::PollingComponent, public esphome::sensor::Sensor {
 public:
  // Constructor now accepts both the ADS sensor and water temperature sensor pointers.
  PhSensor(esphome::sensor::Sensor *ads_sensor, esphome::sensor::Sensor *water_temperature_sensor)
      : esphome::PollingComponent(1000),
        ads_sensor_(ads_sensor),
        water_temperature_sensor_(water_temperature_sensor) {
    ESP_LOGD("pH Sensor", "PhSensor constructor called");
  }

  void setup() override {
    ESP_LOGD("pH Sensor", "PhSensor setup() called");
    // Set default calibration values.
    neutral_voltage_ = 1500.0;  // Default neutral voltage (in mV)
    acid_voltage_ = 2032.44;    // Default acid voltage (in mV)

    esphome::ESPPreferenceObject neutral_pref = esphome::global_preferences->make_preference<float>(0);
    esphome::ESPPreferenceObject acid_pref = esphome::global_preferences->make_preference<float>(4);

    if (neutral_pref.load(&neutral_voltage_)) {
      ESP_LOGD("pH Sensor", "Loaded neutral calibration: %.2f", neutral_voltage_);
    }
    if (acid_pref.load(&acid_voltage_)) {
      ESP_LOGD("pH Sensor", "Loaded acid calibration: %.2f", acid_voltage_);
    }
    // Register the custom service that will trigger calibrate_neutral()
    this->register_service("calibrate_neutral", &PhSensor::calibrate_neutral);
  }

  void update() override {
    ESP_LOGD("pH Sensor", "PhSensor update() called");
    
    if (!ads_sensor_->has_state()) {
      ESP_LOGW("pH Sensor", "ADS1115 has no valid reading yet, publishing default value 7.0");
      publish_state(7.0);
      return;
    }

    float voltage = ads_sensor_->state * 1000.0;  // Convert to millivolts

    // Retrieve water temperature reading and default to 25°C if invalid.
    float temp = water_temperature_sensor_->state;
    if (!std::isfinite(temp)) {
      ESP_LOGW("pH Sensor", "Water temperature reading invalid; defaulting to 25°C");
      temp = 25.0;
    }
    float temperature = temp;

    // Temperature compensation computation.
    float temp_coefficient = 0.03 * (temperature - 25.0);  // 0 at 25°C
    float temp_adjusted_slope = (7.0 - 4.0) /
        (((neutral_voltage_ - 1500.0) / 3.0) - ((acid_voltage_ - 1500.0) / 3.0));
    temp_adjusted_slope *= (1.0 + temp_coefficient);

    float intercept = 7.0 - temp_adjusted_slope * ((neutral_voltage_ - 1500.0) / 3.0);
    float phValue = temp_adjusted_slope * (voltage - 1500.0) / 3.0 + intercept;

    ESP_LOGD("pH Sensor", "Raw Voltage: %.2f mV, Temp: %.2f°C, Computed pH: %.2f", voltage, temperature, phValue);
    publish_state(phValue);
  }

  void calibrate_neutral() {
    if (!ads_sensor_->has_state()) {
      ESP_LOGW("pH Sensor", "Cannot calibrate pH 7.0: No valid ADS1115 reading.");
      return;
    }

    float voltage = ads_sensor_->state * 1000.0;  // Convert to millivolts
    if (voltage > 1322.0 && voltage < 1678.0) {
      neutral_voltage_ = voltage;
      esphome::ESPPreferenceObject neutral_pref = esphome::global_preferences->make_preference<float>(0);
      neutral_pref.save(&neutral_voltage_);
      ESP_LOGD("pH Sensor", "pH 7.0 Calibration Completed: %.2f", neutral_voltage_);
    } else {
      ESP_LOGD("pH Sensor", "Error: Voltage out of range for pH 7.0 calibration.");
    }
  }

  void calibrate_acid() {
    if (!ads_sensor_->has_state()) {
      ESP_LOGW("pH Sensor", "Cannot calibrate pH 4.0: No valid ADS1115 reading.");
      return;
    }

    float voltage = ads_sensor_->state * 1000.0;  // Convert to millivolts
    if (voltage > 1854.0 && voltage < 2210.0) {
      acid_voltage_ = voltage;
      esphome::ESPPreferenceObject acid_pref = esphome::global_preferences->make_preference<float>(4);
      acid_pref.save(&acid_voltage_);
      ESP_LOGD("pH Sensor", "pH 4.0 Calibration Completed: %.2f", acid_voltage_);
    } else {
      ESP_LOGD("pH Sensor", "Error: Voltage out of range for pH 4.0 calibration.");
    }
  }

 private:
  esphome::sensor::Sensor *ads_sensor_;
  esphome::sensor::Sensor *water_temperature_sensor_;
  float neutral_voltage_;
  float acid_voltage_;
};

}  // namespace custom_ph_sensor
}  // namespace esphome
