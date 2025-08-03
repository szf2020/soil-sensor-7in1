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
    float humiditySlope;     // Коэффициент наклона для влажности
    float humidityOffset;    // Смещение для влажности (%)
    float ecSlope;          // Коэффициент наклона для EC
    float ecOffset;         // Смещение для EC (μS/cm)
    float temperatureSlope; // Коэффициент наклона для температуры
    float temperatureOffset; // Смещение для температуры (°C)
    bool enabled;           // Включена ли коррекция
};

// Функции коррекции показаний
class SensorCorrection {
public:
    // Инициализация с заводскими коэффициентами
    static void init();
    
    // Коррекция влажности (регистр 0x0012)
    static float correctHumidity(uint16_t rawValue);
    
    // Коррекция EC (регистр 0x0015)
    static float correctEC(uint16_t rawValue);
    
    // Коррекция температуры (регистр 0x0013)
    static float correctTemperature(uint16_t rawValue);
    
    // Установка коэффициентов коррекции
    static void setCorrectionFactors(const CorrectionFactors& factors);
    
    // Получение текущих коэффициентов
    static CorrectionFactors getCorrectionFactors();
    
    // Включение/отключение коррекции
    static void enableCorrection(bool enabled);
    
    // Сохранение коэффициентов в EEPROM
    static void saveFactors();
    
    // Загрузка коэффициентов из EEPROM
    static void loadFactors();

private:
    static CorrectionFactors factors;
    static bool initialized;
};

// Глобальный экземпляр для удобства
extern SensorCorrection gSensorCorrection;

#endif // SENSOR_CORRECTION_H 