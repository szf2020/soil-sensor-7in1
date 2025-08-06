/**
 * @file sensor_compensation_service.h
 * @brief Сервис компенсации датчиков
 * @details Применяет научные алгоритмы компенсации к показаниям датчиков
 */

#ifndef SENSOR_COMPENSATION_SERVICE_H
#define SENSOR_COMPENSATION_SERVICE_H

#include <Arduino.h>
#include <map>
#include "../../include/business/ISensorCompensationService.h"
#include "../../include/sensor_types.h"
#include "../../include/validation_utils.h"



/**
 * @brief Параметры почвы
 *
 * Содержит физические параметры почвы
 */
struct SoilParameters
{
    float porosity;       // Пористость
    float bulkDensity;    // Объемная плотность
    float fieldCapacity;  // Полевая влагоемкость

    SoilParameters() : porosity(0.45F), bulkDensity(1.40F), fieldCapacity(0.20F) {}
    SoilParameters(float por, float density, float capacity)
        : porosity(por), bulkDensity(density), fieldCapacity(capacity)
    {
    }
};

/**
 * @brief Коэффициенты NPK для разных типов почвы
 *
 * Содержит температурные и влажностные коэффициенты для NPK
 * Источник: [Delgado et al. (2020). DOI:10.1007/s42729-020-00215-4]
 */
struct NPKCoefficients
{
    float delta_N, delta_P, delta_K;        // Температурные коэффициенты
    float epsilon_N, epsilon_P, epsilon_K;  // Влажностные коэффициенты

    NPKCoefficients()
        : delta_N(0.0041F), delta_P(0.0053F), delta_K(0.0032F), epsilon_N(0.01F), epsilon_P(0.008F), epsilon_K(0.012F)
    {
    }
    NPKCoefficients(float dN, float dP, float dK, float eN, float eP, float eK)
        : delta_N(dN), delta_P(dP), delta_K(dK), epsilon_N(eN), epsilon_P(eP), epsilon_K(eK)
    {
    }
};

/**
 * @brief Сервис компенсации датчиков
 *
 * Реализует научные алгоритмы компенсации:
 * - Модель Арчи для EC
 * - Уравнение Нернста для pH
 * - Алгоритм Delgado et al. (2020) для NPK
 */
class SensorCompensationService : public ISensorCompensationService
{
   private:
    // Параметры почвы для разных типов
    std::map<SoilType, SoilParameters> soilParameters;

    // Коэффициенты NPK для разных типов почвы
    std::map<SoilType, NPKCoefficients> npkCoefficients;

    // Константы для расчетов
    static constexpr float R = 8.314F;    // Универсальная газовая постоянная (Дж/(моль·К))
    static constexpr float F = 96485.0F;  // Постоянная Фарадея (Кл/моль)
    static constexpr float T0 = 298.15F;  // Стандартная температура (25°C в Кельвинах)

    // Инициализация коэффициентов типов почвы
    void initializeSoilTypeCoefficients();

    // Инициализация параметров почвы
    void initializeSoilParameters();

    // Инициализация коэффициентов NPK
    void initializeNPKCoefficients();

    // Расчет температуры в Кельвинах
    static float temperatureToKelvin(float celsius);

    // УДАЛЕНО: Старые функции заменены на научные формулы модели Арчи
    // static float calculateECTemperatureFactor(float temperature); - УДАЛЕНО
    // float calculateECHumidityFactor(float humidity, SoilType soilType) const; - УДАЛЕНО

   public:
    /**
     * @brief Конструктор
     *
     * Инициализирует коэффициенты и параметры
     */
    SensorCompensationService();

    /**
     * @brief Деструктор
     */
    virtual ~SensorCompensationService() = default;

    /**
     * @brief Применяет компенсацию к данным датчика
     *
     * @param data Данные датчика для компенсации
     * @param soilType Тип почвы для выбора коэффициентов
     */
    void applyCompensation(SensorData& data, SoilType soilType) override;

    /**
     * @brief Компенсирует EC по модели Арчи
     *
     * @param ec25 EC при 25°C
     * @param soilType Тип почвы
     * @param temperature Температура почвы
     * @param humidity Влажность почвы
     * @return float Скомпенсированное значение EC
     */
    float correctEC(float ec25, SoilType soilType, float temperature) override;

    /**
     * @brief Компенсирует pH по уравнению Нернста
     *
     * @param phRaw Исходное значение pH
     * @param temperature Температура почвы
     * @return float Скомпенсированное значение pH
     */
    float correctPH(float temperature, float phRaw) override;

    /**
     * @brief Компенсирует NPK по алгоритму Delgado et al. (2020)
     *
     * @param temperature Температура почвы
     * @param humidity Влажность почвы
     * @param npk NPK значения для компенсации
     * @param soilType Тип почвы
     */
    void correctNPK(float temperature, float humidity, SoilType soilType, NPKReferences& npk) override;

    /**
     * @brief Получает пористость для типа почвы
     *
     * @param soilType Тип почвы
     * @return float Пористость почвы
     */
    float getPorosity(SoilType soilType) const override;

    /**
     * @brief Проверяет валидность входных данных для компенсации
     *
     * @param soilType Тип почвы
     * @param temperature Температура
     * @param humidity Влажность
     * @return true если данные валидны, false в противном случае
     */
    bool validateCompensationInputs(SoilType soilType, float humidity, float temperature) const override;

    /**
     * @brief Получает параметры почвы для типа
     *
     * @param soilType Тип почвы
     * @return SoilParameters Параметры почвы
     */
    SoilParameters getSoilParameters(SoilType soilType) const;



    /**
     * @brief Получает коэффициенты NPK для типа почвы
     *
     * @param soilType Тип почвы
     * @return NPKCoefficients Коэффициенты NPK
     */
    NPKCoefficients getNPKCoefficients(SoilType soilType) const;

    /**
     * @brief Пересчитывает VWC в ASM (Available Soil Moisture)
     *
     * @param vwc Объемная влажность почвы (сырые данные датчика)
     * @param soilType Тип почвы
     * @return float ASM в процентах (0-100%)
     */
    float vwcToAsm(float vwc, SoilType soilType) const;

    /**
     * @brief Пересчитывает ASM в VWC (Volumetric Water Content)
     *
     * @param asmValue Available Soil Moisture в процентах (0-100%)
     * @param soilType Тип почвы
     * @return float VWC в процентах от объема почвы
     */
    float asmToVwc(float asmValue, SoilType soilType) const;
};

#endif  // SENSOR_COMPENSATION_SERVICE_H
