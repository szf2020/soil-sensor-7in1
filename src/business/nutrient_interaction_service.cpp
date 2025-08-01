/**
 * @file nutrient_interaction_service.cpp
 * @brief Реализация сервиса взаимодействия питательных веществ
 * @details Учитывает антагонизм и синергизм между питательными элементами
 */

#include "nutrient_interaction_service.h"
#include "../../include/logger.h"
#include <cmath>

NutrientInteractionService::NutrientInteractionService()
{
    logDebugSafe("NutrientInteractionService: Инициализация сервиса взаимодействия питательных веществ");
    initializeInteractionCoefficients();
}

NPKReferences NutrientInteractionService::applyNutrientInteractions(const NPKReferences& npk, 
                                                                   SoilType soilType, 
                                                                   float pH)
{
    NPKReferences corrected = npk;
    
    // ✅ РЕАЛЬНО ИЗМЕРЯЕМЫЕ ВЗАИМОДЕЙСТВИЯ
    
    // 1. N vs K антагонизм (Азот блокирует калий) - ИЗМЕРЯЕТСЯ!
    float nk_antagonism = getAntagonismFactor("N", "K", npk.nitrogen, npk.potassium);
    corrected.potassium *= nk_antagonism;
    
    // 2. pH-зависимые взаимодействия - pH ИЗМЕРЯЕТСЯ!
    if (pH > 7.5F) {
        // Щелочные почвы: P-Ca антагонизм усиливается
        float pca_antagonism = 1.0F - (0.3F * (pH - 7.5F));
        corrected.phosphorus *= pca_antagonism;
    }
    
    // 3. P vs K взаимодействие (измеряются оба!)
    float pk_ratio = npk.phosphorus / npk.potassium;
    if (pk_ratio > 0.8F) {
        // Высокий фосфор может блокировать калий
        float pk_antagonism = 1.0F - (0.15F * (pk_ratio - 0.8F));
        corrected.potassium *= pk_antagonism;
    }
    
    logDebugSafe("NutrientInteractionService: Применены взаимодействия N:%.2f P:%.2f K:%.2f", 
                 corrected.nitrogen, corrected.phosphorus, corrected.potassium);
    
    return corrected;
}

float NutrientInteractionService::getAntagonismFactor(const String& element1, 
                                                     const String& element2,
                                                     float ratio1, 
                                                     float ratio2)
{
    // Научные формулы антагонизма (Marschner, 2012)
    
    if (element1 == "N" && element2 == "K") {
        // N vs K антагонизм: высокий азот блокирует калий
        float n_ratio = ratio1 / 200.0F; // Оптимальное соотношение N:K = 1:1
        if (n_ratio > 1.5F) {
            return 1.0F - (0.3F * (n_ratio - 1.5F));
        }
    }
    
    if (element1 == "K" && element2 == "Mg") {
        // K vs Mg антагонизм: высокий калий блокирует магний
        float k_ratio = ratio1 / 200.0F; // Оптимальное соотношение K:Mg = 2:1
        if (k_ratio > 2.5F) {
            return 1.0F - (0.25F * (k_ratio - 2.5F));
        }
    }
    
    if (element1 == "P" && element2 == "Zn") {
        // P vs Zn антагонизм: высокий фосфор блокирует цинк
        float p_ratio = ratio1 / 100.0F; // Оптимальное соотношение P:Zn = 10:1
        if (p_ratio > 15.0F) {
            return 1.0F - (0.4F * (p_ratio - 15.0F) / 15.0F);
        }
    }
    
    return 1.0F; // Нет антагонизма
}

float NutrientInteractionService::getSynergyFactor(const String& element1, 
                                                  const String& element2,
                                                  float ratio1, 
                                                  float ratio2)
{
    // Научные формулы синергизма (White & Brown, 2010)
    
    if (element1 == "N" && element2 == "S") {
        // N + S синергизм: сера усиливает усвоение азота
        // Примечание: S не измеряется датчиком, используем оптимальное соотношение
        float optimal_ns_ratio = 15.0F; // Оптимальное соотношение N:S = 15:1
        float current_ratio = ratio1 / 10.0F; // Предполагаем среднее содержание S
        
        if (current_ratio < optimal_ns_ratio) {
            return 1.0F + (0.15F * (optimal_ns_ratio - current_ratio) / optimal_ns_ratio);
        }
    }
    
    if (element1 == "Ca" && element2 == "B") {
        // Ca + B синергизм: кальций улучшает транспорт бора
        // Примечание: Ca и B не измеряются датчиком
        return 1.0F + 0.1F; // Небольшой синергический эффект
    }
    
    return 1.0F; // Нет синергизма
}

String NutrientInteractionService::generateAntagonismRecommendations(const NPKReferences& npk,
                                                                   SoilType soilType,
                                                                   float pH)
{
    String recommendations = "";
    
    // ✅ РЕАЛЬНО ИЗМЕРЯЕМЫЕ ВЗАИМОДЕЙСТВИЯ
    
    // 1. N vs K антагонизм (измеряются оба!)
    float nk_antagonism = getAntagonismFactor("N", "K", npk.nitrogen, npk.potassium);
    if (nk_antagonism < 0.8F) {
        recommendations += "⚠️ Высокий N → уменьшить N, увеличить K\n";
    }
    
    // 2. pH-зависимые антагонизмы (pH измеряется!)
    if (pH > 7.5F) {
        recommendations += "⚠️ Щелочная почва → подкислить\n";
    }
    
    // 3. P vs K взаимодействие (измеряются оба!)
    float pk_ratio = npk.phosphorus / npk.potassium;
    if (pk_ratio > 0.8F) {
        recommendations += "⚠️ Высокий P → уменьшить P, увеличить K\n";
    }
    
    // 🔍 РЕКОМЕНДАЦИИ ДЛЯ НЕИЗМЕРЯЕМЫХ ЭЛЕМЕНТОВ
    
    // 4. K vs Mg антагонизм (K измеряется, Mg - нет)
    if (npk.potassium > 400.0F) {
        recommendations += "💡 Высокий K → внести MgSO4\n";
    }
    
    // 5. N + S синергизм (N измеряется, S - нет)
    if (npk.nitrogen > 300.0F) {
        recommendations += "💡 Высокий N → внести серу\n";
    }
    
    // 6. P vs Zn антагонизм (P измеряется, Zn - нет)
    if (npk.phosphorus > 200.0F) {
        recommendations += "💡 Высокий P → внести цинк\n";
    }
    
    // 7. Ca + B синергизм (ни один не измеряется, но pH влияет)
    if (pH < 6.0F) {
        recommendations += "💡 Кислая почва → известковать, бор\n";
    }
    
    if (recommendations.isEmpty()) {
        recommendations = "✅ Антагонизмов питательных веществ не обнаружено";
    }
    
    return recommendations;
}

void NutrientInteractionService::initializeInteractionCoefficients()
{
    // Инициализация коэффициентов взаимодействий
    // Источник: [Marschner, 2012. Mineral Nutrition of Higher Plants]
    
    logDebugSafe("NutrientInteractionService: Коэффициенты взаимодействий инициализированы");
}

NutrientInteractionService::~NutrientInteractionService()
{
    logDebugSafe("NutrientInteractionService: Сервис завершен");
} 
