/**
 * @file sensor_types.h
 * @brief Типы данных для датчиков почвы
 * @details Определения структур и перечислений для работы с данными датчиков
 */

#ifndef SENSOR_TYPES_H
#define SENSOR_TYPES_H

#include <Arduino.h>

/**
 * @brief Типы почвы
 */
enum class SoilType : uint8_t
{
    SAND = 0,      // Песок
    LOAM = 1,      // Суглинок
    PEAT = 2,      // Торф
    CLAY = 3,      // Глина
    SANDPEAT = 4   // Песчано-торфяная смесь
};

/**
 * @brief Профили почвы
 */
enum class SoilProfile : uint8_t
{
    SAND = 0,      // Песок
    LOAM = 1,      // Суглинок
    PEAT = 2,      // Торф
    CLAY = 3,      // Глина
    SANDPEAT = 4   // Песчано-торфяная смесь
};

/**
 * @brief Типы среды выращивания
 */
enum class EnvironmentType : uint8_t
{
    OUTDOOR = 0,   // Открытый грунт
    GREENHOUSE = 1, // Теплица
    INDOOR = 2     // Закрытое помещение
};

/**
 * @brief Сезоны
 */
enum class Season : uint8_t
{
    SPRING = 0,    // Весна
    SUMMER = 1,    // Лето
    AUTUMN = 2,    // Осень
    WINTER = 3     // Зима
};

/**
 * @brief Данные датчика
 */
struct SensorData
{
    float temperature;  // Температура (°C)
    float humidity;     // Влажность (%)
    float ec;          // Электропроводность (µS/cm)
    float ph;          // pH
    float nitrogen;    // Азот (mg/kg)
    float phosphorus;  // Фосфор (mg/kg)
    float potassium;   // Калий (mg/kg)
    
    SensorData() : temperature(0.0F), humidity(0.0F), ec(0.0F), ph(0.0F), 
                   nitrogen(0.0F), phosphorus(0.0F), potassium(0.0F) {}
    
    SensorData(float temp, float hum, float ec_val, float ph_val, 
               float n, float p, float k) 
        : temperature(temp), humidity(hum), ec(ec_val), ph(ph_val), 
          nitrogen(n), phosphorus(p), potassium(k) {}
};

/**
 * @brief NPK значения
 */
struct NPKReferences
{
    float nitrogen;    // Азот (mg/kg)
    float phosphorus;  // Фосфор (mg/kg)
    float potassium;   // Калий (mg/kg)
    
    NPKReferences() : nitrogen(0.0F), phosphorus(0.0F), potassium(0.0F) {}
    
    NPKReferences(float n, float p, float k) : nitrogen(n), phosphorus(p), potassium(k) {}
};

/**
 * @brief Параметры компенсации EC
 */
struct ECCompensationParams
{
    float rawValue;
    float temperature;
    float compensationFactor;
    
    ECCompensationParams() : rawValue(0.0F), temperature(25.0F), compensationFactor(1.0F) {}
    
    ECCompensationParams(float raw, float temp, float factor) 
        : rawValue(raw), temperature(temp), compensationFactor(factor) {}
    
    // Builder pattern для предотвращения ошибок
    struct Builder
    {
        float raw = 0.0F;
        float temp = 25.0F;
        float factor = 1.0F;
        
        Builder& setRawValue(float value) { raw = value; return *this; }
        Builder& setTemperature(float value) { temp = value; return *this; }
        Builder& setCompensationFactor(float value) { factor = value; return *this; }
        
        ECCompensationParams build() const { return ECCompensationParams(raw, temp, factor); }
    };
    
    static Builder builder() { return Builder(); }
};

/**
 * @brief Параметры компенсации для культур
 */
struct CropCompensationParams
{
    float rawValue;
    float temperature;
    float moisture;
    
    CropCompensationParams() : rawValue(0.0F), temperature(25.0F), moisture(60.0F) {}
    
    CropCompensationParams(float raw, float temp, float moist) 
        : rawValue(raw), temperature(temp), moisture(moist) {}
};

/**
 * @brief Параметры компенсации EC для культур
 */
struct CropECCompensationParams
{
    float rawValue;
    float temperature;
    
    CropECCompensationParams() : rawValue(0.0F), temperature(25.0F) {}
    
    CropECCompensationParams(float raw, float temp) : rawValue(raw), temperature(temp) {}
};

#endif // SENSOR_TYPES_H 