/**
 * @file ISensorCalibrationService.h
 * @brief Интерфейс для сервиса калибровки датчиков
 * @details Определяет контракт для применения калибровочных данных
 */

#ifndef ISENSOR_CALIBRATION_SERVICE_H
#define ISENSOR_CALIBRATION_SERVICE_H

#include <Arduino.h>
#include "../sensor_types.h"
#include "../validation_utils.h"

/**
 * @brief Интерфейс для сервиса калибровки датчиков
 *
 * Определяет контракт для применения калибровочных данных к показаниям датчиков,
 * включая загрузку калибровочных таблиц и их применение.
 */
class ISensorCalibrationService
{
   public:
    virtual ~ISensorCalibrationService() = default;

    /**
     * @brief Применяет калибровку к данным датчика
     *
     * @param data Данные датчика для калибровки
     * @param profile Профиль почвы для выбора калибровочной таблицы
     */
    virtual void applyCalibration(SensorData& data, SoilProfile profile) = 0;

    /**
     * @brief Применяет калибровку к отдельному значению
     *
     * @param rawValue Исходное значение
     * @param profile Профиль почвы
     * @return float Откалиброванное значение
     */
    virtual float applySingleCalibration(float rawValue, SoilProfile profile) = 0;

    /**
     * @brief Проверяет валидность калибровочных данных
     *
     * @param data Данные датчика для проверки
     * @return true если данные валидны, false в противном случае
     */
    virtual bool validateCalibrationData(const SensorData& data) = 0;

    /**
     * @brief Загружает калибровочную таблицу из CSV
     *
     * @param csvData CSV данные калибровочной таблицы
     * @param profile Профиль почвы
     * @return true если загрузка успешна, false в противном случае
     */
    virtual bool loadCalibrationTable(const String& csvData, SoilProfile profile) = 0;

    /**
     * @brief Проверяет наличие калибровочной таблицы
     *
     * @param profile Профиль почвы
     * @return true если таблица загружена, false в противном случае
     */
    virtual bool hasCalibrationTable(SoilProfile profile) const = 0;

    /**
     * @brief Очищает калибровочную таблицу
     *
     * @param profile Профиль почвы
     */
    virtual void clearCalibrationTable(SoilProfile profile) = 0;

    /**
     * @brief Добавляет точку калибровки pH
     *
     * @param expected Ожидаемое значение
     * @param measured Измеренное значение
     * @return true если добавление успешно
     */
    virtual bool addPHCalibrationPoint(float expected, float measured) = 0;

    /**
     * @brief Добавляет точку калибровки EC
     *
     * @param expected Ожидаемое значение
     * @param measured Измеренное значение
     * @return true если добавление успешно
     */
    virtual bool addECCalibrationPoint(float expected, float measured) = 0;

    /**
     * @brief Добавляет точку калибровки температуры
     *
     * @param expected Ожидаемое значение
     * @param measured Измеренное значение
     * @return true если добавление успешно
     */
    virtual bool addTemperatureCalibrationPoint(float expected, float measured) = 0;

    /**
     * @brief Добавляет точку калибровки влажности
     *
     * @param expected Ожидаемое значение
     * @param measured Измеренное значение
     * @return true если добавление успешно
     */
    virtual bool addHumidityCalibrationPoint(float expected, float measured) = 0;

    /**
     * @brief Устанавливает точку калибровки NPK
     *
     * @param nitrogen Значение азота
     * @param phosphorus Значение фосфора
     * @param potassium Значение калия
     * @return true если установка успешна
     */
    virtual bool setNPKCalibrationPoint(float nitrogen, float phosphorus, float potassium) = 0;

    /**
     * @brief Получает количество точек калибровки
     *
     * @param profile Профиль почвы
     * @param sensorType Тип датчика
     * @return Количество точек
     */
    virtual size_t getCalibrationPointsCount(SoilProfile profile, const String& sensorType) = 0;

    /**
     * @brief Устанавливает offset калибровки температуры
     *
     * @param offset Смещение температуры
     * @return true если установка успешна
     */
    virtual bool setTemperatureOffset(float offset) = 0;

    /**
     * @brief Устанавливает offset калибровки влажности
     *
     * @param offset Смещение влажности
     * @return true если установка успешна
     */
    virtual bool setHumidityOffset(float offset) = 0;

    /**
     * @brief Устанавливает коэффициенты калибровки NPK
     *
     * @param nSlope Коэффициент наклона для азота
     * @param nOffset Смещение для азота
     * @param pSlope Коэффициент наклона для фосфора
     * @param pOffset Смещение для фосфора
     * @param kSlope Коэффициент наклона для калия
     * @param kOffset Смещение для калия
     * @return true если установка успешна
     */
    virtual bool setNPKCoefficients(float nSlope, float nOffset, float pSlope, float pOffset, float kSlope, float kOffset) = 0;
};

#endif  // ISENSOR_CALIBRATION_SERVICE_H
