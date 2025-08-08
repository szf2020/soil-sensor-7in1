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
#include <string>

// Константа для размера буфера lastCalibratedBy
static constexpr size_t CalibratedByMaxLen = 50;

// Структура для хранения коэффициентов коррекции
struct CorrectionFactors {
    // Существующие поля коррекции
    float humiditySlope;      // Коэффициент для влажности
    float humidityOffset;     // Смещение для влажности
    float ecSlope;           // Коэффициент для EC
    float ecOffset;          // Смещение для EC
    float temperatureSlope;   // Коэффициент для температуры
    float temperatureOffset;  // Смещение для температуры
    bool enabled;            // Включена ли коррекция
    
    // НОВЫЕ поля для калибровки
    bool calibrationEnabled;     // Включена ли калибровка
    
    // pH калибровка
    float phSlope;              // Коэффициент pH калибровки
    float phOffset;             // Смещение pH калибровки
    float phCalibrationQuality; // Качество калибровки pH (R²)
    bool phCalibrated;          // Флаг калибровки pH
    
    // EC калибровка
    float ecCalibrationSlope;   // Коэффициент EC калибровки
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
    char lastCalibratedBy[CalibratedByMaxLen]; // Кто калибровал (фиксированный размер для экономии памяти)
};

// Структура для результата калибровки
struct CalibrationResult {
    bool success;           // Успешна ли калибровка
    float slope;           // Наклон (коэффициент)
    float offset;          // Смещение
    float r_squared;       // Коэффициент корреляции (R²)
    const char* quality;   // Качество калибровки
};

// Класс для коррекции показаний датчика
class SensorCorrection {
private:
    CorrectionFactors factors;  // Коэффициенты коррекции
    bool initialized;           // Флаг инициализации
    
    // Кэширование температуры для оптимизации производительности
    mutable float cachedTemperature;     // Кэшированное значение температуры
    mutable unsigned long lastTempRead;  // Время последнего чтения температуры
    static constexpr unsigned long TEMP_CACHE_DURATION = 1000; // 1 секунда кэша

public:
    // Конструктор
    SensorCorrection();
    
    // Инициализация
    void init();
    bool isInitialized() const;
    
    // Коррекция показаний
    float correctHumidity(uint16_t rawValue);
    float correctEC(uint16_t rawValue);
    float correctTemperature(uint16_t rawValue);
    float correctPH(uint16_t rawValue);
    void correctNPK(uint16_t rawN, uint16_t rawP, uint16_t rawK, 
                   float& nitrogen, float& phosphorus, float& potassium);
    
    // Температурная компенсация
    float applyTemperatureCompensation(float value, float temperature) const;
    float getCurrentTemperature() const;
    
    // Управление калибровкой
    void enableCalibration(bool enabled);
    void enableCorrection(bool enabled);
    
    // Функции калибровки
    CalibrationResult calculatePHCalibration(
        float expected_4_01, float expected_6_86, float expected_9_18,
        float measured_4_01, float measured_6_86, float measured_9_18
    );
    CalibrationResult calculateECCalibration(
        float expected_1, float expected_2,
        float measured_1, float measured_2
    );
    CalibrationResult calculateTemperatureCalibration(
        float referenceTemperature, float measuredTemperature
    );
    CalibrationResult calculateHumidityCalibration(
        float referenceHumidity, float measuredHumidity
    );
    
    // Управление коэффициентами
    void setCorrectionFactors(const CorrectionFactors& newFactors);
    const CorrectionFactors& getCorrectionFactors() const;
    
    // Сохранение/загрузка
    void saveFactors();
    void loadFactors();
};

// Глобальный экземпляр
extern SensorCorrection gSensorCorrection;

#endif // SENSOR_CORRECTION_H 