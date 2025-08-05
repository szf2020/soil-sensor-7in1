/**
 * @file sensor_calibration_service.cpp
 * @brief Реализация сервиса калибровки датчиков
 * @details Применяет калибровочные данные к показаниям датчиков
 */

#include "sensor_calibration_service.h"
#include "../../include/calibration_manager.h"
#include "../../include/jxct_constants.h"
#include "../../include/logger.h"
#include "validation_utils.h"  // Для централизованной валидации

// Внутренние переменные и функции
namespace
{
// Внутренняя таблица калибровки с внутренней связностью
std::map<SoilProfile, CalibrationTable> calibrationTablesInternal;
}  // namespace

// Функция доступа к внутренней таблице калибровки
std::map<SoilProfile, CalibrationTable>& getCalibrationTables()
{  // NOLINT(misc-use-internal-linkage)
    return calibrationTablesInternal;
}

SensorCalibrationService::SensorCalibrationService()
{
    logDebugSafe("SensorCalibrationService: Инициализация сервиса калибровки");
}

void SensorCalibrationService::applyCalibration(SensorData& data, SoilProfile profile)
{  // NOLINT(readability-make-member-function-const)
    logDebugSafe("SensorCalibrationService: Применение калибровки для профиля %d", static_cast<int>(profile));

    // Сохраняем исходные значения
    // Сохраняем сырые значения (если структура поддерживает)
    // data.raw_temperature = data.temperature;
    // data.raw_humidity = data.humidity;
    // data.raw_ec = data.ec;
    // data.raw_ph = data.ph;
    // data.raw_nitrogen = data.nitrogen;
    // data.raw_phosphorus = data.phosphorus;
    // data.raw_potassium = data.potassium;

    // Применяем калибровку к каждому параметру
    if (hasCalibrationTable(profile))
    {
        const CalibrationTable& table = getCalibrationTables()[profile];

        if (!table.temperaturePoints.empty())
        {
            data.temperature = applyCalibrationWithInterpolation(data.temperature, table.temperaturePoints);
        }

        if (!table.humidityPoints.empty())
        {
            data.humidity = applyCalibrationWithInterpolation(data.humidity, table.humidityPoints);
        }

        if (!table.ecPoints.empty())
        {
            data.ec = applyCalibrationWithInterpolation(data.ec, table.ecPoints);
        }

        if (!table.phPoints.empty())
        {
            data.ph = applyCalibrationWithInterpolation(data.ph, table.phPoints);
        }

        if (!table.nitrogenPoints.empty())
        {
            data.nitrogen = applyCalibrationWithInterpolation(data.nitrogen, table.nitrogenPoints);
        }

        if (!table.phosphorusPoints.empty())
        {
            data.phosphorus = applyCalibrationWithInterpolation(data.phosphorus, table.phosphorusPoints);
        }

        if (!table.potassiumPoints.empty())
        {
            data.potassium = applyCalibrationWithInterpolation(data.potassium, table.potassiumPoints);
        }
    }
    else
    {
        // Убираем старую логику - просто оставляем исходные значения
        // data.temperature = CalibrationManager::applyCalibration(data.temperature, profile);
        // data.humidity = CalibrationManager::applyCalibration(data.humidity, profile);
        // data.ec = CalibrationManager::applyCalibration(data.ec, profile);
        // data.ph = CalibrationManager::applyCalibration(data.ph, profile);
        // data.nitrogen = CalibrationManager::applyCalibration(data.nitrogen, profile);
        // data.phosphorus = CalibrationManager::applyCalibration(data.phosphorus, profile);
        // data.potassium = CalibrationManager::applyCalibration(data.potassium, profile);
        
        // Просто оставляем исходные значения без изменений
        logDebugSafe("SensorCalibrationService: Используем исходные значения (калибровка не настроена)");
    }

    logDebugSafe("SensorCalibrationService: Калибровка применена");
}

float SensorCalibrationService::applySingleCalibration(float rawValue, SoilProfile profile)
{
    if (hasCalibrationTable(profile))
    {
        const CalibrationTable& table = getCalibrationTables()[profile];
        // Для простоты используем первую доступную таблицу
        if (!table.temperaturePoints.empty())
        {
            return applyCalibrationWithInterpolation(rawValue, table.temperaturePoints);
        }
    }

    // Fallback - просто возвращаем исходное значение
    return rawValue;
}

bool SensorCalibrationService::validateCalibrationData(
    const SensorData& data)  // NOLINT(readability-convert-member-functions-to-static)
{
    auto result = validateFullSensorData(data);
    if (!result.isValid)
    {
        logSensorValidationResult(result, "sensor_calibration_service");
        return false;
    }
    return true;
}

bool SensorCalibrationService::loadCalibrationTable(const String& csvData, SoilProfile profile)
{  // NOLINT(readability-convert-member-functions-to-static)
    logDebugSafe("SensorCalibrationService: Загрузка калибровочной таблицы для профиля %d", static_cast<int>(profile));

    CalibrationTable table;  // NOLINT(misc-const-correctness)
    if (parseCalibrationCSV(csvData, table))
    {
        getCalibrationTables()[profile] = table;
        logDebugSafe("SensorCalibrationService: Таблица загружена успешно");
        return true;
    }

    logDebugSafe("SensorCalibrationService: Ошибка загрузки таблицы");
    return false;
}

bool SensorCalibrationService::hasCalibrationTable(SoilProfile profile) const
{
    const auto iter = getCalibrationTables().find(profile);
    return iter != getCalibrationTables().end() && iter->second.isValid;
}

void SensorCalibrationService::clearCalibrationTable(SoilProfile profile)
{
    auto iter = getCalibrationTables().find(profile);
    if (iter != getCalibrationTables().end())
    {
        getCalibrationTables().erase(iter);
        logDebugSafe("SensorCalibrationService: Таблица для профиля %d очищена", static_cast<int>(profile));
    }
}

size_t SensorCalibrationService::getCalibrationPointsCount(SoilProfile profile, const String& sensorType)
{
    auto tableIter = getCalibrationTables().find(profile);
    if (tableIter == getCalibrationTables().end())
    {
        return 0;
    }

    const CalibrationTable& table = tableIter->second;
    if (sensorType == "temperature")
    {
        return table.temperaturePoints.size();
    }
    if (sensorType == "humidity")
    {
        return table.humidityPoints.size();
    }
    if (sensorType == "ec")
    {
        return table.ecPoints.size();
    }
    if (sensorType == "ph")
    {
        return table.phPoints.size();
    }
    if (sensorType == "nitrogen")
    {
        return table.nitrogenPoints.size();
    }
    if (sensorType == "phosphorus")
    {
        return table.phosphorusPoints.size();
    }
    if (sensorType == "potassium")
    {
        return table.potassiumPoints.size();
    }

    return 0;
}

String SensorCalibrationService::exportCalibrationTable(SoilProfile profile)
{  // NOLINT(readability-convert-member-functions-to-static)
    auto tableIter = getCalibrationTables().find(profile);
    if (tableIter == getCalibrationTables().end())
    {
        return "";
    }

    const CalibrationTable& table = tableIter->second;
    String csv = "sensor_type,raw_value,reference_value\n";  // NOLINT(misc-const-correctness)

    // Экспортируем все точки калибровки с использованием sprintf для избежания временных String объектов
    char buffer[64];
    
    for (const auto& point : table.temperaturePoints)
    {
        snprintf(buffer, sizeof(buffer), "temperature,%.2f,%.2f\n", point.rawValue, point.referenceValue);
        csv += buffer;
    }
    for (const auto& point : table.humidityPoints)
    {
        snprintf(buffer, sizeof(buffer), "humidity,%.2f,%.2f\n", point.rawValue, point.referenceValue);
        csv += buffer;
    }
    for (const auto& point : table.ecPoints)
    {
        snprintf(buffer, sizeof(buffer), "ec,%.2f,%.2f\n", point.rawValue, point.referenceValue);
        csv += buffer;
    }
    for (const auto& point : table.phPoints)
    {
        snprintf(buffer, sizeof(buffer), "ph,%.2f,%.2f\n", point.rawValue, point.referenceValue);
        csv += buffer;
    }
    for (const auto& point : table.nitrogenPoints)
    {
        snprintf(buffer, sizeof(buffer), "nitrogen,%.2f,%.2f\n", point.rawValue, point.referenceValue);
        csv += buffer;
    }
    for (const auto& point : table.phosphorusPoints)
    {
        snprintf(buffer, sizeof(buffer), "phosphorus,%.2f,%.2f\n", point.rawValue, point.referenceValue);
        csv += buffer;
    }
    for (const auto& point : table.potassiumPoints)
    {
        snprintf(buffer, sizeof(buffer), "potassium,%.2f,%.2f\n", point.rawValue, point.referenceValue);
        csv += buffer;
    }

    return csv;
}

// Старые методы удалены - используются новые реализации ниже

float SensorCalibrationService::applyCalibrationWithInterpolation(
    float rawValue,
    const std::vector<CalibrationPoint>& points) const  // NOLINT(readability-convert-member-functions-to-static)
{
    if (points.empty())
    {
        return rawValue;
    }

    if (points.size() == 1)
    {
        return points[0].referenceValue;
    }

    // Находим ближайшие точки для интерполяции
    for (size_t i = 0; i < points.size() - 1; ++i)
    {
        float raw1 = points[i].rawValue;
        float ref1 = points[i].referenceValue;
        float raw2 = points[i + 1].rawValue;
        float ref2 = points[i + 1].referenceValue;
        if (rawValue >= raw1 && rawValue <= raw2)
        {
            return linearInterpolation(rawValue, raw1, ref1, raw2, ref2);
        }
    }

    // Если значение вне диапазона, используем экстраполяцию
    if (rawValue < points[0].rawValue)
    {
        return points[0].referenceValue;
    }
    return points[points.size() - 1].referenceValue;
}

float SensorCalibrationService::linearInterpolation(
    float value, float x1_coord, float y1_coord, float x2_coord,
    float y2_coord) const  // NOLINT(readability-convert-member-functions-to-static)
{
    if (x2_coord == x1_coord)
    {
        return y1_coord;
    }
    return y1_coord + (((y2_coord - y1_coord) * (value - x1_coord)) / (x2_coord - x1_coord));
}

bool SensorCalibrationService::parseCalibrationCSV(const String& csvData, CalibrationTable& table)
{  // NOLINT(readability-convert-member-functions-to-static)
    // Простая реализация парсинга CSV
    // В реальной реализации здесь был бы более сложный парсер

    // Разбиваем на строки
    int startPos = 0;
    int endPos = csvData.indexOf('\n');

    // Пропускаем заголовок
    if (endPos != -1)
    {
        startPos = endPos + 1;
        endPos = csvData.indexOf('\n', startPos);
    }

    while (endPos != -1)
    {
        const String line = csvData.substring(startPos, endPos);
        if (line.length() > 0)
        {
            // Простой парсинг: sensor_type,raw_value,reference_value
            const int comma1 = line.indexOf(',');
            const int comma2 = line.indexOf(',', comma1 + 1);

            if (comma1 != -1 && comma2 != -1)
            {
                const String sensorType = line.substring(0, comma1);
                const float rawValue = line.substring(comma1 + 1, comma2).toFloat();
                const float referenceValue = line.substring(comma2 + 1).toFloat();

                CalibrationPoint point(rawValue, referenceValue);

                if (sensorType == "temperature")
                {
                    table.temperaturePoints.push_back(point);
                }
                else if (sensorType == "humidity")
                {
                    table.humidityPoints.push_back(point);
                }
                else if (sensorType == "ec")
                {
                    table.ecPoints.push_back(point);
                }
                else if (sensorType == "ph")
                {
                    table.phPoints.push_back(point);
                }
                else if (sensorType == "nitrogen")
                {
                    table.nitrogenPoints.push_back(point);
                }
                else if (sensorType == "phosphorus")
                {
                    table.phosphorusPoints.push_back(point);
                }
                else if (sensorType == "potassium")
                {
                    table.potassiumPoints.push_back(point);
                }
            }
        }

        startPos = endPos + 1;
        endPos = csvData.indexOf('\n', startPos);
    }

    // Проверяем валидность таблицы
    table.isValid = (!table.temperaturePoints.empty()) || (!table.humidityPoints.empty()) ||
                    (!table.ecPoints.empty()) || (!table.phPoints.empty()) || (!table.nitrogenPoints.empty()) ||
                    (!table.phosphorusPoints.empty()) || (!table.potassiumPoints.empty());

    return table.isValid;
}

bool SensorCalibrationService::validateCalibrationPoints(
    const std::vector<CalibrationPoint>& points) const  // NOLINT(readability-convert-member-functions-to-static)
{
    if (points.empty())
    {
        return true;
    }

    // Проверяем, что точки отсортированы по возрастанию
    for (size_t i = 1; i < points.size(); ++i)
    {
        if (points[i].rawValue <= points[i - 1].rawValue)
        {
            return false;
        }
    }

    return true;
}

// ========== НОВЫЕ МЕТОДЫ ДЛЯ СОВРЕМЕННОЙ СИСТЕМЫ КАЛИБРОВКИ ==========

#include "../../include/calibration_data.h"
#include "../../include/calibration_storage.h"
#include "../../include/calibration_math.h"

// Глобальные данные для новой системы калибровки
static SensorCalibrationData g_modern_calibration_data;
static CalibrationStorage g_calibration_storage;
static bool g_modern_calibration_initialized = false;

/**
 * @brief Инициализация современной системы калибровки
 */
bool initModernCalibrationSystem() {
    if (g_modern_calibration_initialized) {
        return true;
    }
    
    logInfo("Инициализация современной системы калибровки...");
    
    // Инициализируем хранилище
    if (!g_calibration_storage.init()) {
        logError("Ошибка инициализации хранилища калибровки");
        return false;
    }
    
    // Загружаем сохраненные данные калибровки
    if (g_calibration_storage.hasData()) {
        if (g_calibration_storage.load(g_modern_calibration_data)) {
            logSuccess("Калибровочные данные загружены из хранилища");
        } else {
            logWarn("Ошибка загрузки калибровочных данных, используем значения по умолчанию");
            g_modern_calibration_data = SensorCalibrationData();
        }
    } else {
        logInfo("Калибровочные данные не найдены, используем значения по умолчанию");
        g_modern_calibration_data = SensorCalibrationData();
    }
    
    g_modern_calibration_initialized = true;
    logSuccess("Современная система калибровки инициализирована");
    return true;
}

/**
 * @brief Сохранение современных калибровочных данных
 */
bool saveModernCalibrationData() {
    if (!g_modern_calibration_initialized) {
        logError("Современная система калибровки не инициализирована");
        return false;
    }
    
    g_modern_calibration_data.touch();
    
    if (g_calibration_storage.save(g_modern_calibration_data)) {
        logSuccess("Современные калибровочные данные сохранены");
        return true;
    } else {
        logError("Ошибка сохранения современных калибровочных данных");
        return false;
    }
}

bool SensorCalibrationService::addTemperatureCalibrationPoint(float expected, float measured) {
    if (!initModernCalibrationSystem()) {
        return false;
    }
    
    // Валидация входных данных
    if (!CalibrationMath::validateCalibrationPoint(expected, measured, "temperature")) {
        logWarnSafe("Невалидная точка температуры: expected=%.1f, measured=%.1f", expected, measured);
        return false;
    }
    
    // Рассчитываем offset
    float offset = expected - measured;
    g_modern_calibration_data.temperature.offset = offset;
    g_modern_calibration_data.temperature.is_valid = true;
    g_modern_calibration_data.temperature.timestamp = millis();
    
    logInfoSafe("Установлен offset температуры: %.1f°C", offset);
    
    // Сохраняем данные
    return saveModernCalibrationData();
}

bool SensorCalibrationService::addHumidityCalibrationPoint(float expected, float measured) {
    if (!initModernCalibrationSystem()) {
        return false;
    }
    
    // Валидация входных данных
    if (!CalibrationMath::validateCalibrationPoint(expected, measured, "humidity")) {
        logWarnSafe("Невалидная точка влажности: expected=%.1f, measured=%.1f", expected, measured);
        return false;
    }
    
    // Рассчитываем offset
    float offset = expected - measured;
    g_modern_calibration_data.humidity.offset = offset;
    g_modern_calibration_data.humidity.is_valid = true;
    g_modern_calibration_data.humidity.timestamp = millis();
    
    logInfoSafe("Установлен offset влажности: %.1f%%", offset);
    
    // Сохраняем данные
    return saveModernCalibrationData();
}

bool SensorCalibrationService::addPHCalibrationPoint(float expected, float measured) {
    if (!initModernCalibrationSystem()) {
        return false;
    }
    
    // Валидация входных данных
    if (!CalibrationMath::validateCalibrationPoint(expected, measured, "ph")) {
        logWarnSafe("Невалидная точка pH калибровки: expected=%.1f, measured=%.1f", expected, measured);
        return false;
    }
    
    // Добавляем точку
    ModernCalibrationPoint point(expected, measured);
    g_modern_calibration_data.ph.points.push_back(point);
    
    logInfoSafe("Добавлена точка pH калибровки: expected=%.1f, measured=%.1f", expected, measured);
    
    // Сохраняем данные
    return saveModernCalibrationData();
}

bool SensorCalibrationService::addECCalibrationPoint(float expected, float measured) {
    if (!initModernCalibrationSystem()) {
        return false;
    }
    
    // Валидация входных данных
    if (!CalibrationMath::validateCalibrationPoint(expected, measured, "ec")) {
        logWarnSafe("Невалидная точка EC калибровки: expected=%.0f, measured=%.0f", expected, measured);
        return false;
    }
    
    // Добавляем точку
    ModernCalibrationPoint point(expected, measured);
    g_modern_calibration_data.ec.points.push_back(point);
    
    logInfoSafe("Добавлена точка EC калибровки: expected=%.0f, measured=%.0f", expected, measured);
    
    // Сохраняем данные
    return saveModernCalibrationData();
}

bool SensorCalibrationService::setNPKCalibrationPoint(float nitrogen, float phosphorus, float potassium) {
    if (!initModernCalibrationSystem()) {
        return false;
    }
    
    // Валидация входных данных
    if (!CalibrationMath::validateCalibrationPoint(0.0f, nitrogen, "npk") ||
        !CalibrationMath::validateCalibrationPoint(0.0f, phosphorus, "npk") ||
        !CalibrationMath::validateCalibrationPoint(0.0f, potassium, "npk")) {
        logWarnSafe("Невалидные значения NPK: N=%.0f, P=%.0f, K=%.0f", nitrogen, phosphorus, potassium);
        return false;
    }
    
    // Устанавливаем нулевую точку (дистиллированная вода)
    g_modern_calibration_data.npk.setZeroPoint(nitrogen, phosphorus, potassium);
    
    logInfoSafe("Установлена нулевая точка NPK: N=%.0f, P=%.0f, K=%.0f", nitrogen, phosphorus, potassium);
    
    // Сохраняем данные
    return saveModernCalibrationData();
}

bool SensorCalibrationService::calculatePHCalibration() {
    if (!initModernCalibrationSystem()) {
        return false;
    }
    
    if (g_modern_calibration_data.ph.points.size() < 2) {
        logWarn("Недостаточно точек для расчета pH калибровки (нужно минимум 2)");
        return false;
    }
    
    // Удаляем выбросы
    CalibrationMath::removeOutliers(g_modern_calibration_data.ph.points);
    
    if (g_modern_calibration_data.ph.points.size() < 2) {
        logWarn("После удаления выбросов осталось недостаточно точек pH");
        return false;
    }
    
    // Рассчитываем коэффициенты
    bool success = CalibrationMath::calculateLinearRegression(
        g_modern_calibration_data.ph.points, 
        g_modern_calibration_data.ph.coefficients
    );
    
    if (success) {
        logSuccess("pH калибровка рассчитана: slope=" + String(g_modern_calibration_data.ph.coefficients.slope, 4) + 
                   ", intercept=" + String(g_modern_calibration_data.ph.coefficients.intercept, 4) + 
                   ", R²=" + String(g_modern_calibration_data.ph.coefficients.r_squared, 4));
        
        // Сохраняем данные
        return saveModernCalibrationData();
    } else {
        logError("Ошибка расчета pH калибровки");
        return false;
    }
}

bool SensorCalibrationService::calculateECCalibration() {
    if (!initModernCalibrationSystem()) {
        return false;
    }
    
    if (g_modern_calibration_data.ec.points.size() < 2) {
        logWarn("Недостаточно точек для расчета EC калибровки (нужно минимум 2)");
        return false;
    }
    
    // Удаляем выбросы
    CalibrationMath::removeOutliers(g_modern_calibration_data.ec.points);
    
    if (g_modern_calibration_data.ec.points.size() < 2) {
        logWarn("После удаления выбросов осталось недостаточно точек EC");
        return false;
    }
    
    // Рассчитываем коэффициенты
    bool success = CalibrationMath::calculateLinearRegression(
        g_modern_calibration_data.ec.points, 
        g_modern_calibration_data.ec.coefficients
    );
    
    if (success) {
        logSuccess("EC калибровка рассчитана: slope=" + String(g_modern_calibration_data.ec.coefficients.slope, 4) + 
                   ", intercept=" + String(g_modern_calibration_data.ec.coefficients.intercept, 4) + 
                   ", R²=" + String(g_modern_calibration_data.ec.coefficients.r_squared, 4));
        
        // Сохраняем данные
        return saveModernCalibrationData();
    } else {
        logError("Ошибка расчета EC калибровки");
        return false;
    }
}

bool SensorCalibrationService::setTemperatureOffset(float offset) {
    if (!initModernCalibrationSystem()) {
        return false;
    }
    
    // Валидация offset
    if (abs(offset) > 10.0f) {
        logWarn("Слишком большой offset температуры: " + String(offset));
        return false;
    }
    
    g_modern_calibration_data.temperature.offset = offset;
    g_modern_calibration_data.temperature.is_valid = true;
    g_modern_calibration_data.temperature.timestamp = millis();
    
    logInfo("Установлен offset температуры: " + String(offset) + "°C");
    
    // Сохраняем данные
    return saveModernCalibrationData();
}

bool SensorCalibrationService::setHumidityOffset(float offset) {
    if (!initModernCalibrationSystem()) {
        return false;
    }
    
    // Валидация offset
    if (abs(offset) > 20.0f) {
        logWarn("Слишком большой offset влажности: " + String(offset));
        return false;
    }
    
    g_modern_calibration_data.humidity.offset = offset;
    g_modern_calibration_data.humidity.is_valid = true;
    g_modern_calibration_data.humidity.timestamp = millis();
    
    logInfo("Установлен offset влажности: " + String(offset) + "%");
    
    // Сохраняем данные
    return saveModernCalibrationData();
}

void SensorCalibrationService::resetCalibration() {
    if (!initModernCalibrationSystem()) {
        return;
    }
    
    logInfo("Сброс всех современных калибровочных данных");
    
    g_modern_calibration_data.reset();
    g_calibration_storage.clear();
    
    saveModernCalibrationData();
    
    logSuccess("Современные калибровочные данные сброшены");
}

String SensorCalibrationService::exportCalibrationToJSON() {
    if (!initModernCalibrationSystem()) {
        return "{}";
    }
    
    return g_modern_calibration_data.toJSON();
}

bool SensorCalibrationService::importCalibrationFromJSON(const String& json_data) {
    if (!initModernCalibrationSystem()) {
        return false;
    }
    
    SensorCalibrationData temp_data;
    if (temp_data.fromJSON(json_data)) {
        g_modern_calibration_data = temp_data;
        logSuccess("Калибровочные данные импортированы из JSON");
        return saveModernCalibrationData();
    } else {
        logError("Ошибка импорта калибровочных данных из JSON");
        return false;
    }
}

String SensorCalibrationService::getCalibrationStatus() const {
    // Инициализируем систему при первом обращении
    if (!g_modern_calibration_initialized) {
        initModernCalibrationSystem();
    }
    
    if (!g_modern_calibration_initialized) {
        return "{\"error\":\"Ошибка инициализации системы калибровки\"}";
    }
    
    DynamicJsonDocument doc(1024);
    
    // Общий статус
    doc["is_complete"] = g_modern_calibration_data.isComplete();
    doc["device_id"] = g_modern_calibration_data.device_id;
    doc["updated_at"] = g_modern_calibration_data.updated_at;
    
    // Температура
    JsonObject temp = doc.createNestedObject("temperature");
    temp["is_valid"] = g_modern_calibration_data.temperature.is_valid;
    temp["offset"] = g_modern_calibration_data.temperature.offset;
    temp["status"] = g_modern_calibration_data.temperature.is_valid ? "✅ Готова" : "❌ Не настроена";
    
    // Влажность
    JsonObject hum = doc.createNestedObject("humidity");
    hum["is_valid"] = g_modern_calibration_data.humidity.is_valid;
    hum["offset"] = g_modern_calibration_data.humidity.offset;
    hum["status"] = g_modern_calibration_data.humidity.is_valid ? "✅ Готова" : "❌ Не настроена";
    
    // pH
    JsonObject ph = doc.createNestedObject("ph");
    ph["point_count"] = g_modern_calibration_data.ph.getPointCount();
    ph["is_ready"] = g_modern_calibration_data.ph.isReady();
    ph["r_squared"] = g_modern_calibration_data.ph.coefficients.r_squared;
    ph["status"] = g_modern_calibration_data.ph.isReady() ? 
        "✅ Готова" : "❌ Нужно минимум 2 точки";
    ph["point_count"] = g_modern_calibration_data.ph.getPointCount();
    
    // EC
    JsonObject ec = doc.createNestedObject("ec");
    ec["point_count"] = g_modern_calibration_data.ec.getPointCount();
    ec["is_ready"] = g_modern_calibration_data.ec.isReady();
    ec["r_squared"] = g_modern_calibration_data.ec.coefficients.r_squared;
    ec["status"] = g_modern_calibration_data.ec.isReady() ? 
        "✅ Готова" : "❌ Нужно минимум 2 точки";
    
    // NPK
    JsonObject npk = doc.createNestedObject("npk");
    npk["is_ready"] = g_modern_calibration_data.npk.isReady();
    npk["nitrogen_offset"] = g_modern_calibration_data.npk.nitrogen.offset;
    npk["phosphorus_offset"] = g_modern_calibration_data.npk.phosphorus.offset;
    npk["potassium_offset"] = g_modern_calibration_data.npk.potassium.offset;
    npk["status"] = g_modern_calibration_data.npk.isReady() ? 
        "✅ Готова (нулевая точка установлена)" : 
        "❌ Нулевая точка не установлена";
    
    String result;
    serializeJson(doc, result);
    return result;
}

bool SensorCalibrationService::setNPKCoefficients(float nSlope, float nOffset, float pSlope, float pOffset, float kSlope, float kOffset) {
    if (!initModernCalibrationSystem()) {
        return false;
    }
    
    logInfoSafe("Установка NPK коэффициентов: N(%.3f,%.1f) P(%.3f,%.1f) K(%.3f,%.1f)", 
                nSlope, nOffset, pSlope, pOffset, kSlope, kOffset);
    
    // Устанавливаем коэффициенты для NPK элементов
    g_modern_calibration_data.npk.nitrogen.offset = nOffset;
    g_modern_calibration_data.npk.nitrogen.is_valid = true;
    g_modern_calibration_data.npk.nitrogen.timestamp = millis();
    
    g_modern_calibration_data.npk.phosphorus.offset = pOffset;
    g_modern_calibration_data.npk.phosphorus.is_valid = true;
    g_modern_calibration_data.npk.phosphorus.timestamp = millis();
    
    g_modern_calibration_data.npk.potassium.offset = kOffset;
    g_modern_calibration_data.npk.potassium.is_valid = true;
    g_modern_calibration_data.npk.potassium.timestamp = millis();
    
    g_modern_calibration_data.npk.last_calibration = millis();
    
    logInfo("NPK коэффициенты установлены");
    
    // Сохраняем данные
    return saveModernCalibrationData();
}
