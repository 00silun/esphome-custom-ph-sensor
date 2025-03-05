#pragma once
#include "esphome.h"
#include "esphome/components/ads1115/ads1115.h"
#include <cmath>  // For std::isfinite

namespace esphome {
namespace custom_ph_sensor {

class PhSensor : public esphome::PollingComponent, public esphome::sensor::Sensor {
 public:
  // Constructor with ADS and water temperature sensor pointers.
  PhSensor(esphome::sensor::Sensor *ads_sensor, esphome::sensor::Sensor *water_temperature_sensor)
      : esphome::PollingComponent(1000),
        ads_sensor_(ads_sensor),
        water_temperature_sensor_(water_temperature_sensor) {
  }

  void setup() override {
    neutral_voltage_ = 1500.0;  // Default neutral voltage (in mV)
    acid_voltage_ = 2032.44;    // Default acid voltage (in mV)

    // Load calibration voltages from persistent storage.
    esphome::ESPPreferenceObject neutral_pref = esphome::global_preferences->make_preference<float>(0);
    esphome::ESPPreferenceObject acid_pref = esphome::global_preferences->make_preference<float>(4);

    if (neutral_pref.load(&neutral_voltage_)) {
      ESP_LOGD("pH Sensor", "Loaded neutral calibration: %.2f", neutral_voltage_);
    }
    if (acid_pref.load(&acid_voltage_)) {
      ESP_LOGD("pH Sensor", "Loaded acid calibration: %.2f", acid_voltage_);
    }

    // Load persistent calibration flags.
    esphome::ESPPreferenceObject neutral_cal_pref = esphome::global_preferences->make_preference<bool>(8);
    if (!neutral_cal_pref.load(&neutral_calibrated_)) {
      neutral_calibrated_ = false;
    }
    esphome::ESPPreferenceObject acid_cal_pref = esphome::global_preferences->make_preference<bool>(9);
    if (!acid_cal_pref.load(&acid_calibrated_)) {
      acid_calibrated_ = false;
    }
  }

  void update() override {
    if (!ads_sensor_->has_state()) {
      ESP_LOGW("pH Sensor", "ADS1115 has no valid reading yet, publishing default value 7.0");
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
    if (voltage > 0.0 && voltage < 3000.0) {  // Allow any voltage for now
      neutral_voltage_ = voltage;
      esphome::ESPPreferenceObject neutral_pref = esphome::global_preferences->make_preference<float>(0);
      neutral_pref.save(&neutral_voltage_);
      ESP_LOGD("pH Sensor", "pH 7.0 Calibration Completed: %.2f", neutral_voltage_);
      
      // Mark neutral calibration as done and save persistently.
      neutral_calibrated_ = true;
      esphome::ESPPreferenceObject neutral_cal_pref = esphome::global_preferences->make_preference<bool>(8);
      neutral_cal_pref.save(&neutral_calibrated_);
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
    if (voltage > 0.0 && voltage < 3000.0) {  // Allow any voltage for now
      acid_voltage_ = voltage;
      esphome::ESPPreferenceObject acid_pref = esphome::global_preferences->make_preference<float>(4);
      acid_pref.save(&acid_voltage_);
      ESP_LOGD("pH Sensor", "pH 4.0 Calibration Completed: %.2f", acid_voltage_);
      
      // Mark acid calibration as done and save persistently.
      acid_calibrated_ = true;
      esphome::ESPPreferenceObject acid_cal_pref = esphome::global_preferences->make_preference<bool>(9);
      acid_cal_pref.save(&acid_calibrated_);
    } else {
      ESP_LOGD("pH Sensor", "Error: Voltage out of range for pH 4.0 calibration.");
    }
  }

  // Reset calibration indicators (both neutral and acid) and persist the change.
  void reset_calibration_indicator() {
    neutral_calibrated_ = false;
    acid_calibrated_ = false;
    esphome::ESPPreferenceObject neutral_cal_pref = esphome::global_preferences->make_preference<bool>(8);
    neutral_cal_pref.save(&neutral_calibrated_);
    esphome::ESPPreferenceObject acid_cal_pref = esphome::global_preferences->make_preference<bool>(9);
    acid_cal_pref.save(&acid_calibrated_);
    ESP_LOGD("pH Sensor", "Calibration indicators reset");
  }

  // Return true if both calibration steps are done.
  bool is_calibrated() const {
    return neutral_calibrated_ && acid_calibrated_;
  }

 private:
  esphome::sensor::Sensor *ads_sensor_;
  esphome::sensor::Sensor *water_temperature_sensor_;
  float neutral_voltage_;
  float acid_voltage_;
  bool neutral_calibrated_{false};
  bool acid_calibrated_{false};
};

}  // namespace custom_ph_sensor
}  // namespace esphome
