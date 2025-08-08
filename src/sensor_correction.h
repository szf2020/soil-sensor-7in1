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
    unsigned long lastCalibrationTime; // Время последней калибровки
    char lastCalibratedBy[50];         // Кто калибровал (фиксированный размер для экономии памяти)
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
    // Конструктор
    SensorCorrection();
    
    // Инициализация с заводскими коэффициентами
    void init();
    
    // Проверка инициализации
    bool isInitialized();
    
    // Коррекция влажности (регистр 0x0012)
    float correctHumidity(uint16_t rawValue);
    
    // Коррекция EC (регистр 0x0015)
    float correctEC(uint16_t rawValue);
    
    // Коррекция температуры (регистр 0x0013)
    float correctTemperature(uint16_t rawValue);
    
    // НОВЫЕ: Коррекция pH (регистр 0x0014)
    float correctPH(uint16_t rawValue);
    
    // НОВЫЕ: Коррекция NPK (нулевая точка)
    void correctNPK(uint16_t rawN, uint16_t rawP, uint16_t rawK, 
                          float& nitrogen, float& phosphorus, float& potassium);
    
    // НОВЫЕ: Температурная компенсация pH
    float applyTemperatureCompensation(float value, float temperature);
    
    // Установка коэффициентов коррекции
    void setCorrectionFactors(const CorrectionFactors& factors);
    
    // Получение текущих коэффициентов
    CorrectionFactors getCorrectionFactors();
    
    // Включение/отключение коррекции
    void enableCorrection(bool enabled);
    
    // НОВЫЕ: Включение/отключение калибровки
    void enableCalibration(bool enabled);
    
    // Сохранение коэффициентов в EEPROM
    void saveFactors();
    
    // Загрузка коэффициентов из EEPROM
    void loadFactors();
    
    // НОВЫЕ: Функции калибровки
    CalibrationResult calculatePHCalibration(
        float expected_4_01, float expected_6_86, float expected_9_18,
        float measured_4_01, float measured_6_86, float measured_9_18
    );
    
    // ДВУХТОЧЕЧНАЯ EC-калибровка (линейная)
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

private:
    CorrectionFactors factors;
    bool initialized;
    
    // НОВЫЕ: Получение текущей температуры для компенсации
    float getCurrentTemperature();
};

// Глобальный экземпляр для удобства
extern SensorCorrection gSensorCorrection;

#endif // SENSOR_CORRECTION_H 