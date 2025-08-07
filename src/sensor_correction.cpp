/**
 * @file sensor_correction.cpp
 * @brief Реализация системы коррекции показаний датчика
 */

#include "sensor_correction.h"
#include "logger.h"
#include <Preferences.h>
#include <cmath>

// Статические переменные
CorrectionFactors SensorCorrection::factors = {
    // Существующие поля коррекции
    .humiditySlope = 1.25f,      // Коэффициент для грунта (40% реальных vs 32% показаний)
    .humidityOffset = -5.0f,     // Смещение для грунта
    .ecSlope = 1.35f,           // Коэффициент для EC в грунте
    .ecOffset = 0.0f,           // Смещение для EC
    .temperatureSlope = 1.0f,   // Коэффициент для температуры (обычно 1.0)
    .temperatureOffset = 0.0f,  // Смещение для температуры
    .enabled = true,            // Коррекция включена по умолчанию
    
    // НОВЫЕ поля для калибровки
    .calibrationEnabled = false,     // Калибровка отключена по умолчанию
    
    // pH калибровка
    .phSlope = 1.0f,              // Коэффициент pH калибровки
    .phOffset = 0.0f,             // Смещение pH калибровки
    .phCalibrationQuality = 0.0f, // Качество калибровки pH (R²)
    .phCalibrated = false,        // Флаг калибровки pH
    
    // EC калибровка
    .ecCalibrationSlope = 1.0f,   // Коэффициент EC калибровки
    .ecCalibrationOffset = 0.0f,  // Смещение EC калибровки
    .ecCalibrationQuality = 0.0f, // Качество калибровки EC (R²)
    .ecCalibrated = false,        // Флаг калибровки EC
    
    // Температурная калибровка
    .temperatureCalibrationSlope = 1.0f,   // Коэффициент температурной калибровки
    .temperatureCalibrationOffset = 0.0f,  // Смещение температурной калибровки
    .temperatureCalibrated = false,        // Флаг калибровки температуры
    
    // Влажность калибровка
    .humidityCalibrationSlope = 1.0f,      // Коэффициент влажностной калибровки
    .humidityCalibrationOffset = 0.0f,     // Смещение влажностной калибровки
    .humidityCalibrated = false,           // Флаг калибровки влажности
    
    // NPK калибровка (нулевая точка)
    .npkNitrogenOffset = 0.0f,    // Смещение нуля для азота
    .npkPhosphorusOffset = 0.0f,  // Смещение нуля для фосфора
    .npkPotassiumOffset = 0.0f,   // Смещение нуля для калия
    .npkCalibrated = false,       // Флаг калибровки NPK
    
    // Температурная компенсация
    .temperatureCompensationEnabled = true, // Включена по умолчанию
    .temperatureReference = 25.0f,          // Референсная температура (25°C)
    
    // История калибровок
    .lastCalibrationTime = 0,     // Время последней калибровки
    .lastCalibratedBy = {0}       // Кто калибровал
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
    logDebugSafe("Калибровка: %s", factors.calibrationEnabled ? "включена" : "отключена");
    
    initialized = true;
}

bool SensorCorrection::isInitialized() {
    return initialized;
}

float SensorCorrection::correctHumidity(uint16_t rawValue) {
    if (!factors.enabled) {
        return rawValue / 10.0f; // Заводская калибровка
    }
    
    // Заводская калибровка: rawValue / 10.0 = %RH
    float factoryCal = rawValue / 10.0f;
    
    // Применяем калибровочную коррекцию если включена
    if (factors.calibrationEnabled && factors.humidityCalibrated) {
        factoryCal = (factoryCal * factors.humidityCalibrationSlope) + factors.humidityCalibrationOffset;
    }
    
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
    
    // Применяем калибровочную коррекцию если включена
    if (factors.calibrationEnabled && factors.ecCalibrated) {
        factoryCal = (factoryCal * factors.ecCalibrationSlope) + factors.ecCalibrationOffset;
    }
    
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
    
    // Применяем калибровочную коррекцию если включена
    if (factors.calibrationEnabled && factors.temperatureCalibrated) {
        factoryCal = (factoryCal * factors.temperatureCalibrationSlope) + factors.temperatureCalibrationOffset;
    }
    
    // Применяем коррекцию: y = mx + b
    float corrected = (factoryCal * factors.temperatureSlope) + factors.temperatureOffset;
    
    logDebugSafe("Коррекция температуры: %u → %.1f → %.1f°C", 
                 rawValue, factoryCal, corrected);
    
    return corrected;
}

// НОВЫЕ: Коррекция pH
float SensorCorrection::correctPH(uint16_t rawValue) {
    if (!factors.calibrationEnabled || !factors.phCalibrated) {
        return rawValue / 10.0f; // Заводская калибровка
    }
    
    // Заводская калибровка: rawValue / 10.0 = pH
    float factoryCal = rawValue / 10.0f;
    
    // Применяем калибровочную коррекцию: y = mx + b
    float calibrated = (factoryCal * factors.phSlope) + factors.phOffset;
    
    // Применяем температурную компенсацию если включена
    if (factors.temperatureCompensationEnabled) {
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
    if (!factors.calibrationEnabled || !factors.npkCalibrated) {
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
    nitrogen -= factors.npkNitrogenOffset;
    phosphorus -= factors.npkPhosphorusOffset;
    potassium -= factors.npkPotassiumOffset;
    
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
    float tempDiff = temperature - factors.temperatureReference;
    float compensation = -0.003f * tempDiff; // -0.003 pH/°C
    return value + compensation;
}

// НОВЫЕ: Получение текущей температуры для компенсации
float SensorCorrection::getCurrentTemperature() {
    // TODO: Получить текущую температуру из датчика
    // Пока возвращаем референсную температуру
    return factors.temperatureReference;
}

// НОВЫЕ: Включение/отключение калибровки
void SensorCorrection::enableCalibration(bool enabled) {
    factors.calibrationEnabled = enabled;
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
    
    // Вычисляем наклон и смещение методом наименьших квадратов
    float x1 = measured_4_01, y1 = expected_4_01;
    float x2 = measured_6_86, y2 = expected_6_86;
    float x3 = measured_9_18, y3 = expected_9_18;
    
    // Вычисляем наклон (slope)
    float numerator = (y1 + y2 + y3) * (x1 + x2 + x3) - 3 * (x1*y1 + x2*y2 + x3*y3);
    float denominator = (x1 + x2 + x3) * (x1 + x2 + x3) - 3 * (x1*x1 + x2*x2 + x3*x3);
    
    if (abs(denominator) < 0.001f) {
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
    float expected_1, float expected_2, float expected_3,
    float measured_1, float measured_2, float measured_3
) {
    CalibrationResult result = {false, 0.0f, 0.0f, 0.0f, ""};
    
    // Проверяем валидность данных (для 2-точечной калибровки третья точка может быть 0)
    if (measured_1 <= 0 || measured_2 <= 0) {
        return result;
    }
    
    // Если третья точка равна 0, используем только 2 точки
    bool use_two_points = (measured_3 <= 0);
    
    // Вычисляем наклон и смещение методом наименьших квадратов
    if (use_two_points) {
        // 2-точечная калибровка (линейная)
        float x1 = measured_1, y1 = expected_1;
        float x2 = measured_2, y2 = expected_2;
        
        // Вычисляем наклон (slope) для 2 точек
        if (abs(x2 - x1) < 0.001f) {
            return result; // Деление на ноль
        }
        
        result.slope = (y2 - y1) / (x2 - x1);
        result.offset = y1 - result.slope * x1;
        
        // Для 2 точек R² = 1.0 (идеальная прямая)
        result.r_squared = 1.0f;
    } else {
        // 3-точечная калибровка
        float x1 = measured_1, y1 = expected_1;
        float x2 = measured_2, y2 = expected_2;
        float x3 = measured_3, y3 = expected_3;
        
        // Вычисляем наклон (slope)
        float numerator = (y1 + y2 + y3) * (x1 + x2 + x3) - 3 * (x1*y1 + x2*y2 + x3*y3);
        float denominator = (x1 + x2 + x3) * (x1 + x2 + x3) - 3 * (x1*x1 + x2*x2 + x3*x3);
        
        if (abs(denominator) < 0.001f) {
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
    }
    
    // Проверяем качество калибровки
    if (result.r_squared >= 0.95f) {
        result.success = true;
    }
    
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
    float error = abs(result.offset) / referenceTemperature;
    result.r_squared = 1.0f - error;
    
    result.success = (abs(result.offset) < 2.0f); // Ошибка менее 2°C
    
    // Определяем качество
    if (abs(result.offset) < 0.5f) result.quality = "Отличное";
    else if (abs(result.offset) < 1.0f) result.quality = "Хорошее";
    else if (abs(result.offset) < 2.0f) result.quality = "Удовлетворительное";
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
    float error = abs(result.offset) / referenceHumidity;
    result.r_squared = 1.0f - error;
    
    result.success = (abs(result.offset) < 5.0f); // Ошибка менее 5%
    
    // Определяем качество
    if (abs(result.offset) < 1.0f) result.quality = "Отличное";
    else if (abs(result.offset) < 2.0f) result.quality = "Хорошее";
    else if (abs(result.offset) < 5.0f) result.quality = "Удовлетворительное";
    else result.quality = "Плохое";
    
    return result;
} 

// Добавляем недостающие функции
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
        // Существующие поля
        preferences.putFloat("hum_slope", factors.humiditySlope);
        preferences.putFloat("hum_offset", factors.humidityOffset);
        preferences.putFloat("ec_slope", factors.ecSlope);
        preferences.putFloat("ec_offset", factors.ecOffset);
        preferences.putFloat("temp_slope", factors.temperatureSlope);
        preferences.putFloat("temp_offset", factors.temperatureOffset);
        preferences.putBool("enabled", factors.enabled);
        
        // НОВЫЕ поля калибровки
        preferences.putBool("calibration_enabled", factors.calibrationEnabled);
        
        // pH калибровка
        preferences.putFloat("ph_slope", factors.phSlope);
        preferences.putFloat("ph_offset", factors.phOffset);
        preferences.putFloat("ph_quality", factors.phCalibrationQuality);
        preferences.putBool("ph_calibrated", factors.phCalibrated);
        
        // EC калибровка
        preferences.putFloat("ec_cal_slope", factors.ecCalibrationSlope);
        preferences.putFloat("ec_cal_offset", factors.ecCalibrationOffset);
        preferences.putFloat("ec_cal_quality", factors.ecCalibrationQuality);
        preferences.putBool("ec_calibrated", factors.ecCalibrated);
        
        // Температурная калибровка
        preferences.putFloat("temp_cal_slope", factors.temperatureCalibrationSlope);
        preferences.putFloat("temp_cal_offset", factors.temperatureCalibrationOffset);
        preferences.putBool("temp_calibrated", factors.temperatureCalibrated);
        
        // Влажностная калибровка
        preferences.putFloat("hum_cal_slope", factors.humidityCalibrationSlope);
        preferences.putFloat("hum_cal_offset", factors.humidityCalibrationOffset);
        preferences.putBool("hum_calibrated", factors.humidityCalibrated);
        
        // NPK калибровка
        preferences.putFloat("npk_n_offset", factors.npkNitrogenOffset);
        preferences.putFloat("npk_p_offset", factors.npkPhosphorusOffset);
        preferences.putFloat("npk_k_offset", factors.npkPotassiumOffset);
        preferences.putBool("npk_calibrated", factors.npkCalibrated);
        
        // Температурная компенсация
        preferences.putBool("temp_comp_enabled", factors.temperatureCompensationEnabled);
        preferences.putFloat("temp_reference", factors.temperatureReference);
        
        // История калибровок
        preferences.putULong("last_calibration_time", factors.lastCalibrationTime);
        preferences.putString("last_calibrated_by", String(factors.lastCalibratedBy));
        
        preferences.end();
        logDebugSafe("Коэффициенты коррекции и калибровки сохранены в EEPROM");
    }
}

void SensorCorrection::loadFactors() {
    Preferences preferences;
    if (preferences.begin("sensor_corr", true)) {
        // Существующие поля
        factors.humiditySlope = preferences.getFloat("hum_slope", 1.25f);
        factors.humidityOffset = preferences.getFloat("hum_offset", -5.0f);
        factors.ecSlope = preferences.getFloat("ec_slope", 1.35f);
        factors.ecOffset = preferences.getFloat("ec_offset", 0.0f);
        factors.temperatureSlope = preferences.getFloat("temp_slope", 1.0f);
        factors.temperatureOffset = preferences.getFloat("temp_offset", 0.0f);
        factors.enabled = preferences.getBool("enabled", true);
        
        // НОВЫЕ поля калибровки
        factors.calibrationEnabled = preferences.getBool("calibration_enabled", false);
        
        // pH калибровка
        factors.phSlope = preferences.getFloat("ph_slope", 1.0f);
        factors.phOffset = preferences.getFloat("ph_offset", 0.0f);
        factors.phCalibrationQuality = preferences.getFloat("ph_quality", 0.0f);
        factors.phCalibrated = preferences.getBool("ph_calibrated", false);
        
        // EC калибровка
        factors.ecCalibrationSlope = preferences.getFloat("ec_cal_slope", 1.0f);
        factors.ecCalibrationOffset = preferences.getFloat("ec_cal_offset", 0.0f);
        factors.ecCalibrationQuality = preferences.getFloat("ec_cal_quality", 0.0f);
        factors.ecCalibrated = preferences.getBool("ec_calibrated", false);
        
        // Температурная калибровка
        factors.temperatureCalibrationSlope = preferences.getFloat("temp_cal_slope", 1.0f);
        factors.temperatureCalibrationOffset = preferences.getFloat("temp_cal_offset", 0.0f);
        factors.temperatureCalibrated = preferences.getBool("temp_calibrated", false);
        
        // Влажностная калибровка
        factors.humidityCalibrationSlope = preferences.getFloat("hum_cal_slope", 1.0f);
        factors.humidityCalibrationOffset = preferences.getFloat("hum_cal_offset", 0.0f);
        factors.humidityCalibrated = preferences.getBool("hum_calibrated", false);
        
        // NPK калибровка
        factors.npkNitrogenOffset = preferences.getFloat("npk_n_offset", 0.0f);
        factors.npkPhosphorusOffset = preferences.getFloat("npk_p_offset", 0.0f);
        factors.npkPotassiumOffset = preferences.getFloat("npk_k_offset", 0.0f);
        factors.npkCalibrated = preferences.getBool("npk_calibrated", false);
        
        // Температурная компенсация
        factors.temperatureCompensationEnabled = preferences.getBool("temp_comp_enabled", true);
        factors.temperatureReference = preferences.getFloat("temp_reference", 25.0f);
        
        // История калибровок
        factors.lastCalibrationTime = preferences.getULong("last_calibration_time", 0);
        String lastCalibratedBy = preferences.getString("last_calibrated_by", "");
        strncpy(factors.lastCalibratedBy, lastCalibratedBy.c_str(), 31);
        factors.lastCalibratedBy[31] = '\0'; // Гарантируем null-termination
        
        preferences.end();
        logDebugSafe("Коэффициенты коррекции и калибровки загружены из EEPROM");
    }
} 