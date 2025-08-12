/**
 * @file crop_recommendation_engine.h
 * @brief Реализация движка рекомендаций по культурам
 * @details Вычисляет агрономические рекомендации на основе типа культуры и условий
 */

#ifndef CROP_RECOMMENDATION_ENGINE_H
#define CROP_RECOMMENDATION_ENGINE_H

#include <Arduino.h>
#include <map>
#include <vector>
#include "business/ICropRecommendationEngine.h"

// Используем структуру SensorData из modbus_sensor.h
#include "../modbus_sensor.h"

// Для NPKReferences и SoilType
#include "../../include/sensor_types.h"

// Структуры для устранения проблемы с легко перепутываемыми параметрами (удалены - теперь в sensor_types.h)
// CropCompensationParams и CropECCompensationParams определены в sensor_types.h

// CropConfig определена в ICropRecommendationEngine.h

// Структура для процентов коррекции
struct CorrectionPercentages
{
    float temperature;
    float humidity;
    float ec;
    float ph;
    float nitrogen;
    float phosphorus;
    float potassium;
};

// Структура для цветовых индикаторов
struct ColorIndicators
{
    String temperature;
    String humidity;
    String ec;
    String ph;
    String nitrogen;
    String phosphorus;
    String potassium;
};

// Структура результата рекомендаций
struct RecommendationResult
{
    String cropType;
    String growingType;
    String season;
    String soilType;  // Добавляем тип почвы
    String recommendations;
    String healthStatus;
    String scientificNotes;
    
    // Новые поля для системного алгоритма
    CropConfig tableValues;              // Исходные табличные значения
    CropConfig growingTypeAdjusted;      // После коррекции типа выращивания
    CropConfig soilTypeAdjusted;         // После коррекции типа почвы (НОВОЕ!)
    CropConfig finalCalculated;          // Итоговые расчетные значения
    CropConfig scientificallyCompensated; // Научно компенсированные значения
    CorrectionPercentages correctionPercentages;
    ColorIndicators colorIndicators;
};

class CropRecommendationEngine : public ICropRecommendationEngine
{
   private:
    std::map<String, CropConfig> cropConfigs;

    // Коэффициенты компенсации датчиков [Источник: SSSA Journal, 2008; Advances in Agronomy, 2014]
    const float pH_alpha = -0.01F;    // Температурный коэффициент для pH
    const float pH_beta = 0.005F;     // Влажностный коэффициент для pH
    const float EC_gamma = 0.02F;     // Температурный коэффициент для EC
    const float NPK_delta = 0.03F;    // Температурный коэффициент для NPK
    const float NPK_epsilon = 0.01F;  // Влажностный коэффициент для NPK

    // УДАЛЕНО: Функции компенсации датчиков
    // Используется SensorCompensationService для единообразной компенсации

    void initializeCropConfigs();
    // ❌ УДАЛЕНО: Старые функции корректировок - заменены на системный алгоритм
    // ❌ УДАЛЕНО: applySeasonalAdjustments, applyGrowingTypeAdjustments, applySoilTypeAdjustments
    String generateScientificRecommendations(const SensorData& data, const CropConfig& config, const String& cropType,
                                             const String& soilType);
    String calculateSoilHealthStatus(const SensorData& data, const CropConfig& config);
    String generateScientificNotes(const SensorData& data, const CropConfig& config, const String& cropType,
                                   const String& soilType);
    
    // Новые методы для системного алгоритма
    CropConfig getTableValues(const String& cropType) const;
    CropConfig applyGrowingTypeCorrection(const CropConfig& table, const String& growingType);
    CropConfig applySoilTypeCorrection(const CropConfig& adjusted, const String& soilType);
    CropConfig applySeasonalCorrection(const CropConfig& adjusted, const String& season);
    CropConfig getScientificallyCompensated(const SensorData& data, const String& cropType);
    CorrectionPercentages calculateCorrectionPercentages(const CropConfig& table, const CropConfig& final);
    ColorIndicators calculateColorIndicators(const CropConfig& final, const CropConfig& scientific);

   public:
    CropRecommendationEngine();

    // Основной метод генерации рекомендаций
    RecommendationResult generateRecommendation(const SensorData& data, const String& cropType,
                                                const String& growingType = "outdoor", const String& season = "summer") override;

    // Получение списка доступных культур
    std::vector<String> getAvailableCrops() const;

    // Получение конфигурации культуры
    CropConfig getCropConfig(const String& cropType) const override;

    // Валидация данных с датчиков
    bool validateSensorData(const SensorData& data) const;

    // Получение научных данных о культуре
    String getCropScientificInfo(const String& cropType) const;

    // Реализация интерфейса ICropRecommendationEngine
    RecValues computeRecommendations(const String& cropId, const SoilProfile& soilProfile,
                                     const EnvironmentType& envType) override;

    void applySeasonalCorrection(RecValues& rec, Season season, bool isGreenhouse) override;

    // 🌱 Специфические рекомендации по культурам для неизмеряемых элементов
    String generateCropSpecificRecommendations(const String& cropName, 
                                             const NPKReferences& npk,
                                             SoilType soilType, 
                                             float pH,
                                             const String& season = "none") override;
};

#endif  // CROP_RECOMMENDATION_ENGINE_H
