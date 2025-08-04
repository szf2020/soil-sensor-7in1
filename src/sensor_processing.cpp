/**
 * @file sensor_processing.cpp
 * @brief Общая логика обработки данных датчиков
 * @details Унифицированная обработка калибровки и компенсации для всех типов датчиков
 */

#include "sensor_processing.h"
#include "logger.h"
#include "jxct_config_vars.h"
#include "business/sensor_calibration_service.h"
#include "business/sensor_compensation_service.h"
#include "sensor_types.h"
#include "advanced_filters.h"

// Глобальные экземпляры бизнес-сервисов
extern SensorCalibrationService gCalibrationService;
extern SensorCompensationService gCompensationService;

namespace SensorProcessing {

// Константы для маппинга профилей почвы
static const std::array<SoilType, 13> SOIL_TYPES = {{
    SoilType::SAND,        // 0
    SoilType::LOAM,        // 1
    SoilType::PEAT,        // 2
    SoilType::CLAY,        // 3
    SoilType::SANDPEAT,    // 4
    SoilType::SILT,        // 5 - НОВЫЙ
    SoilType::CLAY_LOAM,   // 6 - Глинистый суглинок - НОВЫЙ
    SoilType::ORGANIC,     // 7 - НОВЫЙ
    SoilType::SANDY_LOAM,  // 8 - Песчанистый суглинок - НОВЫЙ
    SoilType::SILTY_LOAM,  // 9 - НОВЫЙ
    SoilType::LOAMY_CLAY,  // 10 - Суглинистая глина - НОВЫЙ
    SoilType::SALINE,      // 11 - НОВЫЙ
    SoilType::ALKALINE     // 12 - НОВЫЙ
}};

static const std::array<SoilProfile, 13> SOIL_PROFILES = {{
    SoilProfile::SAND,        // 0
    SoilProfile::LOAM,        // 1
    SoilProfile::PEAT,        // 2
    SoilProfile::CLAY,        // 3
    SoilProfile::SANDPEAT,    // 4
    SoilProfile::SILT,        // 5 - НОВЫЙ
    SoilProfile::CLAY_LOAM,   // 6 - Суглинок - НОВЫЙ
    SoilProfile::ORGANIC,     // 7 - НОВЫЙ
    SoilProfile::SANDY_LOAM,  // 8 - Лёгкий суглинок - НОВЫЙ
    SoilProfile::SILTY_LOAM,  // 9 - НОВЫЙ
    SoilProfile::LOAMY_CLAY,  // 10 - Тяжелый суглинок - НОВЫЙ
    SoilProfile::SALINE,      // 11 - НОВЫЙ
    SoilProfile::ALKALINE     // 12 - НОВЫЙ
}};

/**
 * @brief Получить тип почвы по индексу профиля
 * @param profileIndex Индекс профиля почвы (0-12)
 * @return SoilType соответствующий профилю
 */
SoilType getSoilType(int profileIndex) {
    if (profileIndex < 0 || profileIndex >= 13) {
        profileIndex = 1; // По умолчанию LOAM
    }
    return SOIL_TYPES[profileIndex];
}

/**
 * @brief Получить профиль почвы по индексу
 * @param profileIndex Индекс профиля почвы (0-12)
 * @return SoilProfile соответствующий профилю
 */
SoilProfile getSoilProfile(int profileIndex) {
    if (profileIndex < 0 || profileIndex >= 13) {
        profileIndex = 1; // По умолчанию LOAM
    }
    return SOIL_PROFILES[profileIndex];
}

/**
 * @brief Применить полную обработку данных датчика
 * @param sensorData Данные датчика для обработки
 * @param config Конфигурация системы
 */
void processSensorData(SensorData& sensorData, const Config& config) {
    // 1. Применяем калибровку ТОЛЬКО если включена
    if (config.flags.calibrationEnabled) {
        logDebugSafe("📊 Применяем калибровку датчика");
        
        const SoilProfile profile = getSoilProfile(config.soilProfile);
        gCalibrationService.applyCalibration(sensorData, profile);
    } else {
        logDebugSafe("📊 Калибровка отключена");
    }

    // 2. Применяем научную компенсацию ТОЛЬКО если включена
    if (config.flags.compensationEnabled) {  // ✅ Правильный флаг для компенсации
        logDebugSafe("🔬 Применяем научную компенсацию датчика");
        
        const SoilType soil = getSoilType(config.soilProfile);
        
        // EC: консервативная температурная компенсация
        sensorData.ec = gCompensationService.correctEC(sensorData.ec, soil, sensorData.temperature);

        // pH: температурная поправка по уравнению Нернста
        sensorData.ph = gCompensationService.correctPH(sensorData.temperature, sensorData.ph);

        // NPK: температурная и влажностная компенсация
        NPKReferences npk{sensorData.nitrogen, sensorData.phosphorus, sensorData.potassium};
        gCompensationService.correctNPK(sensorData.temperature, sensorData.humidity, soil, npk);

        // Сохраняем скорректированные NPK данные в sensorData
        sensorData.nitrogen = npk.nitrogen;
        sensorData.phosphorus = npk.phosphorus;
        sensorData.potassium = npk.potassium;
    } else {
        logDebugSafe("🔬 Компенсация отключена");
    }

    // 3. Применяем улучшенную фильтрацию ТОЛЬКО если включена
    if (config.adaptiveFiltering || config.kalmanEnabled) {
        logDebugSafe("🔧 Применяем улучшенную фильтрацию");
        AdvancedFilters::applyAdvancedFiltering(sensorData);
    } else {
        logDebugSafe("🔧 Фильтрация отключена");
    }
}

} // namespace SensorProcessing 