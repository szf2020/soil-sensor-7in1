/**
 * @file sensor_correction.cpp
 * @brief Реализация системы коррекции показаний датчика
 */

#include "sensor_correction.h"
#include "logger.h"
#include <Preferences.h>

// Статические переменные
CorrectionFactors SensorCorrection::factors = {
    .humiditySlope = 1.25f,      // Коэффициент для грунта (40% реальных vs 32% показаний)
    .humidityOffset = -5.0f,     // Смещение для грунта
    .ecSlope = 1.35f,           // Коэффициент для EC в грунте
    .ecOffset = 0.0f,           // Смещение для EC
    .temperatureSlope = 1.0f,   // Коэффициент для температуры (обычно 1.0)
    .temperatureOffset = 0.0f,  // Смещение для температуры
    .enabled = true             // Коррекция включена по умолчанию
};

bool SensorCorrection::initialized = false;

// Глобальный экземпляр
SensorCorrection gSensorCorrection;

void SensorCorrection::init() {
    if (initialized) return;
    
                    logSystem("Инициализация системы коррекции показаний...");
    
    // Загружаем коэффициенты из EEPROM
    loadFactors();
    
                    logSuccess("Система коррекции показаний инициализирована");
    logDebugSafe("Коэффициенты влажности: slope=%.2f, offset=%.1f", 
                 factors.humiditySlope, factors.humidityOffset);
    logDebugSafe("Коэффициенты EC: slope=%.2f, offset=%.1f", 
                 factors.ecSlope, factors.ecOffset);
    
    initialized = true;
}

float SensorCorrection::correctHumidity(uint16_t rawValue) {
    if (!factors.enabled) {
        return rawValue / 10.0f; // Заводская калибровка
    }
    
    // Заводская калибровка: rawValue / 10.0 = %RH
    float factoryCal = rawValue / 10.0f;
    
    // Применяем коррекцию: y = mx + b
    float corrected = (factoryCal * factors.humiditySlope) + factors.humidityOffset;
    
    // Ограничиваем значения 0-100%
    if (corrected < 0.0f) corrected = 0.0f;
    if (corrected > 100.0f) corrected = 100.0f;
    
    logDebugSafe("Коррекция влажности: %u → %.1f → %.1f%%", 
                 rawValue, factoryCal, corrected);
    
    return corrected;
}

float SensorCorrection::correctEC(uint16_t rawValue) {
    if (!factors.enabled) {
        return static_cast<float>(rawValue); // Заводская калибровка
    }
    
    // Заводская калибровка: rawValue = μS/cm
    float factoryCal = static_cast<float>(rawValue);
    
    // Применяем коррекцию: y = mx + b
    float corrected = (factoryCal * factors.ecSlope) + factors.ecOffset;
    
    // Ограничиваем значения > 0
    if (corrected < 0.0f) corrected = 0.0f;
    
    logDebugSafe("Коррекция EC: %u → %.1f → %.1f μS/cm", 
                 rawValue, factoryCal, corrected);
    
    return corrected;
}

float SensorCorrection::correctTemperature(uint16_t rawValue) {
    if (!factors.enabled) {
        return rawValue / 10.0f; // Заводская калибровка
    }
    
    // Заводская калибровка: rawValue / 10.0 = °C
    float factoryCal = rawValue / 10.0f;
    
    // Применяем коррекцию: y = mx + b
    float corrected = (factoryCal * factors.temperatureSlope) + factors.temperatureOffset;
    
    logDebugSafe("Коррекция температуры: %u → %.1f → %.1f°C", 
                 rawValue, factoryCal, corrected);
    
    return corrected;
}

void SensorCorrection::setCorrectionFactors(const CorrectionFactors& newFactors) {
    factors = newFactors;
    logSystem("Коэффициенты коррекции обновлены");
    saveFactors();
}

CorrectionFactors SensorCorrection::getCorrectionFactors() {
    return factors;
}

void SensorCorrection::enableCorrection(bool enabled) {
    factors.enabled = enabled;
    logSystem("Коррекция показаний " + String(enabled ? "включена" : "отключена"));
    saveFactors();
}

void SensorCorrection::saveFactors() {
    Preferences preferences;
    if (preferences.begin("sensor_corr", false)) {
        preferences.putFloat("hum_slope", factors.humiditySlope);
        preferences.putFloat("hum_offset", factors.humidityOffset);
        preferences.putFloat("ec_slope", factors.ecSlope);
        preferences.putFloat("ec_offset", factors.ecOffset);
        preferences.putFloat("temp_slope", factors.temperatureSlope);
        preferences.putFloat("temp_offset", factors.temperatureOffset);
        preferences.putBool("enabled", factors.enabled);
        preferences.end();
        logDebugSafe("Коэффициенты коррекции сохранены в EEPROM");
    }
}

void SensorCorrection::loadFactors() {
    Preferences preferences;
    if (preferences.begin("sensor_corr", true)) {
        factors.humiditySlope = preferences.getFloat("hum_slope", 1.25f);
        factors.humidityOffset = preferences.getFloat("hum_offset", -5.0f);
        factors.ecSlope = preferences.getFloat("ec_slope", 1.35f);
        factors.ecOffset = preferences.getFloat("ec_offset", 0.0f);
        factors.temperatureSlope = preferences.getFloat("temp_slope", 1.0f);
        factors.temperatureOffset = preferences.getFloat("temp_offset", 0.0f);
        factors.enabled = preferences.getBool("enabled", true);
        preferences.end();
        logDebugSafe("Коэффициенты коррекции загружены из EEPROM");
    }
} 