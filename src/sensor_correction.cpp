/**
 * @file sensor_correction.cpp
 * @brief Реализация системы коррекции показаний датчика
 */

#include "sensor_correction.h"
#include "logger.h"
#include <Preferences.h>
#include <cmath>

// Глобальный экземпляр
SensorCorrection gSensorCorrection;

// Конструктор по умолчанию инициализирует factors
SensorCorrection::SensorCorrection() : initialized(false) {
    // Инициализация factors с заводскими значениями (C++17 совместимо)
    // Существующие поля коррекции
    factors.humiditySlope = 1.25f;      // Коэффициент для грунта (40% реальных vs 32% показаний)
    factors.humidityOffset = -5.0f;     // Смещение для грунта
    factors.ecSlope = 1.35f;           // Коэффициент для EC в грунте
    factors.ecOffset = 0.0f;           // Смещение для EC
    factors.temperatureSlope = 1.0f;   // Коэффициент для температуры (обычно 1.0)
    factors.temperatureOffset = 0.0f;  // Смещение для температуры
    factors.enabled = true;            // Коррекция включена по умолчанию
    
    // НОВЫЕ поля для калибровки
    factors.calibrationEnabled = false;     // Калибровка отключена по умолчанию
    
    // pH калибровка
    factors.phSlope = 1.0f;              // Коэффициент pH калибровки
    factors.phOffset = 0.0f;             // Смещение pH калибровки
    factors.phCalibrationQuality = 0.0f; // Качество калибровки pH (R²)
    factors.phCalibrated = false;        // Флаг калибровки pH
    
    // EC калибровка
    factors.ecCalibrationSlope = 1.0f;   // Коэффициент EC калибровки
    factors.ecCalibrationOffset = 0.0f;  // Смещение EC калибровки
    factors.ecCalibrationQuality = 0.0f; // Качество калибровки EC (R²)
    factors.ecCalibrated = false;        // Флаг калибровки EC
    
    // Температурная калибровка
    factors.temperatureCalibrationSlope = 1.0f;   // Коэффициент температурной калибровки
    factors.temperatureCalibrationOffset = 0.0f;  // Смещение температурной калибровки
    factors.temperatureCalibrated = false;        // Флаг калибровки температуры
    
    // Влажность калибровка
    factors.humidityCalibrationSlope = 1.0f;      // Коэффициент влажностной калибровки
    factors.humidityCalibrationOffset = 0.0f;     // Смещение влажностной калибровки
    factors.humidityCalibrated = false;           // Флаг калибровки влажности
    
    // NPK калибровка (нулевая точка)
    factors.npkNitrogenOffset = 0.0f;    // Смещение нуля для азота
    factors.npkPhosphorusOffset = 0.0f;  // Смещение нуля для фосфора
    factors.npkPotassiumOffset = 0.0f;   // Смещение нуля для калия
    factors.npkCalibrated = false;       // Флаг калибровки NPK
    
    // Температурная компенсация
    factors.temperatureCompensationEnabled = true; // Включена по умолчанию
    factors.temperatureReference = 25.0f;          // Референсная температура (25°C)
    
    // История калибровок
    factors.lastCalibrationTime = 0;     // Время последней калибровки
    factors.lastCalibratedBy = "";       // Кто калибровал
}

void SensorCorrection::init() {
    if (this->initialized) return;
    
    logSystem("Инициализация системы коррекции показаний...");
    
    // Загружаем коэффициенты из EEPROM
    this->loadFactors();
    
    logSuccess("Система коррекции показаний инициализирована");
    logDebugSafe("Коэффициенты влажности: slope=%.2f, offset=%.1f", 
                 this->factors.humiditySlope, this->factors.humidityOffset);
    logDebugSafe("Коэффициенты EC: slope=%.2f, offset=%.1f", 
                 this->factors.ecSlope, this->factors.ecOffset);
    logDebugSafe("Калибровка: %s", this->factors.calibrationEnabled ? "включена" : "отключена");
    
    this->initialized = true;
}

bool SensorCorrection::isInitialized() {
    return this->initialized;
}

float SensorCorrection::correctHumidity(uint16_t rawValue) {
    if (!this->factors.enabled) {
        return rawValue / 10.0f; // Заводская калибровка
    }
    
    // Заводская калибровка: rawValue / 10.0 = %RH
    float factoryCal = rawValue / 10.0f;
    
    // Применяем калибровочную коррекцию если включена
    if (this->factors.calibrationEnabled && this->factors.humidityCalibrated) {
        factoryCal = (factoryCal * this->factors.humidityCalibrationSlope) + this->factors.humidityCalibrationOffset;
    }
    
    // Применяем коррекцию: y = mx + b
    float corrected = (factoryCal * this->factors.humiditySlope) + this->factors.humidityOffset;
    
    // Ограничиваем значения 0-100%
    if (corrected < 0.0f) corrected = 0.0f;
    if (corrected > 100.0f) corrected = 100.0f;
    
    logDebugSafe("Коррекция влажности: %u → %.1f → %.1f%%", 
                 rawValue, factoryCal, corrected);
    
    return corrected;
}

float SensorCorrection::correctEC(uint16_t rawValue) {
    if (!this->factors.enabled) {
        return static_cast<float>(rawValue); // Заводская калибровка
    }
    
    // Заводская калибровка: rawValue = μS/cm
    float factoryCal = static_cast<float>(rawValue);
    
    // Применяем калибровочную коррекцию если включена
    if (this->factors.calibrationEnabled && this->factors.ecCalibrated) {
        factoryCal = (factoryCal * this->factors.ecCalibrationSlope) + this->factors.ecCalibrationOffset;
    }
    
    // Применяем коррекцию: y = mx + b
    float corrected = (factoryCal * this->factors.ecSlope) + this->factors.ecOffset;
    
    // Ограничиваем значения > 0
    if (corrected < 0.0f) corrected = 0.0f;
    
    logDebugSafe("Коррекция EC: %u → %.1f → %.1f μS/cm", 
                 rawValue, factoryCal, corrected);
    
    return corrected;
}

float SensorCorrection::correctTemperature(uint16_t rawValue) {
    if (!this->factors.enabled) {
        return rawValue / 10.0f; // Заводская калибровка
    }
    
    // Заводская калибровка: rawValue / 10.0 = °C
    float factoryCal = rawValue / 10.0f;
    
    // Применяем калибровочную коррекцию если включена
    if (this->factors.calibrationEnabled && this->factors.temperatureCalibrated) {
        factoryCal = (factoryCal * this->factors.temperatureCalibrationSlope) + this->factors.temperatureCalibrationOffset;
    }
    
    // Применяем коррекцию: y = mx + b
    float corrected = (factoryCal * this->factors.temperatureSlope) + this->factors.temperatureOffset;
    
    logDebugSafe("Коррекция температуры: %u → %.1f → %.1f°C", 
                 rawValue, factoryCal, corrected);
    
    return corrected;
}

// НОВЫЕ: Коррекция pH
float SensorCorrection::correctPH(uint16_t rawValue) {
    if (!this->factors.calibrationEnabled || !this->factors.phCalibrated) {
        return rawValue / 10.0f; // Заводская калибровка
    }
    
    // Заводская калибровка: rawValue / 10.0 = pH
    float factoryCal = rawValue / 10.0f;
    
    // Применяем калибровочную коррекцию: y = mx + b
    float calibrated = (factoryCal * this->factors.phSlope) + this->factors.phOffset;
    
    // Применяем температурную компенсацию если включена
    if (this->factors.temperatureCompensationEnabled) {
        float currentTemp = getCurrentTemperature();
        calibrated = applyTemperatureCompensation(calibrated, currentTemp);
    }
    
    logDebugSafe("Коррекция pH: %u → %.2f → %.2f pH", 
                 rawValue, factoryCal, calibrated);
    
    return calibrated;
}

// НОВЫЕ: Коррекция NPK (нулевая точка)
void SensorCorrection::correctNPK(uint16_t rawN, uint16_t rawP, uint16_t rawK, 
                                 float& nitrogen, float& phosphorus, float& potassium) {
    if (!this->factors.calibrationEnabled || !this->factors.npkCalibrated) {
        // Заводская калибровка без смещения нуля
        nitrogen = static_cast<float>(rawN);
        phosphorus = static_cast<float>(rawP);
        potassium = static_cast<float>(rawK);
        return;
    }
    
    // Заводская калибровка
    nitrogen = static_cast<float>(rawN);
    phosphorus = static_cast<float>(rawP);
    potassium = static_cast<float>(rawK);
    
    // Применяем смещение нуля (калибровка по дистиллированной воде)
    nitrogen -= this->factors.npkNitrogenOffset;
    phosphorus -= this->factors.npkPhosphorusOffset;
    potassium -= this->factors.npkPotassiumOffset;
    
    // Ограничиваем значения > 0
    if (nitrogen < 0.0f) nitrogen = 0.0f;
    if (phosphorus < 0.0f) phosphorus = 0.0f;
    if (potassium < 0.0f) potassium = 0.0f;
    
    logDebugSafe("Коррекция NPK: N:%u→%.1f P:%u→%.1f K:%u→%.1f mg/kg", 
                 rawN, nitrogen, rawP, phosphorus, rawK, potassium);
}

// НОВЫЕ: Температурная компенсация pH
float SensorCorrection::applyTemperatureCompensation(float value, float temperature) {
    // pH температурная компенсация по уравнению Нернста
    // Правильная константа: 0.0169 pH/°C (исправлено по замечанию CodeRabbit)
    float tempDiff = temperature - this->factors.temperatureReference;
    float compensation = -0.0169f * tempDiff; // -0.0169 pH/°C
    return value + compensation;
}

// НОВЫЕ: Получение текущей температуры для компенсации
float SensorCorrection::getCurrentTemperature() {
    // Получаем текущую температуру из датчика (регистр 0x0013)
    extern uint16_t getSensorTemperature(); // Объявление внешней функции
    
    uint16_t rawTemp = getSensorTemperature();
    
    // Проверяем, что датчик отвечает (0xFFFF означает ошибку чтения)
    // 0°C - это валидное значение температуры, поэтому проверяем только ошибку
    if (rawTemp != 0xFFFF) {
        // Применяем заводскую калибровку: rawValue / 10.0 = °C
        float currentTemp = rawTemp / 10.0f;
        
        // Применяем калибровочную коррекцию если включена
        if (this->factors.calibrationEnabled && this->factors.temperatureCalibrated) {
            currentTemp = (currentTemp * this->factors.temperatureCalibrationSlope) + this->factors.temperatureCalibrationOffset;
        }
        
        // Применяем коррекцию: y = mx + b
        if (this->factors.enabled) {
            currentTemp = (currentTemp * this->factors.temperatureSlope) + this->factors.temperatureOffset;
        }
        
        return currentTemp;
    }
    
    // Fallback: возвращаем референсную температуру только при ошибке чтения
    logWarnSafe("Не удалось прочитать температуру из датчика, используем референсную: %.1f°C", this->factors.temperatureReference);
    return this->factors.temperatureReference;
}

// НОВЫЕ: Включение/отключение калибровки
void SensorCorrection::enableCalibration(bool enabled) {
    this->factors.calibrationEnabled = enabled;
    logSystem("Калибровка показаний " + String(enabled ? "включена" : "отключена"));
    saveFactors();
}

// НОВЫЕ: Функции калибровки
CalibrationResult SensorCorrection::calculatePHCalibration(
    float expected_4_01, float expected_6_86, float expected_9_18,
    float measured_4_01, float measured_6_86, float measured_9_18
) {
    CalibrationResult result = {false, 0.0f, 0.0f, 0.0f, ""};
    
    // Проверяем валидность данных
    if (measured_4_01 <= 0 || measured_6_86 <= 0 || measured_9_18 <= 0) {
        return result;
    }
    
    // Вычисляем наклон и смещение методом наименьших квадратов (каноническая форма)
    float x1 = measured_4_01, y1 = expected_4_01;
    float x2 = measured_6_86, y2 = expected_6_86;
    float x3 = measured_9_18, y3 = expected_9_18;
    
    // Каноническая форма наименьших квадратов (избегаем переполнения)
    const float Sx = x1 + x2 + x3;
    const float Sy = y1 + y2 + y3;
    const float Sxx = x1*x1 + x2*x2 + x3*x3;
    const float Sxy = x1*y1 + x2*y2 + x3*y3;
    
    // Вычисляем наклон (slope)
    const float numerator = 3.0f * Sxy - Sx * Sy;
    const float denominator = 3.0f * Sxx - Sx * Sx;
    
    if (fabsf(denominator) < 0.001f) {
        return result; // Деление на ноль
    }
    
    result.slope = numerator / denominator;
    
    // Вычисляем смещение (offset)
    result.offset = (y1 + y2 + y3 - result.slope * (x1 + x2 + x3)) / 3.0f;
    
    // Вычисляем R² (коэффициент корреляции)
    float y_mean = (y1 + y2 + y3) / 3.0f;
    float ss_tot = pow(y1 - y_mean, 2) + pow(y2 - y_mean, 2) + pow(y3 - y_mean, 2);
    
    float y1_pred = result.slope * x1 + result.offset;
    float y2_pred = result.slope * x2 + result.offset;
    float y3_pred = result.slope * x3 + result.offset;
    
    float ss_res = pow(y1 - y1_pred, 2) + pow(y2 - y2_pred, 2) + pow(y3 - y3_pred, 2);
    
    if (ss_tot > 0.001f) {
        result.r_squared = 1.0f - (ss_res / ss_tot);
    } else {
        result.r_squared = 0.0f;
    }
    
    result.success = (result.r_squared > 0.95f); // Минимальное качество
    
    // Определяем качество
    if (result.r_squared >= 0.995f) result.quality = "Отличное";
    else if (result.r_squared >= 0.99f) result.quality = "Хорошее";
    else if (result.r_squared >= 0.98f) result.quality = "Удовлетворительное";
    else result.quality = "Плохое";
    
    return result;
}

CalibrationResult SensorCorrection::calculateECCalibration(
    float expected_1, float expected_2,
    float measured_1, float measured_2
) {
    CalibrationResult result = {false, 0.0f, 0.0f, 0.0f, ""};
    
    // Проверяем валидность данных
    if (measured_1 <= 0 || measured_2 <= 0) {
        return result;
    }
    
    // 2-точечная калибровка (линейная)
    const float x1 = measured_1, y1 = expected_1;
    const float x2 = measured_2, y2 = expected_2;

    // Вычисляем наклон (slope) для 2 точек
    if (fabsf(x2 - x1) < 0.001f) {
        return result; // Деление на ноль
    }

    result.slope = (y2 - y1) / (x2 - x1);
    result.offset = y1 - result.slope * x1;

    // Для 2 точек R² = 1.0 (идеальная прямая)
    result.r_squared = 1.0f;
    
    // Проверяем качество калибровки
    result.success = (result.r_squared >= 0.95f);
    
    // Определяем качество
    if (result.r_squared >= 0.995f) result.quality = "Отличное";
    else if (result.r_squared >= 0.99f) result.quality = "Хорошее";
    else if (result.r_squared >= 0.98f) result.quality = "Удовлетворительное";
    else result.quality = "Плохое";
    
    return result;
}

CalibrationResult SensorCorrection::calculateTemperatureCalibration(
    float referenceTemperature, float measuredTemperature
) {
    CalibrationResult result = {false, 0.0f, 0.0f, 0.0f, ""};
    
    // Линейная калибровка температуры
    result.slope = 1.0f; // Обычно 1.0 для температуры
    result.offset = referenceTemperature - measuredTemperature;
    
    // Вычисляем качество на основе ошибки
    float error = fabsf(result.offset) / referenceTemperature;
    result.r_squared = 1.0f - error;
    
    result.success = (fabsf(result.offset) < 2.0f); // Ошибка менее 2°C
    
    // Определяем качество
    if (fabsf(result.offset) < 0.5f) result.quality = "Отличное";
    else if (fabsf(result.offset) < 1.0f) result.quality = "Хорошее";
    else if (fabsf(result.offset) < 2.0f) result.quality = "Удовлетворительное";
    else result.quality = "Плохое";
    
    return result;
}

CalibrationResult SensorCorrection::calculateHumidityCalibration(
    float referenceHumidity, float measuredHumidity
) {
    CalibrationResult result = {false, 0.0f, 0.0f, 0.0f, ""};
    
    // Линейная калибровка влажности
    result.slope = 1.0f; // Обычно 1.0 для влажности
    result.offset = referenceHumidity - measuredHumidity;
    
    // Вычисляем качество на основе ошибки
    float error = fabsf(result.offset) / referenceHumidity;
    result.r_squared = 1.0f - error;
    
    result.success = (fabsf(result.offset) < 5.0f); // Ошибка менее 5%
    
    // Определяем качество
    if (fabsf(result.offset) < 1.0f) result.quality = "Отличное";
    else if (fabsf(result.offset) < 2.0f) result.quality = "Хорошее";
    else if (fabsf(result.offset) < 5.0f) result.quality = "Удовлетворительное";
    else result.quality = "Плохое";
    
    return result;
} 

// Добавляем недостающие функции
void SensorCorrection::setCorrectionFactors(const CorrectionFactors& newFactors) {
    logDebugSafe("🔧 Обновляем коэффициенты коррекции...");
    factors = newFactors;
    logSystem("Коэффициенты коррекции обновлены");
    logDebugSafe("🔧 EC калибровка: slope=%.4f, offset=%.4f, calibrated=%s", 
                 this->factors.ecCalibrationSlope, this->factors.ecCalibrationOffset, 
                 this->factors.ecCalibrated ? "true" : "false");
    saveFactors();
}

CorrectionFactors SensorCorrection::getCorrectionFactors() {
    return factors;
}

void SensorCorrection::enableCorrection(bool enabled) {
    this->factors.enabled = enabled;
    logSystem("Коррекция показаний " + String(enabled ? "включена" : "отключена"));
    saveFactors();
}

void SensorCorrection::saveFactors() {
    logDebugSafe("🔧 Начинаем сохранение коэффициентов коррекции...");
    Preferences preferences;
    if (preferences.begin("sensor_corr", false)) {
        logDebugSafe("✅ Preferences открыты успешно");
        
        // Существующие поля
        preferences.putFloat("hum_slope", this->factors.humiditySlope);
        preferences.putFloat("hum_offset", this->factors.humidityOffset);
        preferences.putFloat("ec_slope", this->factors.ecSlope);
        preferences.putFloat("ec_offset", this->factors.ecOffset);
        preferences.putFloat("temp_slope", this->factors.temperatureSlope);
        preferences.putFloat("temp_offset", this->factors.temperatureOffset);
        preferences.putBool("enabled", this->factors.enabled);
        
        // НОВЫЕ поля калибровки
        preferences.putBool("calibration_enabled", this->factors.calibrationEnabled);
        
        // pH калибровка
        preferences.putFloat("ph_slope", this->factors.phSlope);
        preferences.putFloat("ph_offset", this->factors.phOffset);
        preferences.putFloat("ph_quality", this->factors.phCalibrationQuality);
        preferences.putBool("ph_calibrated", this->factors.phCalibrated);
        
        // EC калибровка
        preferences.putFloat("ec_cal_slope", this->factors.ecCalibrationSlope);
        preferences.putFloat("ec_cal_offset", this->factors.ecCalibrationOffset);
        preferences.putFloat("ec_cal_quality", this->factors.ecCalibrationQuality);
        preferences.putBool("ec_calibrated", this->factors.ecCalibrated);
        
        // Температурная калибровка
        preferences.putFloat("temp_cal_slope", this->factors.temperatureCalibrationSlope);
        preferences.putFloat("temp_cal_offset", this->factors.temperatureCalibrationOffset);
        preferences.putBool("temp_calibrated", this->factors.temperatureCalibrated);
        
        // Влажностная калибровка
        preferences.putFloat("hum_cal_slope", this->factors.humidityCalibrationSlope);
        preferences.putFloat("hum_cal_offset", this->factors.humidityCalibrationOffset);
        preferences.putBool("hum_calibrated", this->factors.humidityCalibrated);
        
        // NPK калибровка
        preferences.putFloat("npk_n_offset", this->factors.npkNitrogenOffset);
        preferences.putFloat("npk_p_offset", this->factors.npkPhosphorusOffset);
        preferences.putFloat("npk_k_offset", this->factors.npkPotassiumOffset);
        preferences.putBool("npk_calibrated", this->factors.npkCalibrated);
        
        // Температурная компенсация
        preferences.putBool("temp_comp_enabled", this->factors.temperatureCompensationEnabled);
        preferences.putFloat("temp_reference", this->factors.temperatureReference);
        
        // История калибровок
        preferences.putULong("last_calibration_time", this->factors.lastCalibrationTime);
        preferences.putString("last_calibrated_by", String(this->factors.lastCalibratedBy.c_str()));
        
        preferences.end();
        logSuccess("✅ Коэффициенты коррекции и калибровки сохранены в EEPROM");
        logDebugSafe("🔧 EC калибровка: slope=%.4f, offset=%.4f, calibrated=%s", 
                     this->factors.ecCalibrationSlope, this->factors.ecCalibrationOffset, 
                     this->factors.ecCalibrated ? "true" : "false");
    } else {
        logErrorSafe("❌ Ошибка открытия Preferences для сохранения");
    }
}

void SensorCorrection::loadFactors() {
    Preferences preferences;
    if (preferences.begin("sensor_corr", true)) {
        // Существующие поля
        this->factors.humiditySlope = preferences.getFloat("hum_slope", 1.25f);
        this->factors.humidityOffset = preferences.getFloat("hum_offset", -5.0f);
        this->factors.ecSlope = preferences.getFloat("ec_slope", 1.35f);
        this->factors.ecOffset = preferences.getFloat("ec_offset", 0.0f);
        this->factors.temperatureSlope = preferences.getFloat("temp_slope", 1.0f);
        this->factors.temperatureOffset = preferences.getFloat("temp_offset", 0.0f);
        this->factors.enabled = preferences.getBool("enabled", true);
        
        // НОВЫЕ поля калибровки
        this->factors.calibrationEnabled = preferences.getBool("calibration_enabled", false);
        
        // pH калибровка
        this->factors.phSlope = preferences.getFloat("ph_slope", 1.0f);
        this->factors.phOffset = preferences.getFloat("ph_offset", 0.0f);
        this->factors.phCalibrationQuality = preferences.getFloat("ph_quality", 0.0f);
        this->factors.phCalibrated = preferences.getBool("ph_calibrated", false);
        
        // EC калибровка
        this->factors.ecCalibrationSlope = preferences.getFloat("ec_cal_slope", 1.0f);
        this->factors.ecCalibrationOffset = preferences.getFloat("ec_cal_offset", 0.0f);
        this->factors.ecCalibrationQuality = preferences.getFloat("ec_cal_quality", 0.0f);
        this->factors.ecCalibrated = preferences.getBool("ec_calibrated", false);
        
        // Температурная калибровка
        this->factors.temperatureCalibrationSlope = preferences.getFloat("temp_cal_slope", 1.0f);
        this->factors.temperatureCalibrationOffset = preferences.getFloat("temp_cal_offset", 0.0f);
        this->factors.temperatureCalibrated = preferences.getBool("temp_calibrated", false);
        
        // Влажностная калибровка
        this->factors.humidityCalibrationSlope = preferences.getFloat("hum_cal_slope", 1.0f);
        this->factors.humidityCalibrationOffset = preferences.getFloat("hum_cal_offset", 0.0f);
        this->factors.humidityCalibrated = preferences.getBool("hum_calibrated", false);
        
        // NPK калибровка
        this->factors.npkNitrogenOffset = preferences.getFloat("npk_n_offset", 0.0f);
        this->factors.npkPhosphorusOffset = preferences.getFloat("npk_p_offset", 0.0f);
        this->factors.npkPotassiumOffset = preferences.getFloat("npk_k_offset", 0.0f);
        this->factors.npkCalibrated = preferences.getBool("npk_calibrated", false);
        
        // Температурная компенсация
        this->factors.temperatureCompensationEnabled = preferences.getBool("temp_comp_enabled", true);
        this->factors.temperatureReference = preferences.getFloat("temp_reference", 25.0f);
        
        // История калибровок
        this->factors.lastCalibrationTime = preferences.getULong("last_calibration_time", 0);
        String lastCalibratedBy = preferences.getString("last_calibrated_by", "");
        this->factors.lastCalibratedBy = lastCalibratedBy.c_str();
        
        preferences.end();
        logDebugSafe("Коэффициенты коррекции и калибровки загружены из EEPROM");
    }
} 
