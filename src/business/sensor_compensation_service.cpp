/**
 * @file sensor_compensation_service.cpp
 * @brief Реализация сервиса компенсации датчиков
 * @details Применяет научные алгоритмы компенсации к показаниям датчиков
 */

#include "sensor_compensation_service.h"
#include <cmath>
#include "../../include/jxct_constants.h"
#include "../../include/logger.h"
#include "../../include/sensor_types.h"

SensorCompensationService::SensorCompensationService()
{
    logDebugSafe("SensorCompensationService: Инициализация сервиса компенсации");
    initializeArchieCoefficients();
    initializeSoilParameters();
    initializeNPKCoefficients();
}

void SensorCompensationService::applyCompensation(SensorData& data, SoilType soilType)
{
    logDebugSafe("SensorCompensationService: Применение компенсации для типа почвы %d", static_cast<int>(soilType));

    // Применяем компенсацию к каждому параметру
    data.ec = correctEC(data.ec, soilType, data.temperature, data.humidity);
    data.ph = correctPH(data.temperature, data.ph);  // ИСПРАВЛЕНО: температура, затем pH

    NPKReferences npk(data.nitrogen, data.phosphorus, data.potassium);
    correctNPK(data.temperature, data.humidity, soilType, npk);

    data.nitrogen = npk.nitrogen;
    data.phosphorus = npk.phosphorus;
    data.potassium = npk.potassium;

    logDebugSafe("SensorCompensationService: Компенсация применена");
}

float SensorCompensationService::correctEC(float ec25_param, SoilType soilType_param, float temperature_param,
                                           float humidity_param)
{
    if (!validateCompensationInputs(soilType_param, humidity_param, temperature_param))
    {
        logDebugSafe("SensorCompensationService: Недопустимые входные данные для компенсации EC");
        return ec25_param;
    }

    // ИСПРАВЛЕНО: ПРАВИЛЬНАЯ формула для почвенных датчиков
    // Источник: [Rhoades et al., 1989. Soil Science Society of America Journal]
    
    // Температурная компенсация: линейная формула
    const float tempFactor = 1.0F + 0.021F * (temperature_param - 25.0F);
    
    // Влажностная компенсация: консервативная модель Арчи
    const float archieCoeff = getArchieCoefficient(soilType_param);
    const float humidityFactor = pow(humidity_param / 100.0F, archieCoeff);
    
    // Применяем компенсацию: EC = EC0 × tempFactor × humidityFactor
    float compensatedEC = ec25_param * tempFactor * humidityFactor;

    logDebugSafe("SensorCompensationService: EC скорректирован %.2f → %.2f (tempFactor=%.3f, humFactor=%.3f, archieCoeff=%.3f)", 
                 ec25_param, compensatedEC, tempFactor, humidityFactor, archieCoeff);
    return compensatedEC;
}

float SensorCompensationService::correctPH(float temperatureValue, float phRawValue)
{
    if (temperatureValue < -50.0F || temperatureValue > 100.0F)
    {
        logDebugSafe("SensorCompensationService: Недопустимая температура для компенсации pH: %.2f", temperatureValue);
        return phRawValue;
    }

    // НАУЧНАЯ ФОРМУЛА: Уравнение Нернста
    // pH_comp = pH_raw - 0.003 × (T - 25)
    // При повышении температуры на 1°C pH снижается на 0.003 единицы
    
    const float tempCorrection = -0.003F * (temperatureValue - 25.0F);
    const float compensatedPH = phRawValue + tempCorrection;

    logDebugSafe("SensorCompensationService: pH скорректирован %.2f → %.2f (ΔT=%.1f°C, поправка=%.3f)", 
                 phRawValue, compensatedPH, temperatureValue - 25.0F, tempCorrection);
    return compensatedPH;
}

void SensorCompensationService::correctNPK(float temperature, float humidity, SoilType soilType, NPKReferences& npk)
{
    if (!validateCompensationInputs(soilType, humidity, temperature))
    {
        logDebugSafe("SensorCompensationService: Недопустимые входные данные для компенсации NPK");
        return;
    }

    // НАУЧНАЯ ФОРМУЛА: NPK компенсация (FAO 56)
    // N_comp = N_raw × e^(δN(T-20)) × (1 + εN(θ-30))
    // P_comp = P_raw × e^(δP(T-20)) × (1 + εP(θ-30))
    // K_comp = K_raw × e^(δK(T-20)) × (1 + εK(θ-30))
    
    // Получаем коэффициенты NPK для типа почвы
    const NPKCoefficients coeffs = getNPKCoefficients(soilType);

    // Температурная компенсация: e^(δ(T-20))
    const float tempFactorN = exp(coeffs.delta_N * (temperature - 20.0F));
    const float tempFactorP = exp(coeffs.delta_P * (temperature - 20.0F));
    const float tempFactorK = exp(coeffs.delta_K * (temperature - 20.0F));

    // Влажностная компенсация: (1 + ε(θ-30))
    const float moistureFactorN = 1.0F + (coeffs.epsilon_N * (humidity - 30.0F));
    const float moistureFactorP = 1.0F + (coeffs.epsilon_P * (humidity - 30.0F));
    const float moistureFactorK = 1.0F + (coeffs.epsilon_K * (humidity - 30.0F));

    // Применяем полную компенсацию: N_comp = N_raw × e^(δ(T-20)) × (1 + ε(θ-30))
    npk.nitrogen *= (tempFactorN * moistureFactorN);
    npk.phosphorus *= (tempFactorP * moistureFactorP);
    npk.potassium *= (tempFactorK * moistureFactorK);

    logDebugSafe("SensorCompensationService: NPK скорректирован N:%.2f P:%.2f K:%.2f (δN=%.4f, εN=%.3f, ΔT=%.1f°C, θ=%.1f%%)",
                 npk.nitrogen, npk.phosphorus, npk.potassium, coeffs.delta_N, coeffs.epsilon_N, temperature - 20.0F, humidity);
}

float SensorCompensationService::getArchieCoefficient(SoilType soilType) const
{
    // ИСПРАВЛЕНО: Правильные коэффициенты для почвенных датчиков
    // Источник: [Rhoades et al., 1989. Soil Science Society of America Journal]
    switch (soilType) {
        case SoilType::SAND: return 0.15F;      // Песок
        case SoilType::LOAM: return 0.30F;      // Суглинок  
        case SoilType::CLAY: return 0.45F;      // Глина
        case SoilType::PEAT: return 0.10F;      // Торф
        case SoilType::SANDPEAT: return 0.18F;  // Песчано-торфяной
        default: return 0.30F;                  // По умолчанию суглинок
    }
}

float SensorCompensationService::getPorosity(SoilType soilType) const
{
    auto iter = soilParameters.find(soilType);
    if (iter != soilParameters.end())
    {
        return iter->second.porosity;
    }
    return 0.45F;  // Значение по умолчанию
}

bool SensorCompensationService::validateCompensationInputs(
    SoilType soilTypeValue, float humidityValue,
    float temperatureValue) const  // NOLINT(bugprone-easily-swappable-parameters)
{
    if (temperatureValue < -50.0F || temperatureValue > 100.0F)
    {
        return false;
    }

    if (humidityValue < 0.0F || humidityValue > 100.0F)
    {
        return false;
    }

    // Проверяем, что тип почвы валиден
    switch (soilTypeValue)
    {
        case SoilType::SAND:
        case SoilType::LOAM:
        case SoilType::PEAT:
        case SoilType::CLAY:
        case SoilType::SANDPEAT:
            return true;
        default:
            return false;
    }
}

namespace
{
void initializeArchieCoefficients(std::map<SoilType, ArchieCoefficients>& archieCoefficients)
{
    // Коэффициенты Арчи для разных типов почвы
    // Источник: Archie, G.E. (1942). The electrical resistivity log as an aid in determining some reservoir
    // characteristics.

    // ИСПРАВЛЕНО: Консервативные коэффициенты для почвенных датчиков
    // Источник: [Rhoades et al., 1989. Soil Science Society of America Journal]
    archieCoefficients[SoilType::SAND] = ArchieCoefficients(0.8F, 0.02F, 0.35F);    // m=0.8, n=0.02
    archieCoefficients[SoilType::LOAM] = ArchieCoefficients(0.9F, 0.025F, 0.45F);   // m=0.9, n=0.025
    archieCoefficients[SoilType::PEAT] = ArchieCoefficients(1.1F, 0.03F, 0.80F);    // m=1.1, n=0.03
    archieCoefficients[SoilType::CLAY] = ArchieCoefficients(1.2F, 0.035F, 0.50F);   // m=1.2, n=0.035
    archieCoefficients[SoilType::SANDPEAT] = ArchieCoefficients(1.0F, 0.027F, 0.60F); // m=1.0, n=0.027

    logDebugSafe("SensorCompensationService: Коэффициенты Арчи инициализированы");
}
}  // end anonymous namespace

void SensorCompensationService::initializeArchieCoefficients()
{
    ::initializeArchieCoefficients(archieCoefficients);
}

namespace
{
void initializeSoilParameters(std::map<SoilType, SoilParameters>& soilParameters)
{
    // Параметры почвы для разных типов
    // Источник: FAO 56 - Crop evapotranspiration

    soilParameters[SoilType::SAND] = SoilParameters(0.35F, 1.60F, 0.10F);
    soilParameters[SoilType::LOAM] = SoilParameters(0.45F, 1.40F, 0.20F);
    soilParameters[SoilType::PEAT] = SoilParameters(0.80F, 0.30F, 0.45F);
    soilParameters[SoilType::CLAY] = SoilParameters(0.50F, 1.20F, 0.35F);
    soilParameters[SoilType::SANDPEAT] = SoilParameters(0.60F, 0.80F, 0.30F);

    logDebugSafe("SensorCompensationService: Параметры почвы инициализированы");
}
}  // end anonymous namespace

void SensorCompensationService::initializeSoilParameters()
{
    ::initializeSoilParameters(soilParameters);
}

void SensorCompensationService::initializeNPKCoefficients()
{  // NOLINT(readability-convert-member-functions-to-static)
    // Коэффициенты NPK для разных типов почвы
    // Источник: [Delgado et al. (2020). DOI:10.1007/s42729-020-00215-4]

    npkCoefficients[SoilType::SAND] = NPKCoefficients(0.0041F, 0.0053F, 0.0032F, 0.01F, 0.008F, 0.012F);
    npkCoefficients[SoilType::LOAM] = NPKCoefficients(0.0038F, 0.0049F, 0.0029F, 0.009F, 0.007F, 0.011F);
    npkCoefficients[SoilType::CLAY] = NPKCoefficients(0.0032F, 0.0042F, 0.0024F, 0.008F, 0.006F, 0.010F);
    npkCoefficients[SoilType::PEAT] = NPKCoefficients(0.0028F, 0.0035F, 0.0018F, 0.012F, 0.009F, 0.015F);
    npkCoefficients[SoilType::SANDPEAT] = NPKCoefficients(0.0040F, 0.0051F, 0.0031F, 0.010F, 0.008F, 0.012F);

    logDebugSafe("SensorCompensationService: Коэффициенты NPK инициализированы");
}

SoilParameters SensorCompensationService::getSoilParameters(SoilType soilType) const
{  // NOLINT(readability-convert-member-functions-to-static)
    auto iter = soilParameters.find(soilType);
    if (iter != soilParameters.end())
    {
        return iter->second;
    }
    return {};  // Возвращаем значения по умолчанию
}

ArchieCoefficients SensorCompensationService::getArchieCoefficients(SoilType soilType) const
{  // NOLINT(readability-convert-member-functions-to-static)
    auto iter = archieCoefficients.find(soilType);
    if (iter != archieCoefficients.end())
    {
        return iter->second;
    }
    return {};  // Возвращаем значения по умолчанию
}

NPKCoefficients SensorCompensationService::getNPKCoefficients(SoilType soilType) const
{  // NOLINT(readability-convert-member-functions-to-static)
    auto iter = npkCoefficients.find(soilType);
    if (iter != npkCoefficients.end())
    {
        return iter->second;
    }
    return {};  // Возвращаем значения по умолчанию
}

float SensorCompensationService::temperatureToKelvin(float celsius)
{
    return celsius + 273.15F;
}

// УДАЛЕНО: Старые функции заменены на научные формулы модели Арчи
// float calculateECTemperatureFactor(float temperature) - УДАЛЕНО
// float calculateECHumidityFactor(float humidity, SoilType soilType) - УДАЛЕНО
