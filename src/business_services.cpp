/**
 * @file business_services.cpp
 * @brief Реализация доступа к бизнес-сервисам
 */

#include "business_services.h"
#include "business/crop_recommendation_engine.h"
#include "business/sensor_calibration_service.h"
#include "business/sensor_compensation_service.h"
#include "business/nutrient_interaction_service.h"

// Глобальные экземпляры бизнес-сервисов (объявлены в main.cpp)
extern CropRecommendationEngine gCropEngine;
extern SensorCalibrationService gCalibrationService;
extern SensorCompensationService gCompensationService;
extern NutrientInteractionService gNutrientInteractionService;

// Получение экземпляров бизнес-сервисов
ICropRecommendationEngine& getCropEngine()
{  // NOLINT(misc-use-internal-linkage)
    return gCropEngine;
}

ISensorCalibrationService& getCalibrationService()
{  // NOLINT(misc-use-internal-linkage)
    return gCalibrationService;
}

ISensorCompensationService& getCompensationService()
{  // NOLINT(misc-use-internal-linkage)
    return gCompensationService;
}

INutrientInteractionService& getNutrientInteractionService()
{  // NOLINT(misc-use-internal-linkage)
    return gNutrientInteractionService;
}
