#pragma once

#include <Arduino.h>
#include "sensor_types.h"

// Структуры для валидации
struct CropParameters
{
    float optimal_temperature;
    float optimal_humidity;
    float optimal_ec;
    float optimal_ph;
    float optimal_nitrogen;
    float optimal_phosphorus;
    float optimal_potassium;
};

struct EnvironmentAdjustment
{
    float temperature_offset;
    float humidity_offset;
    float ec_offset;
    float nitrogen_factor;
    float phosphorus_factor;
    float potassium_factor;
};

// Интерфейс сервиса научной валидации
class IScientificValidationService
{
public:
    virtual ~IScientificValidationService() = default;
    
    // Валидация формул компенсации
    virtual bool validateCompensationFormula(const char* formula_type, const char* formula, const char* source) const = 0;
    
    // Валидация коэффициентов почв
    virtual bool validateSoilCoefficients(SoilType soil_type, float ec_coeff, float ph_buffer, float water_holding) const = 0;
    
    // Валидация рекомендаций по культурам
    virtual bool validateCropRecommendation(const char* crop_name, const CropParameters& params) const = 0;
    
    // Валидация сезонных корректировок
    virtual bool validateSeasonalAdjustment(const char* season, float nitrogen_factor, float phosphorus_factor, float potassium_factor) const = 0;
    
    // Валидация корректировок среды выращивания
    virtual bool validateEnvironmentAdjustment(const char* environment, const EnvironmentAdjustment& adjustment) const = 0;
    
    // Генерация отчета валидации
    virtual String generateValidationReport() const = 0;
    
    // Проверка статуса источника
    virtual bool isSourceVerified(const char* source_title) const = 0;
    
    // Получение DOI источника
    virtual const char* getSourceDOI(const char* source_title) const = 0;
    
    // Получение журнала источника
    virtual const char* getSourceJournal(const char* source_title) const = 0;
}; 