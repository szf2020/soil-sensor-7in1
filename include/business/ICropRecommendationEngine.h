/**
 * @file ICropRecommendationEngine.h
 * @brief Интерфейс для движка рекомендаций по культурам
 * @details Определяет контракт для вычисления агрономических рекомендаций
 */

#ifndef ICROP_RECOMMENDATION_ENGINE_H
#define ICROP_RECOMMENDATION_ENGINE_H

#include <Arduino.h>
#include "../sensor_types.h"  // Для SoilProfile

// Структура для рекомендаций
struct RecValues
{
    float t;    // Температура
    float hum;  // Влажность
    float ec;   // Электропроводность
    float ph;   // pH
    float n;    // Азот
    float p;    // Фосфор
    float k;    // Калий
};

// Перечисления для типов (удалены - теперь в sensor_types.h)
// Season и EnvironmentType определены в sensor_types.h

// SoilProfile определен в sensor_types.h

/**
 * @brief Интерфейс для движка рекомендаций по культурам
 *
 * Определяет контракт для вычисления агрономических рекомендаций
 * на основе типа культуры, профиля почвы и условий окружающей среды.
 */
class ICropRecommendationEngine
{
   public:
    virtual ~ICropRecommendationEngine() = default;

    /**
     * @brief Вычисляет рекомендации для указанной культуры
     *
     * @param cropId Идентификатор культуры (tomato, cucumber, etc.)
     * @param soilProfile Профиль почвы (SAND, LOAM, CLAY, PEAT, SANDPEAT)
     * @param envType Тип окружающей среды (OUTDOOR, GREENHOUSE, INDOOR)
     * @return RecValues Структура с рекомендуемыми значениями
     */
    virtual RecValues computeRecommendations(const String& cropId, const SoilProfile& soilProfile,
                                             const EnvironmentType& envType) = 0;

    /**
     * @brief Применяет сезонные корректировки к рекомендациям
     *
     * @param rec Рекомендации для корректировки
     * @param season Текущий сезон
     * @param isGreenhouse Признак теплицы
     */
    virtual void applySeasonalCorrection(RecValues& rec, Season season, bool isGreenhouse) = 0;

    /**
     * @brief Генерирует специфические рекомендации по культурам для неизмеряемых элементов
     *
     * @param cropName Название культуры
     * @param npk NPK данные
     * @param soilType Тип почвы
     * @param pH Значение pH
     * @param season Текущий сезон для корректировки порогов
     * @return String Рекомендации
     */
    virtual String generateCropSpecificRecommendations(const String& cropName,
                                                      const NPKReferences& npk,
                                                      SoilType soilType,
                                                      float pH,
                                                      const String& season = "none") = 0;
};

#endif  // ICROP_RECOMMENDATION_ENGINE_H
