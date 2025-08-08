/**
 * @file sensor_correction.h
 * @brief Система коррекции показаний датчика
 *
 * Применяется программная коррекция показаний на основе экспериментальных данных
 * для улучшения точности измерений в грунте.
 */

#ifndef SENSOR_CORRECTION_H
#define SENSOR_CORRECTION_H

#include <Arduino.h>

// Структура для хранения коэффициентов коррекции
struct CorrectionFactors {
    // Существующие поля коррекции (НЕ ТРОГАЕМ)
    float humiditySlope;     // Коэффициент наклона для влажности
    float humidityOffset;    // Смещение для влажности (%)
    float ecSlope;          // Коэффициент наклона для EC
    float ecOffset;         // Смещение для EC (μS/cm)
    float temperatureSlope; // Коэффициент наклона для температуры
    float temperatureOffset; // Смещение для температуры (°C)
    bool enabled;           // Включена ли коррекция
    
    // НОВЫЕ поля для калибровки
    bool calibrationEnabled;     // Включена ли калибровка
    
    // pH калибровка
    float phSlope;              // Коэффициент pH калибровки
    float phOffset;             // Смещение pH калибровки
    float phCalibrationQuality; // Качество калибровки pH (R²)
    bool phCalibrated;          // Флаг калибровки pH
    
    // EC калибровка
    float ecCalibrationSlope;   // Коэффициент EC калибровки (отдельный от коррекции)
    float ecCalibrationOffset;  // Смещение EC калибровки
    float ecCalibrationQuality; // Качество калибровки EC (R²)
    bool ecCalibrated;          // Флаг калибровки EC
    
    // Температурная калибровка
    float temperatureCalibrationSlope;   // Коэффициент температурной калибровки
    float temperatureCalibrationOffset;  // Смещение температурной калибровки
    bool temperatureCalibrated;          // Флаг калибровки температуры
    
    // Влажность калибровка
    float humidityCalibrationSlope;      // Коэффициент влажностной калибровки
    float humidityCalibrationOffset;     // Смещение влажностной калибровки
    bool humidityCalibrated;             // Флаг калибровки влажности
    
    // NPK калибровка (нулевая точка)
    float npkNitrogenOffset;    // Смещение нуля для азота
    float npkPhosphorusOffset;  // Смещение нуля для фосфора
    float npkPotassiumOffset;   // Смещение нуля для калия
    bool npkCalibrated;         // Флаг калибровки NPK
    
    // Температурная компенсация
    bool temperatureCompensationEnabled; // Включена ли температурная компенсация
    float temperatureReference;          // Референсная температура (25°C)
    
    // История калибровок
    unsigned long lastCalibrationTime;   // Время последней калибровки
    char lastCalibratedBy[32];           // Кто калибровал
};

// Структура для результатов калибровки
struct CalibrationResult {
    bool success;
    float slope;
    float offset;
    float r_squared;
    String quality;
};

// Функции коррекции показаний
class SensorCorrection {
public:
    // Инициализация с заводскими коэффициентами
    static void init();
    
    // Проверка инициализации
    static bool isInitialized();
    
    // Коррекция влажности (регистр 0x0012)
    static float correctHumidity(uint16_t rawValue);
    
    // Коррекция EC (регистр 0x0015)
    static float correctEC(uint16_t rawValue);
    
    // Коррекция температуры (регистр 0x0013)
    static float correctTemperature(uint16_t rawValue);
    
    // НОВЫЕ: Коррекция pH (регистр 0x0014)
    static float correctPH(uint16_t rawValue);
    
    // НОВЫЕ: Коррекция NPK (нулевая точка)
    static void correctNPK(uint16_t rawN, uint16_t rawP, uint16_t rawK, 
                          float& nitrogen, float& phosphorus, float& potassium);
    
    // НОВЫЕ: Температурная компенсация pH
    static float applyTemperatureCompensation(float value, float temperature);
    
    // Установка коэффициентов коррекции
    static void setCorrectionFactors(const CorrectionFactors& factors);
    
    // Получение текущих коэффициентов
    static CorrectionFactors getCorrectionFactors();
    
    // Включение/отключение коррекции
    static void enableCorrection(bool enabled);
    
    // НОВЫЕ: Включение/отключение калибровки
    static void enableCalibration(bool enabled);
    
    // Сохранение коэффициентов в EEPROM
    static void saveFactors();
    
    // Загрузка коэффициентов из EEPROM
    static void loadFactors();
    
    // НОВЫЕ: Функции калибровки
    static CalibrationResult calculatePHCalibration(
        float expected_4_01, float expected_6_86, float expected_9_18,
        float measured_4_01, float measured_6_86, float measured_9_18
    );
    
    // ДВУХТОЧЕЧНАЯ EC-калибровка (линейная)
    static CalibrationResult calculateECCalibration(
        float expected_1, float expected_2,
        float measured_1, float measured_2
    );
    
    static CalibrationResult calculateTemperatureCalibration(
        float referenceTemperature, float measuredTemperature
    );
    
    static CalibrationResult calculateHumidityCalibration(
        float referenceHumidity, float measuredHumidity
    );

private:
    static CorrectionFactors factors;
    static bool initialized;
    
    // НОВЫЕ: Получение текущей температуры для компенсации
    static float getCurrentTemperature();
};

// Глобальный экземпляр для удобства
extern SensorCorrection gSensorCorrection;

#endif // SENSOR_CORRECTION_H 