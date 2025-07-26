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

// Глобальные экземпляры бизнес-сервисов
extern SensorCalibrationService gCalibrationService;
extern SensorCompensationService gCompensationService;

namespace SensorProcessing {

// Константы для маппинга профилей почвы
static const std::array<SoilType, 5> SOIL_TYPES = {{
    SoilType::SAND,     // 0
    SoilType::LOAM,     // 1
    SoilType::PEAT,     // 2
    SoilType::CLAY,     // 3
    SoilType::SANDPEAT  // 4
}};

static const std::array<SoilProfile, 5> SOIL_PROFILES = {{
    SoilProfile::SAND,     // 0
    SoilProfile::LOAM,     // 1
    SoilProfile::PEAT,     // 2
    SoilProfile::CLAY,     // 3
    SoilProfile::SANDPEAT  // 4
}};

/**
 * @brief Получить тип почвы по индексу профиля
 * @param profileIndex Индекс профиля почвы (0-4)
 * @return SoilType соответствующий профилю
 */
SoilType getSoilType(int profileIndex) {
    if (profileIndex < 0 || profileIndex >= 5) {
        profileIndex = 1; // По умолчанию LOAM
    }
    return SOIL_TYPES[profileIndex];
}

/**
 * @brief Получить профиль почвы по индексу
 * @param profileIndex Индекс профиля почвы (0-4)
 * @return SoilProfile соответствующий профилю
 */
SoilProfile getSoilProfile(int profileIndex) {
    if (profileIndex < 0 || profileIndex >= 5) {
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
    // 1. ВСЕГДА применяем калибровку (расчет по точкам)
    logDebugSafe("📊 Применяем калибровку датчика");
    
    const SoilProfile profile = getSoilProfile(config.soilProfile);
    gCalibrationService.applyCalibration(sensorData, profile);

    // 2. Применяем научную компенсацию ТОЛЬКО если включена
    if (config.flags.calibrationEnabled) {
        logDebugSafe("🔬 Применяем научную компенсацию датчика");
        
        const SoilType soil = getSoilType(config.soilProfile);
        
        // EC: температурная компенсация по модели Арчи
        sensorData.ec = gCompensationService.correctEC(sensorData.ec, soil, sensorData.temperature, sensorData.humidity);

        // pH: температурная поправка по уравнению Нернста
        sensorData.ph = gCompensationService.correctPH(sensorData.temperature, sensorData.ph);

        // NPK: температурная и влажностная компенсация
        NPKReferences npk{sensorData.nitrogen, sensorData.phosphorus, sensorData.potassium};
        gCompensationService.correctNPK(sensorData.temperature, sensorData.humidity, soil, npk);

        // Сохраняем скорректированные NPK данные в sensorData
        sensorData.nitrogen = npk.nitrogen;
        sensorData.phosphorus = npk.phosphorus;
        sensorData.potassium = npk.potassium;
    }
}

} // namespace SensorProcessing 