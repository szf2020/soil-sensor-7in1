/**
 * @file nutrient_interaction_service.h
 * @brief Реализация сервиса взаимодействия питательных веществ
 * @details Учитывает антагонизм и синергизм между питательными элементами
 */

#ifndef NUTRIENT_INTERACTION_SERVICE_H
#define NUTRIENT_INTERACTION_SERVICE_H

#include "../../include/business/INutrientInteractionService.h"
#include <map>

/**
 * @brief Реализация сервиса взаимодействия питательных веществ
 * 
 * Учитывает научно обоснованные взаимодействия между питательными элементами:
 * - Антагонизмы: N vs K, K vs Mg, P vs Zn, P vs Ca (pH-зависимый)
 * - Синергизмы: N + S, Ca + B
 * 
 * Источники:
 * - Marschner, 2012. Mineral Nutrition of Higher Plants
 * - White & Brown, 2010. Plant Nutrition for Sustainable Development
 */
class NutrientInteractionService : public INutrientInteractionService
{
private:
    // Карта коэффициентов взаимодействий
    std::map<String, NutrientInteraction> interactionCoefficients;
    
    // Инициализация коэффициентов взаимодействий
    void initializeInteractionCoefficients();

public:
    /**
     * @brief Конструктор
     */
    NutrientInteractionService();
    
    /**
     * @brief Деструктор
     */
    virtual ~NutrientInteractionService();
    
    // Реализация методов интерфейса
    NPKReferences applyNutrientInteractions(const NPKReferences& npk, 
                                          SoilType soilType, 
                                          float pH) override;
    
    float getAntagonismFactor(const String& element1, 
                             const String& element2,
                             float ratio1, 
                             float ratio2) override;
    
    float getSynergyFactor(const String& element1, 
                          const String& element2,
                          float ratio1, 
                          float ratio2) override;
    
    String generateAntagonismRecommendations(const NPKReferences& npk,
                                           SoilType soilType,
                                           float pH) override;
};

#endif // NUTRIENT_INTERACTION_SERVICE_H 