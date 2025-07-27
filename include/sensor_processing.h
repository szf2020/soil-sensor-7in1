/**
 * @file sensor_processing.h
 * @brief Заголовочный файл для общей логики обработки данных датчиков
 * @details Унифицированная обработка калибровки и компенсации для всех типов датчиков
 */

#pragma once

#include "jxct_config_vars.h"  // Для Config
#include "modbus_sensor.h"     // Для SensorData
#include "sensor_types.h"      // Для SoilType, SoilProfile

namespace SensorProcessing
{

/**
 * @brief Получить тип почвы по индексу профиля
 * @param profileIndex Индекс профиля почвы (0-4)
 * @return SoilType соответствующий профилю
 */
SoilType getSoilType(int profileIndex);

/**
 * @brief Получить профиль почвы по индексу
 * @param profileIndex Индекс профиля почвы (0-4)
 * @return SoilProfile соответствующий профилю
 */
SoilProfile getSoilProfile(int profileIndex);

/**
 * @brief Применить полную обработку данных датчика
 * @param sensorData Данные датчика для обработки
 * @param config Конфигурация системы
 *
 * @details Применяет:
 * 1. Калибровку (ВСЕГДА) - расчет по точкам
 * 2. Компенсацию (если включена) - научные формулы
 */
void processSensorData(SensorData& sensorData, const Config& config);

}  // namespace SensorProcessing
