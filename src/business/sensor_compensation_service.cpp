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
    initializeSoilParameters();
    initializeNPKCoefficients();
}

void SensorCompensationService::applyCompensation(SensorData& data, SoilType soilType)
{
    logDebugSafe("SensorCompensationService: Применение компенсации для типа почвы %d", static_cast<int>(soilType));

    // Применяем компенсацию к каждому параметру
    data.ec = correctEC(data.ec, soilType, data.temperature);
    data.ph = correctPH(data.temperature, data.ph);  // ИСПРАВЛЕНО: температура, затем pH

    NPKReferences npk(data.nitrogen, data.phosphorus, data.potassium);
    correctNPK(data.temperature, data.humidity, soilType, npk);

    data.nitrogen = npk.nitrogen;
    data.phosphorus = npk.phosphorus;
    data.potassium = npk.potassium;

    logDebugSafe("SensorCompensationService: Компенсация применена");
}

float SensorCompensationService::correctEC(float ec25_param, SoilType soilType_param, float temperature_param)
{
    if (!validateCompensationInputs(soilType_param, 50.0F, temperature_param))  // Используем среднюю влажность
    {
        logDebugSafe("SensorCompensationService: Недопустимые входные данные для компенсации EC");
        return ec25_param;
    }

    // ИСПРАВЛЕНО: Упрощенная формула только с температурной компенсацией
    // Источник: [Rhoades et al., 1989. Soil Science Society of America Journal]
    
    // Температурная компенсация: линейная формула
    const float tempFactor = 1.0F + 0.021F * (temperature_param - 25.0F);
    
    // Применяем компенсацию: EC = EC0 × tempFactor
    float compensatedEC = ec25_param * tempFactor;

    logDebugSafe("SensorCompensationService: EC скорректирован %.2f → %.2f (tempFactor=%.3f)", 
                 ec25_param, compensatedEC, tempFactor);
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

    // НАУЧНАЯ ФОРМУЛА: NPK компенсация (Delgado et al., 2020)
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
        case SoilType::SILT:
        case SoilType::CLAY_LOAM:
        case SoilType::ORGANIC:
        case SoilType::SANDY_LOAM:
        case SoilType::SILTY_LOAM:
        case SoilType::LOAMY_CLAY:
        case SoilType::SALINE:
        case SoilType::ALKALINE:
            return true;
        default:
            return false;
    }
}



namespace
{
void initializeSoilParameters(std::map<SoilType, SoilParameters>& soilParameters)
{
    // Параметры почвы для разных типов
    // Источник: [Delgado et al. (2020). DOI:10.1007/s42729-020-00215-4]

    soilParameters[SoilType::SAND] = SoilParameters(0.35F, 1.60F, 0.10F);
    soilParameters[SoilType::LOAM] = SoilParameters(0.45F, 1.40F, 0.20F);
    soilParameters[SoilType::PEAT] = SoilParameters(0.80F, 0.30F, 0.45F);
    soilParameters[SoilType::CLAY] = SoilParameters(0.50F, 1.20F, 0.35F);
    soilParameters[SoilType::SANDPEAT] = SoilParameters(0.60F, 0.80F, 0.30F);
    
    // Новые типы почв
    soilParameters[SoilType::SILT] = SoilParameters(0.40F, 1.30F, 0.15F);           // Иловая почва
    soilParameters[SoilType::CLAY_LOAM] = SoilParameters(0.55F, 1.25F, 0.25F);      // Глинистый суглинок
    soilParameters[SoilType::ORGANIC] = SoilParameters(0.90F, 0.25F, 0.50F);        // Органическая почва
    soilParameters[SoilType::SANDY_LOAM] = SoilParameters(0.30F, 1.50F, 0.12F);     // Песчанистый суглинок
    soilParameters[SoilType::SILTY_LOAM] = SoilParameters(0.42F, 1.35F, 0.18F);     // Иловатый суглинок
    soilParameters[SoilType::LOAMY_CLAY] = SoilParameters(0.60F, 1.15F, 0.30F);     // Суглинистая глина
    soilParameters[SoilType::SALINE] = SoilParameters(0.35F, 1.45F, 0.20F);         // Засоленная почва
    soilParameters[SoilType::ALKALINE] = SoilParameters(0.50F, 1.30F, 0.25F);       // Щелочная почва

    logDebugSafe("SensorCompensationService: Параметры почвы инициализированы (13 типов)");
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
    
    // Новые типы почв - коэффициенты NPK
    npkCoefficients[SoilType::SILT] = NPKCoefficients(0.0035F, 0.0045F, 0.0027F, 0.009F, 0.007F, 0.011F);
    npkCoefficients[SoilType::CLAY_LOAM] = NPKCoefficients(0.0030F, 0.0039F, 0.0022F, 0.008F, 0.006F, 0.010F);
    npkCoefficients[SoilType::ORGANIC] = NPKCoefficients(0.0025F, 0.0032F, 0.0016F, 0.013F, 0.010F, 0.016F);
    npkCoefficients[SoilType::SANDY_LOAM] = NPKCoefficients(0.0039F, 0.0050F, 0.0030F, 0.010F, 0.008F, 0.012F);
    npkCoefficients[SoilType::SILTY_LOAM] = NPKCoefficients(0.0036F, 0.0047F, 0.0028F, 0.009F, 0.007F, 0.011F);
    npkCoefficients[SoilType::LOAMY_CLAY] = NPKCoefficients(0.0028F, 0.0037F, 0.0021F, 0.008F, 0.006F, 0.010F);
    npkCoefficients[SoilType::SALINE] = NPKCoefficients(0.0045F, 0.0058F, 0.0035F, 0.007F, 0.005F, 0.008F);
    npkCoefficients[SoilType::ALKALINE] = NPKCoefficients(0.0033F, 0.0043F, 0.0026F, 0.009F, 0.007F, 0.011F);

    logDebugSafe("SensorCompensationService: Коэффициенты NPK инициализированы (13 типов)");
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

// ============================================================================
// ФУНКЦИИ ДЛЯ ПЕРЕСЧЕТА VWC ↔ ASM (Available Soil Moisture)
// ============================================================================

float SensorCompensationService::vwcToAsm(float vwc, SoilType soilType) const
{
    // Получаем параметры почвы
    SoilParameters params = getSoilParameters(soilType);
    
    // ASM = (VWC - PWP) / (FC - PWP) * 100%
    // Где: VWC - объемная влажность почвы (сырые данные датчика)
    //      FC - полевая влагоемкость (Field Capacity)
    //      PWP - точка увядания (Permanent Wilting Point)
    
    // Приближенные значения PWP для разных типов почв (USDA)
    float pwp = 0.0F;
    switch (soilType) {
        case SoilType::SAND: pwp = 0.05F; break;
        case SoilType::SANDY_LOAM: pwp = 0.08F; break;
        case SoilType::LOAM: pwp = 0.12F; break;
        case SoilType::SILTY_LOAM: pwp = 0.15F; break;
        case SoilType::CLAY_LOAM: pwp = 0.18F; break;
        case SoilType::CLAY: pwp = 0.20F; break;
        case SoilType::PEAT: pwp = 0.25F; break;
        case SoilType::SILT: pwp = 0.10F; break;
        case SoilType::ORGANIC: pwp = 0.20F; break;
        case SoilType::SANDPEAT: pwp = 0.12F; break;
        case SoilType::LOAMY_CLAY: pwp = 0.22F; break;
        case SoilType::SALINE: pwp = 0.12F; break;
        case SoilType::ALKALINE: pwp = 0.15F; break;
        default: pwp = 0.12F; break;
    }
    
    float fc = params.fieldCapacity;
    
    // Защита от деления на ноль
    if (fc <= pwp) {
        return 0.0F;
    }
    
        // Пересчет VWC в ASM
    float asmValue = (vwc - pwp) / (fc - pwp) * 100.0F;

    // Ограничиваем значения от 0 до 100%
    if (asmValue < 0.0F) asmValue = 0.0F;
    if (asmValue > 100.0F) asmValue = 100.0F;

    return asmValue;
}

float SensorCompensationService::asmToVwc(float asmValue, SoilType soilType) const
{
    // Получаем параметры почвы
    SoilParameters params = getSoilParameters(soilType);

    // VWC = PWP + (ASM / 100%) * (FC - PWP)
    // Обратный пересчет ASM в VWC

    // Приближенные значения PWP для разных типов почв (USDA)
    float pwp = 0.0F;
    switch (soilType) {
        case SoilType::SAND: pwp = 0.05F; break;
        case SoilType::SANDY_LOAM: pwp = 0.08F; break;
        case SoilType::LOAM: pwp = 0.12F; break;
        case SoilType::SILTY_LOAM: pwp = 0.15F; break;
        case SoilType::CLAY_LOAM: pwp = 0.18F; break;
        case SoilType::CLAY: pwp = 0.20F; break;
        case SoilType::PEAT: pwp = 0.25F; break;
        case SoilType::SILT: pwp = 0.10F; break;
        case SoilType::ORGANIC: pwp = 0.20F; break;
        case SoilType::SANDPEAT: pwp = 0.12F; break;
        case SoilType::LOAMY_CLAY: pwp = 0.22F; break;
        case SoilType::SALINE: pwp = 0.12F; break;
        case SoilType::ALKALINE: pwp = 0.15F; break;
        default: pwp = 0.12F; break;
    }

    float fc = params.fieldCapacity;

    // Защита от деления на ноль
    if (fc <= pwp) {
        return pwp;
    }

    // Пересчет ASM в VWC
    float vwc = pwp + (asmValue / 100.0F) * (fc - pwp);

    // Ограничиваем значения
    if (vwc < pwp) vwc = pwp;
    if (vwc > fc) vwc = fc;

    return vwc;
}
