/**
 * @file INutrientInteractionService.h
 * @brief Интерфейс сервиса взаимодействия питательных веществ
 * @details Учитывает антагонизм и синергизм между питательными элементами
 */

#ifndef INUTRIENT_INTERACTION_SERVICE_H
#define INUTRIENT_INTERACTION_SERVICE_H

#include <Arduino.h>
#include "../sensor_types.h"

/**
 * @brief Структура для хранения взаимодействий питательных веществ
 */
struct NutrientInteraction
{
    float antagonism_factor;    // Фактор антагонизма (0.0 - 1.0)
    float synergy_factor;       // Фактор синергизма (1.0+)
    const char* mechanism;      // Механизм взаимодействия
    const char* source;         // Научный источник
};

/**
 * @brief Интерфейс сервиса взаимодействия питательных веществ
 */
class INutrientInteractionService
{
public:
    virtual ~INutrientInteractionService() = default;

    /**
     * @brief Применяет корректировки взаимодействий к NPK данным
     * 
     * @param npk Исходные NPK данные
     * @param soilType Тип почвы
     * @param pH Значение pH почвы
     * @return NPKReferences Скорректированные NPK данные
     */
    virtual NPKReferences applyNutrientInteractions(const NPKReferences& npk, 
                                                   SoilType soilType, 
                                                   float pH) = 0;

    /**
     * @brief Получает фактор антагонизма между двумя элементами
     * 
     * @param element1 Первый элемент
     * @param element2 Второй элемент
     * @param ratio1 Соотношение первого элемента
     * @param ratio2 Соотношение второго элемента
     * @return float Фактор антагонизма
     */
    virtual float getAntagonismFactor(const String& element1, 
                                     const String& element2,
                                     float ratio1, 
                                     float ratio2) = 0;

    /**
     * @brief Получает фактор синергизма между двумя элементами
     * 
     * @param element1 Первый элемент
     * @param element2 Второй элемент
     * @param ratio1 Соотношение первого элемента
     * @param ratio2 Соотношение второго элемента
     * @return float Фактор синергизма
     */
    virtual float getSynergyFactor(const String& element1, 
                                  const String& element2,
                                  float ratio1, 
                                  float ratio2) = 0;

    /**
     * @brief Генерирует рекомендации по устранению антагонизмов
     * 
     * @param npk NPK данные
     * @param soilType Тип почвы
     * @param pH Значение pH
     * @return String Рекомендации
     */
    virtual String generateAntagonismRecommendations(const NPKReferences& npk,
                                                    SoilType soilType,
                                                    float pH) = 0;
};

#endif // INUTRIENT_INTERACTION_SERVICE_H 