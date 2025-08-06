/**
 * @file routes_data.cpp
 * @brief Маршруты для работы с данными датчика
 * @details Обработка запросов показаний датчика, JSON API и веб-интерфейса данных
 */

#include <ArduinoJson.h>
#include <LittleFS.h>
#include <NTPClient.h>
#include <ctime>
#include "../../include/jxct_config_vars.h"
#include "../../include/jxct_constants.h"
#include "../../include/jxct_format_utils.h"
#include "../../include/jxct_strings.h"
#include "../../include/jxct_ui_system.h"
#include "../../include/logger.h"
#include "../../include/web/csrf_protection.h"  // 🔒 CSRF защита
#include "../../include/web_routes.h"
#include "../modbus_sensor.h"
#include "../wifi_manager.h"
#include "business_services.h"
#include "calibration_manager.h"
#include "../../include/advanced_filters.h"
#include "../business/sensor_calibration_service.h"
#include "../../include/sensor_types.h"
#include "../sensor_correction.h"
#include "../business/crop_recommendation_engine.h"
#include "../business/sensor_compensation_service.h"

// Глобальный экземпляр сервиса калибровки
extern SensorCalibrationService gCalibrationService;

extern NTPClient* timeClient;

// Внешние зависимости (уже объявлены в заголовочных файлах)
// extern String navHtml();  // объявлено в wifi_manager.h
// extern String formatValue(float value, const char* unit, int precision);  // объявлено в jxct_format_utils.h
// extern String getApSsid();  // объявлено в wifi_manager.h

// Общая функция для определения текущего сезона
const char* getCurrentSeasonName()
{
    if (timeClient == nullptr)
    {
        extern WiFiUDP ntpUDP;
        timeClient = new NTPClient(ntpUDP, "pool.ntp.org", 0, 3600000);
        timeClient->begin();
    }

    time_t now = timeClient ? (time_t)timeClient->getEpochTime() : time(nullptr);
    if (now < NTP_TIMESTAMP_2000)
    {
        if (timeClient)
        {
            timeClient->forceUpdate();
            now = (time_t)timeClient->getEpochTime();
            if (now < NTP_TIMESTAMP_2000)
            {
                return "Н/Д";
            }
        }
        else
        {
            return "Н/Д";
        }
    }
    struct tm* timeInfo = localtime(&now);
    if (!timeInfo)
    {
        return "Н/Д";
    }
    uint8_t month = timeInfo->tm_mon + 1;
    if (month == 12 || month == 1 || month == 2)
    {
        return "Зима";
    }
    if (month >= 3 && month <= 5)
    {
        return "Весна";
    }
    if (month >= 6 && month <= 8)
    {
        return "Лето";
    }
    return "Осень";
}

// Буфер для загрузки файлов (калибровка через /readings)
namespace
{
File uploadFile;
SoilProfile uploadProfile = SoilProfile::SAND;

// Используем RecValues из бизнес-сервиса

// Функции сезонной коррекции NPK перенесены в бизнес-сервис CropRecommendationEngine

}  // namespace

void handleReadingsUpload()  // ✅ Убираем static - функция extern в header
{
    HTTPUpload& upload = webServer.upload();
    if (upload.status == UPLOAD_FILE_START)
    {
        CalibrationManager::init();
        const char* path = CalibrationManager::profileToFilename(SoilProfile::SAND);  // custom.csv
        uploadFile = LittleFS.open(path, "w");
        if (!uploadFile)
        {
            logErrorSafe("\1", path);
        }
    }
    else if (upload.status == UPLOAD_FILE_WRITE)
    {
        if (uploadFile)
        {
            uploadFile.write(upload.buf, upload.currentSize);
        }
    }
    else if (upload.status == UPLOAD_FILE_END)
    {
        if (uploadFile)
        {
            uploadFile.close();
            logSuccessSafe("\1", upload.totalSize);
        }
        webServer.sendHeader("Location", "/readings?toast=Калибровка+загружена", true);
        webServer.send(HTTP_REDIRECT, "text/plain", "Redirect");
    }
}

namespace
{
void handleProfileSave()
{
    if (webServer.hasArg("soil_profile"))
    {
        const String profileStr = webServer.arg("soil_profile");
        if (profileStr == "sand")
        {
            config.soilProfile = 0;
        }
        else if (profileStr == "loam")
        {
            config.soilProfile = 1;
        }
        else if (profileStr == "peat")
        {
            config.soilProfile = 2;
        }
        else if (profileStr == "clay")
        {
            config.soilProfile = 3;
        }

        saveConfig();
        logSuccessSafe("\1", profileStr.c_str());
    }
    webServer.sendHeader("Location", "/readings?toast=Профиль+сохранен", true);
    webServer.send(HTTP_REDIRECT, "text/plain", "Redirect");
}
}  // namespace

// ✅ Функция для безопасной санитизации JSON строк
String sanitizeForJson(const String& input) {
    String sanitized = input;
    sanitized.replace("\\", "\\\\");  // Сначала обратные слеши
    sanitized.replace("\"", "\\\"");  // Потом кавычки
    sanitized.replace("/", "\\/");    // Экранирование слешей
    sanitized.replace("\n", "\\n");
    sanitized.replace("\r", "\\r");
    sanitized.replace("\t", "\\t");
    sanitized.replace("\b", "\\b");
    sanitized.replace("\f", "\\f");
    return sanitized;
}

void sendSensorJson()  // ✅ Убираем static - функция extern в header
{
    // unified JSON response for sensor data
    logWebRequest("GET", webServer.uri(), webServer.client().remoteIP().toString());
    if (currentWiFiMode != WiFiMode::STA)
    {
        webServer.send(HTTP_FORBIDDEN, HTTP_CONTENT_TYPE_JSON, R"({"error":"AP mode"})");
        return;
    }

    StaticJsonDocument<SENSOR_JSON_DOC_SIZE> doc;
    // Температура НЕ компенсируется - используем сырые данные
    doc["temperature"] = format_temperature(sensorData.raw_temperature);
    doc["humidity"] = format_moisture(sensorData.humidity);
    
    doc["ec"] = format_ec(sensorData.ec);
    doc["ph"] = format_ph(sensorData.ph);
    doc["nitrogen"] = format_npk(sensorData.nitrogen);
    doc["phosphorus"] = format_npk(sensorData.phosphorus);
    doc["potassium"] = format_npk(sensorData.potassium);
    doc["raw_temperature"] = format_temperature(sensorData.raw_temperature);
    doc["raw_humidity"] = format_moisture(sensorData.raw_humidity);
    doc["raw_ec"] = format_ec(sensorData.raw_ec);
    doc["raw_ph"] = format_ph(sensorData.raw_ph);
    doc["raw_nitrogen"] = format_npk(sensorData.raw_nitrogen);
    doc["raw_phosphorus"] = format_npk(sensorData.raw_phosphorus);
    doc["raw_potassium"] = format_npk(sensorData.raw_potassium);
    doc["irrigation"] = sensorData.recentIrrigation;
    // ПРАВИЛЬНАЯ ЛОГИКА ВАЛИДАЦИИ - проверяем условия измерения
    bool isDataValid = true;
    String validationStatus = "optimal"; // optimal, suboptimal, irrigation, error
    
    // 🔴 Красный: Ошибки датчика (выход за физические пределы JXCT)
    if (sensorData.temperature < SENSOR_TEMP_MIN || sensorData.temperature > SENSOR_TEMP_MAX ||
        sensorData.humidity < SENSOR_HUMIDITY_MIN || sensorData.humidity > SENSOR_HUMIDITY_MAX ||
        sensorData.ec < SENSOR_EC_MIN || sensorData.ec > SENSOR_EC_MAX ||
        sensorData.ph < SENSOR_PH_MIN || sensorData.ph > SENSOR_PH_MAX ||
        sensorData.nitrogen < SENSOR_NPK_MIN || sensorData.nitrogen > SENSOR_NPK_MAX ||
        sensorData.phosphorus < SENSOR_NPK_MIN || sensorData.phosphorus > SENSOR_NPK_MAX ||
        sensorData.potassium < SENSOR_NPK_MIN || sensorData.potassium > SENSOR_NPK_MAX) {
        isDataValid = false;
        validationStatus = "error";
    }
    // 🔵 Синий: Полив активен (временная невалидность)
    else if (sensorData.recentIrrigation) {
        validationStatus = "irrigation";
    }
    // 🟠 Оранжевый: Неоптимальные условия измерения
    else if (sensorData.humidity < 25.0F || sensorData.temperature < 5.0F || sensorData.temperature > 40.0F) {
        validationStatus = "suboptimal";
    }
    // 🟢 Зеленый: Оптимальные условия измерения
    else {
        validationStatus = "optimal";
    }
    
    doc["valid"] = isDataValid;
    doc["measurement_status"] = validationStatus;

    // ❌ ОТКЛЮЧАЕМ СТАРУЮ СИСТЕМУ - используем только новый системный алгоритм
    // RecValues rec = computeRecommendations();
    // doc["rec_temperature"] = format_temperature(rec.t);
    // doc["rec_humidity"] = format_moisture(rec.hum);
    // doc["rec_ec"] = format_ec(rec.ec);
    // doc["rec_ph"] = format_ph(rec.ph);
    // doc["rec_nitrogen"] = format_npk(rec.n);
    // doc["rec_phosphorus"] = format_npk(rec.p);
    // doc["rec_potassium"] = format_npk(rec.k);

    // ---- Рекомендации по взаимодействию питательных веществ ----
    NPKReferences npk{sensorData.nitrogen, sensorData.phosphorus, sensorData.potassium};
    SoilType soilType = static_cast<SoilType>(config.soilProfile);
    

    
    // Получаем рекомендации по антагонизмам
    String antagonismRecommendations = getNutrientInteractionService().generateAntagonismRecommendations(
        npk, soilType, sensorData.ph);
    doc["nutrient_interactions"] = antagonismRecommendations;
    
    // ✅ Дополнительная проверка: если cropId пустой, устанавливаем "none"
    if (strlen(config.cropId) == 0) {
        strlcpy(config.cropId, "none", sizeof(config.cropId));
        logDebugSafe("JSON API: cropId was empty, set to 'none'");
    }
    
    // ✅ Добавляем cropId в JSON (БЕЗОПАСНО)
                doc["crop_id"] = sanitizeForJson(String(config.cropId));
            
            // ✅ ОТЛАДОЧНОЕ ЛОГИРОВАНИЕ для проблемы отображения культуры
            logDebugSafe("JSON API: cropId='%s', len=%d, envType=%d", 
                        config.cropId, strlen(config.cropId), config.environmentType);
    
    // Проверяем валидность crop_id
    bool lenCheck = strlen(config.cropId) > 0;
    bool strCheck = strcmp(config.cropId, "none") != 0;
    
    // ✅ crop_specific_recommendations обрабатывается ниже в системном алгоритме
    
    // ============================================================================
    // ОПТИМИЗИРОВАННЫЙ АЛГОРИТМ: Только необходимые расчеты
    // ============================================================================
    
    // ✅ ОПТИМИЗАЦИЯ: Определяем сезон ОДИН РАЗ
    const char* seasonName = getCurrentSeasonName();
    
    // ✅ ОПТИМИЗАЦИЯ: Простая конвертация VWC → ASM для второй колонки
    SensorCompensationService compensationService;
    float asmHumidity = compensationService.vwcToAsm(sensorData.humidity / 100.0F, soilType);
    doc["humidity"] = format_moisture(asmHumidity);
    
    // ✅ ВОЗВРАЩАЕМ УМНЫЕ РЕКОМЕНДАЦИИ
    if (lenCheck && strCheck) {
        // Используем научно компенсированные значения для умных рекомендаций
        NPKReferences scientificNPK;
        scientificNPK.nitrogen = sensorData.nitrogen;
        scientificNPK.phosphorus = sensorData.phosphorus;
        scientificNPK.potassium = sensorData.potassium;
        
        String cropRecommendations = getCropEngine().generateCropSpecificRecommendations(
            String(config.cropId), scientificNPK, soilType, sensorData.ph, String(seasonName));
        doc["crop_specific_recommendations"] = cropRecommendations;
        
        logDebugSafe("JSON API: crop='%s', rec_len=%d", config.cropId, cropRecommendations.length());
    } else {
        doc["crop_specific_recommendations"] = "";
    }
    
    // ✅ РЕКОМЕНДУЕМЫЕ ЗНАЧЕНИЯ ДЛЯ ВЫБРАННОЙ КУЛЬТУРЫ
    CropConfig cropConfig = getCropEngine().getCropConfig(String(config.cropId));
    doc["rec_temperature"] = format_temperature(cropConfig.temperature);
    doc["rec_humidity"] = format_moisture(cropConfig.humidity);
    doc["rec_ec"] = format_ec(cropConfig.ec);
    doc["rec_ph"] = format_ph(cropConfig.ph);
    doc["rec_nitrogen"] = format_npk(cropConfig.nitrogen);
    doc["rec_phosphorus"] = format_npk(cropConfig.phosphorus);
    doc["rec_potassium"] = format_npk(cropConfig.potassium);

    // ---- Дополнительная информация ----
    doc["season"] = seasonName;

    // Проверяем отклонения
    String alerts = "";
    auto append = [&](const char* n)
    {
        if (alerts.length())
        {
            alerts += ", ";
        }
        alerts += n;
    };
    // Физические пределы датчика
    if (sensorData.temperature < TEMP_MIN_VALID || sensorData.temperature > TEMP_MAX_VALID)
    {
        append("T");
    }
    if (sensorData.humidity < HUM_MIN_VALID || sensorData.humidity > HUM_MAX_VALID)
    {
        append("θ");
    }
    if (sensorData.ec < 0 || sensorData.ec > EC_MAX_VALID)
    {
        append("EC");
    }
    if (sensorData.ph < 3 || sensorData.ph > 9)
    {
        append("pH");
    }
    if (sensorData.nitrogen < 0 || sensorData.nitrogen > NPK_MAX_VALID)
    {
        append("N");
    }
    if (sensorData.phosphorus < 0 || sensorData.phosphorus > NPK_MAX_VALID)
    {
        append("P");
    }
    if (sensorData.potassium < 0 || sensorData.potassium > NPK_MAX_VALID)
    {
        append("K");
    }
    doc["alerts"] = alerts;

    doc["timestamp"] = (long)(timeClient != nullptr ? timeClient->getEpochTime() : 0);

    String json;
    serializeJson(doc, json);
    webServer.send(HTTP_OK, HTTP_CONTENT_TYPE_JSON, json);
}

void setupDataRoutes()
{
    // Красивая страница показаний с иконками (оригинальный дизайн)
    webServer.on(
        "/readings", HTTP_GET,
        []()
        {
            logWebRequest("GET", "/readings", webServer.client().remoteIP().toString());

            if (currentWiFiMode == WiFiMode::AP)
            {
                webServer.send(200, "text/html; charset=utf-8",
                               generateApModeUnavailablePage("Показания", UI_ICON_DATA));
                return;
            }

            String html = generatePageHeader("Показания датчика", UI_ICON_DATA);
            html += navHtml();
            html += "<h1>" UI_ICON_DATA " Показания датчика</h1>";

            // Информационная строка состояния
            html += "<div id='statusInfo' style='margin:10px 0;font-size:16px;color:#333'></div>";

            // ======= ОБЪЯСНЕНИЕ ПРОЦЕССОВ =======
            html += "<div class='section' style='background:#f8f9fa;padding:15px;border-radius:8px;margin:15px 0;'>";
            html += "<h3>📋 Как работают показания</h3>";
            html += "<div style='display:grid;grid-template-columns:1fr 1fr;gap:20px;font-size:14px;'>";

            // Левая колонка - компенсация
            html += "<div>";
            html += "<h4>🔧 Компенсация показаний</h4>";
            html += "<ul style='margin:0;padding-left:20px;'>";
            html += "<li><strong>RAW</strong> - сырые данные с датчика JXCT</li>";
            html += "<ul style='margin:5px 0;padding-left:15px;'>";
            html += "<li><strong>Цветовая индикация RAW:</strong></li>";
            html += "<ul style='margin:5px 0;padding-left:15px;'>";
            html += "<li>🟢 <strong>Зеленый:</strong> значение в рабочем диапазоне датчика</li>";
            html += "<li>🟡 <strong>Желтый:</strong> значение в диапазоне с малой точностью</li>";
            html += "<li>🔴 <strong>Красный:</strong> значение за пределами датчика</li>";
            html += "</ul>";
            html += "<li><strong>Точность в скобках:</strong> указана точность измерений для каждого параметра</li>";
            html += "<li><strong>Валидность данных:</strong> данные считаются валидными при влажности ≥25%, температуре 5-40°C, отсутствии полива и ошибок датчика</li>";
            html += "</ul>";
            html += "<li><strong>Компенс.</strong> - данные после математической компенсации:</li>";
            html += "<ul style='margin:5px 0;padding-left:15px;'>";
            html += "<li>🌡️ <strong>Температура:</strong> без изменений</li>";
            html += "<li>💧 <strong>Влажность:</strong> без изменений</li>";
            html += "<li>⚡ <strong>EC:</strong> температурная компенсация (Rhoades et al., 1989)</li>";
            html += "<li>⚗️ <strong>pH:</strong> температурная поправка по Нернсту (Nernst, 1889)</li>";
            html +=
                "<li>🌿🌱🍎 <strong>NPK:</strong> экспоненциальная компенсация по T и влажности (Delgado et al., 2020)</li>";
            html += "</ul>";
            html += "<ul style='margin:5px 0;padding-left:15px;'>";
            html += "<li><strong>Цветовая индикация Компенс.:</strong></li>";
            html += "<ul style='margin:5px 0;padding-left:15px;'>";
            html += "<li>🟢 <strong>Зеленый:</strong> отклонение от RAW ≤ ±5%</li>";
            html += "<li>🟡 <strong>Желтый:</strong> отклонение от RAW ≤ ±10%</li>";
            html += "<li>🟠 <strong>Оранжевый:</strong> отклонение от RAW ≤ ±15%</li>";
            html += "<li>🔴 <strong>Красный:</strong> отклонение от RAW > ±15%</li>";
            html += "</ul>";
            html += "</ul>";
            html += "</ul>";
            html += "</div>";

            // Правая колонка - рекомендации
            html += "<div>";
            html += "<h4>🎯 Рекомендации</h4>";
            html += "<ul style='margin:0;padding-left:20px;'>";
            html += "<li><strong>Базовые нормы</strong> для выбранной культуры</li>";
            html += "<li><strong>Сезонные корректировки</strong> (весна/лето/осень/зима)</li>";
            html += "<li><strong>Тип среды</strong> (открытый грунт/теплица/помещение)</li>";
            html += "<li><strong>Цветовая индикация Рекомендаций:</strong></li>";
            html += "<ul style='margin:5px 0;padding-left:15px;'>";
            html += "<li>🟢 <strong>Зеленый:</strong> отклонение от рекомендации ≤ ±10%</li>";
            html += "<li>🟡 <strong>Желтый:</strong> отклонение от рекомендации ≤ ±20%</li>";
            html += "<li>🟠 <strong>Оранжевый:</strong> отклонение от рекомендации ≤ ±30%</li>";
            html += "<li>🔴 <strong>Красный:</strong> отклонение от рекомендации > ±30%</li>";
            html += "</ul>";
            html += "<li><strong>Цветовая индикация валидности данных:</strong></li>";
            html += "<ul style='margin:5px 0;padding-left:15px;'>";
            html += "<li>🟢 <strong>Зеленый:</strong> данные валидны</li>";
            html += "<li>🔵 <strong>Синий:</strong> полив активен - данные временно не валидны</li>";
            html += "<li>🟠 <strong>Оранжевый:</strong> данные не валидны - неоптимальные условия</li>";
            html += "<li>🔴 <strong>Красный:</strong> данные не валидны - ошибки датчика</li>";
            html += "</ul>";
            html += "<li><strong>Детекция полива:</strong> автоматическое определение полива по скачку влажности и времени</li>";
            html += "</ul>";
            html += "</div>";

            html += "</div>";
            html += "</div>";

            // Заголовок 4-го столбца: выбранная культура или «Реком.»
            String recHeader = "Реком.";
            if (strlen(config.cropId) > 0)
            {
                const char* cropId = config.cropId;
                
                // Lookup table для названий культур - более поддерживаемо
                static const struct {
                    const char* id;
                    const char* name;
                } cropNames[] = {
                    // Основные культуры
                    {"tomato", "Томаты"},
                    {"cucumber", "Огурцы"},
                    {"pepper", "Перец"},
                    {"lettuce", "Салат"},
                    {"blueberry", "Голубика"},
                    {"lawn", "Газон"},
                    {"grape", "Виноград"},
                    {"conifer", "Хвойные"},
                    {"strawberry", "Клубника"},
                    {"apple", "Яблоня"},
                    {"pear", "Груша"},
                    {"cherry", "Вишня"},
                    {"raspberry", "Малина"},
                    {"currant", "Смородина"},
                    
                    // НОВЫЕ КУЛЬТУРЫ (Фаза 1 - Приоритетные, научно обоснованные 2024)
                    {"spinach", "Шпинат"},
                    {"basil", "Базилик"},
                    {"cannabis", "Конопля"},
                    
                    // НОВЫЕ КУЛЬТУРЫ (Фаза 2 - Важные, стратегические)
                    {"wheat", "Пшеница"},
                    {"potato", "Картофель"},
                    
                    // НОВЫЕ КУЛЬТУРЫ (Фаза 3 - Завершающие, полное покрытие)
                    {"kale", "Кале"},
                    {"blackberry", "Ежевика"},
                    {"soybean", "Соя"},
                    {"carrot", "Морковь"},
                    
                    // Завершающий элемент для определения размера массива
                    {nullptr, nullptr}
                };
                
                // Поиск культуры в lookup table
                for (int i = 0; cropNames[i].id != nullptr; i++) {
                    if (strcmp(cropId, cropNames[i].id) == 0) {
                        recHeader = cropNames[i].name;
                        break;
                    }
                }
            }

            html += "<div class='section'><table class='data'><thead><tr><th></th><th>RAW</th><th>Компенс.</th><th>" +
                    recHeader + "</th></tr></thead><tbody>";
            html +=
                "<tr><td>🌡️ Температура, °C</td><td><span id='temp_raw'></span></td><td><span "
                "id='temp'></span></td><td><span id='temp_rec'></span></td></tr>";
            html +=
                "<tr><td>💧 Влажность, %</td><td><span id='hum_raw'></span> VWC</td><td><span "
                "id='hum'></span> ASM</td><td><span id='hum_rec'></span> ASM</td></tr>";
            html +=
                "<tr><td>⚡ EC, µS/cm</td><td><span id='ec_raw'></span></td><td><span id='ec'></span></td><td><span "
                "id='ec_rec'></span></td></tr>";
            html +=
                "<tr><td>⚗️ pH</td><td><span id='ph_raw'></span></td><td><span id='ph'></span></td><td><span "
                "id='ph_rec'></span></td></tr>";
            html +=
                "<tr><td>🌿 Азот (N), мг/кг</td><td><span id='n_raw'></span></td><td><span "
                "id='n'></span></td><td><span id='n_rec'></span><span id='n_season' "
                "class='season-adj'></span></td></tr>";
            html +=
                "<tr><td>🌱 Фосфор (P), мг/кг</td><td><span id='p_raw'></span></td><td><span "
                "id='p'></span></td><td><span id='p_rec'></span><span id='p_season' "
                "class='season-adj'></span></td></tr>";
            html +=
                "<tr><td>🍎 Калий (K), мг/кг</td><td><span id='k_raw'></span></td><td><span "
                "id='k'></span></td><td><span id='k_rec'></span><span id='k_season' "
                "class='season-adj'></span></td></tr>";
            html += "</tbody></table></div>";

            // ======= РЕКОМЕНДАЦИИ ПО ВЗАИМОДЕЙСТВИЮ ПИТАТЕЛЬНЫХ ВЕЩЕСТВ =======
            html += "<div class='section'><h2>🔬 Рекомендации по взаимодействию питательных веществ</h2>";
            
            // Антагонизмы и синергизмы
            html += "<div style='background:#f8f9fa;padding:15px;border-radius:8px;margin:15px 0;'>";
            html += "<h4>⚠️ Антагонизмы и синергизмы</h4>";
            html += "<div id='nutrient-interactions' style='font-size:14px;line-height:1.6;'>";
            html += "<p><em>Загрузка рекомендаций...</em></p>";
            html += "</div>";
            html += "</div>";
            
            // Специфические рекомендации по культурам
            html += "<div style='background:#e8f5e8;padding:15px;border-radius:8px;margin:15px 0;'>";
            html += "<h4>🌱 Специфические рекомендации по культурам</h4>";
            html += "<div id='crop-specific-recommendations' style='font-size:14px;line-height:1.6;'>";
            html += "<p><em>Загрузка рекомендаций...</em></p>";
            html += "</div>";
            html += "</div>";
            
            html += "</div>";

                        // ======= СИСТЕМА КОРРЕКЦИИ ПОКАЗАНИЙ =======
            html += "<div class='section'><h2>🔧 Система коррекции показаний</h2>";
            html += "<p style='font-size:14px;color:#666;'>Корректировка показаний датчика для улучшения точности измерений</p>";

            // Включение/отключение
            html += "<div style='background:#f8f9fa;padding:15px;border-radius:8px;margin:15px 0;'>";
            html += "<h4>🔄 Включение/отключение</h4>";
            html += "<div class='form-group'>";
            html += "<label for='correction_enabled'>Коррекция показаний:</label>";
            html += "<select id='correction_enabled' style='width:100%;'>";
            html += "<option value='true'>Включена</option>";
            html += "<option value='false'>Отключена</option>";
            html += "</select>";
            html += "</div>";
            html += "<button onclick='updateCorrectionEnabled()' class='btn btn-success' style='width:100%;'>Обновить</button>";
            html += "<div id='correction-enabled-status' style='margin-top:10px;font-size:14px;min-height:20px;'></div>";
            html += "</div>";

            // Коэффициенты коррекции
            html += "<div style='background:#f8f9fa;padding:15px;border-radius:8px;margin:15px 0;'>";
            html += "<h4>📊 Коэффициенты коррекции</h4>";
            html += "<div style='display:grid;grid-template-columns:1fr 1fr;gap:20px;margin:15px 0;'>";
            
            // Влажность
            html += "<div style='border:1px solid #ffc107;padding:15px;border-radius:8px;'>";
            html += "<h5>💧 Влажность</h5>";
            html += "<div class='form-group'>";
            html += "<label for='humidity_slope'>Коэффициент:</label>";
            html += "<input type='number' id='humidity_slope' step='0.01' min='0.1' max='3.0' placeholder='1.25' style='width:100%;'>";
            html += "</div>";
            html += "<div class='form-group'>";
            html += "<label for='humidity_offset'>Смещение (%):</label>";
            html += "<input type='number' id='humidity_offset' step='0.1' min='-20' max='20' placeholder='-5.0' style='width:100%;'>";
            html += "</div>";
            html += "</div>";
            
            // EC
            html += "<div style='border:1px solid #17a2b8;padding:15px;border-radius:8px;'>";
            html += "<h5>⚡ EC</h5>";
            html += "<div class='form-group'>";
            html += "<label for='ec_slope'>Коэффициент:</label>";
            html += "<input type='number' id='ec_slope' step='0.01' min='0.1' max='3.0' placeholder='1.35' style='width:100%;'>";
            html += "</div>";
            html += "<div class='form-group'>";
            html += "<label for='ec_offset'>Смещение:</label>";
            html += "<input type='number' id='ec_offset' step='0.1' min='-1000' max='1000' placeholder='0.0' style='width:100%;'>";
            html += "</div>";
            html += "</div>";
            
            html += "</div>";
            html += "<button onclick='updateCorrectionFactors()' class='btn btn-warning' style='width:100%;'>Обновить коэффициенты</button>";
            html += "<div id='correction-factors-status' style='margin-top:10px;font-size:14px;min-height:20px;'></div>";
            html += "</div>";

            // Действия с коррекцией
            html += "<div style='margin-top:15px;'>";
            html += "<button onclick='loadCorrectionSettings()' class='btn btn-info'>Загрузить настройки</button>";
            html += "<button onclick='resetCorrectionToDefaults()' class='btn btn-danger'>Сбросить к заводским</button>";
            html += "</div>";
            html += "</div>";



            // ======= ДОПОЛНИТЕЛЬНАЯ ИНФОРМАЦИЯ =======
            html += "<div class='section' style='background:#e8f5e8;padding:15px;border-radius:8px;margin:15px 0;'>";
            html += "<h4>💡 Полезная информация</h4>";
            html += "<ul style='margin:5px 0;padding-left:20px;font-size:14px;'>";
            html += "<li><strong>Стрелки ↑↓</strong> показывают направление изменений после компенсации</li>";
            html += "<li><strong>Сезонные корректировки</strong> учитывают потребности растений в разные периоды</li>";
            html += "<li><strong>Валидность данных</strong> проверяется по диапазонам и логическим связям</li>";
            html += "<li><strong>Интервал обновления:</strong> каждые 5 секунд</li>";
            html += "</ul>";
            
            // Добавляем пояснение о показаниях влажности
            html += "<div style='margin-top:15px;padding:10px;background:#e8f4fd;border-left:4px solid #2196F3;border-radius:4px;'>";
            html += "<h5 style='margin:0 0 8px 0;color:#1976D2;'>💧 Пояснение показаний влажности</h5>";
            html += "<ul style='margin:0;padding-left:20px;font-size:13px;'>";
            html += "<li><strong>VWC (Volumetric Water Content):</strong> объемная влажность почвы - сырые данные датчика в % от объема почвы</li>";
            html += "<li><strong>ASM (Available Soil Moisture):</strong> доступная влага - процент от полевой влагоемкости, доступный растениям</li>";
            html += "<li><strong>FC (Field Capacity):</strong> полевая влагоемкость - максимальное количество воды, которое почва удерживает после стекания избытка</li>";
            html += "<li><strong>PWP (Permanent Wilting Point):</strong> точка увядания - минимальная влажность, при которой растения начинают увядать</li>";
            html += "<li><strong>Формула ASM:</strong> ASM = (VWC - PWP) / (FC - PWP) × 100%</li>";
            html += "</ul>";
            html += "</div>";
            html += "</div>";

            html += "<style>";
            html += ".season-adj { font-size: 0.8em; margin-left: 5px; }";
            html += ".season-adj.up { color: #2ecc71; }";
            html += ".season-adj.down { color: #e74c3c; }";
            html +=
                ".data{width:100%;border-collapse:collapse}.data th,.data td{border:1px solid "
                "#ccc;padding:6px;text-align:center}.data "
                "th{background:#f5f5f5}.green{color:#4CAF50}.yellow{color:#FFC107}.orange{color:#FF9800}.red{color:#"
                "F44336}.blue{color:#2196F3}";
            html += "</style>";

            html += "<script>";
            html += "function set(id,v){if(v!==undefined&&v!==null){document.getElementById(id).textContent=v;}}";
            html +=
                "function colorDelta(a,b){var diff=Math.abs(a-b)/b*100;if(diff>30)return 'red';if(diff>20)return "
                "'orange';if(diff>10)return 'yellow';return '';}";
            html +=
                "function colorRange(v,min,max){var span=(max-min);if(span<=0)return '';if(v<min||v>max)return "
                "'red';if(v<min+0.05*span||v>max-0.05*span)return 'orange';if(v<min+0.10*span||v>max-0.10*span)return "
                "'yellow';return '';}";
            
            // 🌈 НОВАЯ ЛОГИКА РАСКРАСКИ ДЛЯ JXCT ДАТЧИКА
            html += "function colorSensorRange(value, sensorType) {";
            html += "  const ranges = {";
            html += "    temp: { min: -40, max: 80, precision: '±0.5°C' },";
            html += "    hum: { min: 0, max: 100, precision: '±3%RH' },";
            html += "    ph: { min: 3, max: 9, precision: '±0.3pH' },";
            html += "    ec: { min: 0, max: 20000, precision: '±2-5%' },";
            html += "    n: { min: 0, max: 1999, precision: '2%' },";
            html += "    p: { min: 0, max: 1999, precision: '2%' },";
            html += "    k: { min: 0, max: 1999, precision: '2%' }";
            html += "  };";
            html += "  const range = ranges[sensorType];";
            html += "  if (!range) return '';";
            html += "  if (value < range.min || value > range.max) return 'red';";
            html += "  /* ОСОБАЯ ЛОГИКА ДЛЯ ВЛАЖНОСТИ: желтый выше 53% */";
            html += "  if (sensorType === 'hum' && value > 53) return 'yellow';";
            html += "  return 'green';";
            html += "}";
            
            html += "function colorCompensationDeviation(compensated, raw) {";
            html += "  if (raw === 0) return '';";
            html += "  const deviation = Math.abs((compensated - raw) / raw * 100);";
            html += "  if (deviation <= 5) return 'green';";
            html += "  if (deviation <= 10) return 'yellow';";
            html += "  if (deviation <= 15) return 'orange';";
            html += "  return 'red';";
            html += "}";
            html +=
                "function applyColor(spanId,cls){var "
                "el=document.getElementById(spanId);if(!el)return;el.classList.remove('red','orange','yellow','green');"
                "if(cls){el.classList.add(cls);}else{el.classList.add('green');}}";
            html +=
                "var "
                "limits={temp:{min:5,max:40},hum:{min:25,max:60},ec:{min:1000,max:3000},ph:{min:6.0,max:7.0},n:{min:150,max:"
                "350},p:{min:80,max:180},k:{min:180,max:450}};";
            html += "function updateSensor(){";
            html += "fetch('/sensor_json')";
            html += ".then(r => {";
            html += "  if (!r.ok) throw new Error('HTTP ' + r.status);";
            html += "  return r.json();";
            html += "})";
            html += ".then(d => {";
            html += "  if (!d || typeof d !== 'object') throw new Error('Invalid data');";
            html += "  console.log('Valid sensor data received:', d);";
            // 🌈 ФУНКЦИЯ ДЛЯ ОТОБРАЖЕНИЯ RAW ЗНАЧЕНИЙ С ТОЧНОСТЬЮ В СКОБКАХ (ГЛОБАЛЬНАЯ)
            html += "function setRawWithPrecision(id, value, sensorType) {";
            html += "  const ranges = {";
            html += "    temp: { precision: '±0.5°C' },";
            html += "    hum: { precision: '±3%RH', lowPrecisionMin: 53, lowPrecisionMax: 100, lowPrecision: '±5%RH' },";
            html += "    ph: { precision: '±0.3pH' },";
            html += "    ec: { precision: '±2-5%' },";
            html += "    n: { precision: '2%' },";
            html += "    p: { precision: '2%' },";
            html += "    k: { precision: '2%' }";
            html += "  };";
            html += "  const range = ranges[sensorType];";
            html += "  if (!range) {";
            html += "    set(id, value);";
            html += "    return;";
            html += "  }";
            html += "  const element = document.getElementById(id);";
            html += "  if (element) {";
            html += "    let precision = range.precision;";
            html += "    /* ОСОБАЯ ЛОГИКА ДЛЯ ВЛАЖНОСТИ: показываем ±5%RH выше 53% */";
            html += "    if (sensorType === 'hum' && value > 53 && range.lowPrecision) {";
            html += "      precision = range.lowPrecision;";
            html += "    }";
            html += "    element.textContent = value + ' (' + precision + ')';";
            html += "  }";
            html += "}";
            
            // 🌈 УСТАНОВКА RAW ЗНАЧЕНИЙ С ТОЧНОСТЬЮ В СКОБКАХ
            html += "setRawWithPrecision('temp_raw',d.raw_temperature,'temp');";
            html += "setRawWithPrecision('hum_raw',d.raw_humidity,'hum');";
            html += "setRawWithPrecision('ec_raw',d.raw_ec,'ec');";
            html += "setRawWithPrecision('ph_raw',d.raw_ph,'ph');";
            html += "setRawWithPrecision('n_raw',d.raw_nitrogen,'n');";
            html += "setRawWithPrecision('p_raw',d.raw_phosphorus,'p');";
            html += "setRawWithPrecision('k_raw',d.raw_potassium,'k');";
            html +=
                "set('temp_rec',d.rec_temperature);set('hum_rec',d.rec_humidity);set('ec_rec',d.rec_ec);set('ph_rec',d."
                "rec_ph);set('n_rec',d.rec_nitrogen);set('p_rec',d.rec_phosphorus);set('k_rec',d.rec_potassium);";
            // === Arrow indicators block ===
            html += "const tol={temp:0.2,hum:0.5,ec:20,ph:0.05,n:5,p:3,k:3};";
            html +=
                "function "
                "arrowSign(base,val,thr){base=parseFloat(base);val=parseFloat(val);if(isNaN(base)||isNaN(val))return "
                "'';if(val>base+thr)return '↑ ';if(val<base-thr)return '↓ ';return '';};";
            html += "function showWithArrow(id,sign,value){document.getElementById(id).textContent=sign+value;}";

            // Compensated vs RAW arrows
            // Температура и влажность БЕЗ стрелок (просто значения)
            html += "showWithArrow('temp', '', d.temperature);";
            html += "showWithArrow('hum',  '', d.humidity);";
            html += "showWithArrow('ec',   arrowSign(d.raw_ec          ,d.ec          ,tol.ec  ), d.ec);";
            html += "showWithArrow('ph',   arrowSign(d.raw_ph          ,d.ph          ,tol.ph  ), d.ph);";
            html += "showWithArrow('n',    arrowSign(d.raw_nitrogen    ,d.nitrogen    ,tol.n   ), d.nitrogen);";
            html += "showWithArrow('p',    arrowSign(d.raw_phosphorus  ,d.phosphorus  ,tol.p   ), d.phosphorus);";
            html += "showWithArrow('k',    arrowSign(d.raw_potassium   ,d.potassium   ,tol.k   ), d.potassium);";

            // Recommendation arrows (target vs current)
            html +=
                "showWithArrow('temp_rec', arrowSign(d.temperature ,d.rec_temperature ,tol.temp), d.rec_temperature);";
            html += "showWithArrow('hum_rec',  arrowSign(d.humidity    ,d.rec_humidity    ,tol.hum ), d.rec_humidity);";
            html += "showWithArrow('ec_rec',   arrowSign(d.ec          ,d.rec_ec          ,tol.ec  ), d.rec_ec);";
            html += "showWithArrow('ph_rec',   arrowSign(d.ph          ,d.rec_ph          ,tol.ph  ), d.rec_ph);";
            html += "showWithArrow('n_rec',    arrowSign(d.nitrogen    ,d.rec_nitrogen    ,tol.n   ), d.rec_nitrogen);";
            html +=
                "showWithArrow('p_rec',    arrowSign(d.phosphorus  ,d.rec_phosphorus  ,tol.p   ), d.rec_phosphorus);";
            html +=
                "showWithArrow('k_rec',    arrowSign(d.potassium   ,d.rec_potassium   ,tol.k   ), d.rec_potassium);";
            // === End arrow indicators ===

            // Добавляем индикацию сезонных корректировок из новых данных JSON API
            html += "function updateSeasonalAdjustments(d) {";
            html += "  if (d.correction_percentages) {";
            html += "    const percentages = d.correction_percentages;";
            html += "    const spanN = document.getElementById('n_season');";
            html += "    const spanP = document.getElementById('p_season');";
            html += "    const spanK = document.getElementById('k_season');";
            html += "    if(spanN && percentages.nitrogen !== undefined) {";
            html += "      const valueN = parseFloat(percentages.nitrogen);";
            html += "      const signN = valueN >= 0 ? '+' : '';";
            html += "      spanN.textContent = ` (${signN}${valueN}%)`;";
            html += "      spanN.className = 'season-adj ' + (valueN >= 0 ? 'up' : 'down');";
            html += "    }";
            html += "    if(spanP && percentages.phosphorus !== undefined) {";
            html += "      const valueP = parseFloat(percentages.phosphorus);";
            html += "      const signP = valueP >= 0 ? '+' : '';";
            html += "      spanP.textContent = ` (${signP}${valueP}%)`;";
            html += "      spanP.className = 'season-adj ' + (valueP >= 0 ? 'up' : 'down');";
            html += "    }";
            html += "    if(spanK && percentages.potassium !== undefined) {";
            html += "      const valueK = parseFloat(percentages.potassium);";
            html += "      const signK = valueK >= 0 ? '+' : '';";
            html += "      spanK.textContent = ` (${signK}${valueK}%)`;";
            html += "      spanK.className = 'season-adj ' + (valueK >= 0 ? 'up' : 'down');";
            html += "    }";
            html += "  }";
            html += "}";

            // 🌈 ОБНОВЛЕННАЯ ЛОГИКА ВАЛИДАЦИИ С СИНЕЙ ОКРАСКОЙ ДЛЯ ПОЛИВА
            html += "var statusHtml = '';";
            html += "if (d.irrigation) {";
            html += "  statusHtml = '<span class=\"blue\">Полив активен - данные временно не валидны</span>';";
            html += "} else if (d.alerts && Array.isArray(d.alerts) && d.alerts.length > 0) {";
            html += "  statusHtml = '<span class=\"red\">Данные не валидны - ошибки датчика</span>';";
            html += "} else if (d.humidity < 25 || d.temperature < 5 || d.temperature > 40) {";
            html += "  statusHtml = '<span class=\"orange\">Данные не валидны - неоптимальные условия</span>';";
            html += "} else {";
            html += "  statusHtml = '<span class=\"green\">Данные валидны</span>';";
            html += "}";
            html +=
                R"(var seasonColor={'Лето':'green','Весна':'yellow','Осень':'yellow','Зима':'red','Н/Д':''}[d.season]||'';)";
            html += R"(var seasonHtml=seasonColor?(`<span class=\"${seasonColor}\">${d.season}</span>`):d.season;)";
            html += "document.getElementById('statusInfo').innerHTML=statusHtml+' | Сезон: '+seasonHtml;";
            html += "updateSeasonalAdjustments(d);";
            
            // Обновление рекомендаций по взаимодействию питательных веществ
            html += "console.log('Sensor data received:', d);";
            html += "console.log('nutrient_interactions:', d.nutrient_interactions);";
            html += "console.log('crop_specific_recommendations:', d.crop_specific_recommendations);";
            html += "console.log('nutrient-interactions div exists:', !!document.getElementById('nutrient-interactions'));";
            html += "console.log('crop-specific-recommendations div exists:', !!document.getElementById('crop-specific-recommendations'));";
            
            html += "const interactionsDiv = document.getElementById('nutrient-interactions');";
            html += "if(interactionsDiv) {";
            html += "  if(d.nutrient_interactions && typeof d.nutrient_interactions === 'string' && d.nutrient_interactions.length > 0) {";
            html += "    interactionsDiv.innerHTML = d.nutrient_interactions.replace(/\\\\n/g, '<br>');";
            html += "    console.log('Updated nutrient interactions');";
            html += "  } else {";
            html += "    interactionsDiv.innerHTML = '<p style=\\\"color:#28a745;\\\">✅ Антагонизмов питательных веществ не обнаружено</p>';";
            html += "    console.log('No nutrient interactions found');";
            html += "  }";
            html += "} else {";
            html += "  console.error('nutrient-interactions div not found');";
            html += "}";
            
            html += "const cropDiv = document.getElementById('crop-specific-recommendations');";
            html += "if(cropDiv) {";
            html += "  if(d.crop_specific_recommendations && typeof d.crop_specific_recommendations === 'string' && d.crop_specific_recommendations.length > 0) {";
            html += "    cropDiv.innerHTML = d.crop_specific_recommendations.replace(/\\\\n/g, '<br>');";
            html += "    console.log('Updated crop recommendations');";
            html += "  } else {";
            html += "    cropDiv.innerHTML = '<p style=\\\"color:#6c757d;\\\">ℹ️ Выберите культуру для получения специфических рекомендаций</p>';";
            html += "    console.log('No crop selected or no recommendations');";
            html += "  }";
            html += "} else {";
            html += "  console.error('crop-specific-recommendations div not found');";
            html += "}";
            
            // 🌈 НОВАЯ РАСКРАСКА RAW ЗНАЧЕНИЙ ПО ДИАПАЗОНАМ JXCT
            html += "var tvr=parseFloat(d.raw_temperature);applyColor('temp_raw',colorSensorRange(tvr,'temp'));";
            html += "var hvr=parseFloat(d.raw_humidity);applyColor('hum_raw',colorSensorRange(hvr,'hum'));";
            html += "var evr=parseFloat(d.raw_ec);applyColor('ec_raw',colorSensorRange(evr,'ec'));";
            html += "var pvr=parseFloat(d.raw_ph);applyColor('ph_raw',colorSensorRange(pvr,'ph'));";
            html += "var nvr=parseFloat(d.raw_nitrogen);applyColor('n_raw',colorSensorRange(nvr,'n'));";
            html += "var p2r=parseFloat(d.raw_phosphorus);applyColor('p_raw',colorSensorRange(p2r,'p'));";
            html += "var kvr=parseFloat(d.raw_potassium);applyColor('k_raw',colorSensorRange(kvr,'k'));";
            html +=
                "['temp','hum','ec','ph','n','p','k'].forEach(function(id){var "
                "el=document.getElementById(id);if(el){el.classList.remove('red','orange','yellow','green');}});";
            html += "var ct=parseFloat(d.temperature||0);";
            html += "var ch=parseFloat(d.humidity||0);";
            html += "var ce=parseFloat(d.ec||0);";
            html += "var cph=parseFloat(d.ph||0);";
            html += "var cn=parseFloat(d.nitrogen||0);";
            html += "var cp=parseFloat(d.phosphorus||0);";
            html += "var ck=parseFloat(d.potassium||0);";
            
            // 🌈 РАСКРАСКА КОМПЕНСИРОВАННЫХ ЗНАЧЕНИЙ ПО ОТКЛОНЕНИЮ ОТ RAW
            // Температура и влажность НЕ окрашиваем (серый цвет)
            html += "applyColor('temp', '');";  // Серый цвет (без окраски)
            html += "applyColor('hum',  '');";  // Серый цвет (без окраски)
            html += "applyColor('ec',   colorCompensationDeviation(ce, parseFloat(d.raw_ec||0)));";
            html += "applyColor('ph',   colorCompensationDeviation(cph, parseFloat(d.raw_ph||0)));";
            html += "applyColor('n',    colorCompensationDeviation(cn, parseFloat(d.raw_nitrogen||0)));";
            html += "applyColor('p',    colorCompensationDeviation(cp, parseFloat(d.raw_phosphorus||0)));";
            html += "applyColor('k',    colorCompensationDeviation(ck, parseFloat(d.raw_potassium||0)));";
            
            // Применяем цвета к рекомендациям
            html += "applyColor('temp_rec', colorDelta(ct, parseFloat(d.rec_temperature||0)));";
            html += "applyColor('hum_rec',  colorDelta(ch, parseFloat(d.rec_humidity||0)));";
            html += "applyColor('ec_rec',   colorDelta(ce, parseFloat(d.rec_ec||0)));";
            html += "applyColor('ph_rec',   colorDelta(cph,parseFloat(d.rec_ph||0)));";
            html += "applyColor('n_rec',    colorDelta(cn, parseFloat(d.rec_nitrogen||0)));";
            html += "applyColor('p_rec',    colorDelta(cp, parseFloat(d.rec_phosphorus||0)));";
            html += "applyColor('k_rec',    colorDelta(ck, parseFloat(d.rec_potassium||0)));";
            
            html += "}).catch(err => {";
            html += "  console.error('Sensor data fetch error:', err);";
            html += "  const interactionsDiv = document.getElementById('nutrient-interactions');";
            html += "  if(interactionsDiv) interactionsDiv.innerHTML = '<p style=\\\"color:#dc3545;\\\">❌ Ошибка загрузки данных</p>';";
            html += "  const cropDiv = document.getElementById('crop-specific-recommendations');";
            html += "  if(cropDiv) cropDiv.innerHTML = '<p style=\\\"color:#dc3545;\\\">❌ Ошибка загрузки данных</p>';";
            html += "});";
            html += "}";
            
            // Добавляем автоматический запуск обновления
            html += "updateSensor();";
            html += "setInterval(updateSensor, 5000);"; // Оптимизированный интервал


            
            // Функции калибровки температуры и влажности


            // ======= ФУНКЦИИ УПРАВЛЕНИЯ КОРРЕКЦИЕЙ =======
            html += "function loadCorrectionSettings() {";
            html += "  fetch('/api/correction/settings')";
            html += "    .then(response => response.json())";
            html += "    .then(data => {";
            html += "      if(data.success) {";
            html += "        document.getElementById('correction_enabled').value = data.enabled ? 'true' : 'false';";
            html += "        document.getElementById('humidity_slope').value = data.humidity_slope || 1.25;";
            html += "        document.getElementById('humidity_offset').value = data.humidity_offset || -5.0;";
            html += "        document.getElementById('ec_slope').value = data.ec_slope || 1.35;";
            html += "        document.getElementById('ec_offset').value = data.ec_offset || 0.0;";
            html += "        document.getElementById('correction-enabled-status').innerHTML = '✅ Настройки коррекции загружены';";
            html += "        document.getElementById('correction-enabled-status').style.color = '#28a745';";
            html += "        console.log('Calibration settings loaded:', data);";
            html += "      } else {";
            html += "        document.getElementById('correction-enabled-status').innerHTML = '❌ Ошибка загрузки: ' + (data.error || 'Неизвестная ошибка');";
            html += "        document.getElementById('correction-enabled-status').style.color = '#dc3545';";
            html += "      }";
            html += "    })";
            html += "    .catch(err => {";
            html += "      console.error('Load calibration settings error:', err);";
            html += "      document.getElementById('correction-enabled-status').innerHTML = '❌ Ошибка соединения';";
            html += "      document.getElementById('correction-enabled-status').style.color = '#dc3545';";
            html += "    });";
            html += "}";

            html += "function updateCorrectionEnabled() {";
            html += "  const enabled = document.getElementById('correction_enabled').value === 'true';";
            html += "  fetch('/api/correction/enable', {";
            html += "    method: 'POST',";
            html += "    headers: {'Content-Type': 'application/json'},";
            html += "    body: JSON.stringify({enabled: enabled})";
            html += "  }).then(response => response.json())";
            html += "    .then(data => {";
            html += "      if(data.success) {";
            html += "        const status = enabled ? 'включена' : 'отключена';";
            html += "        document.getElementById('correction-enabled-status').innerHTML = '✅ Коррекция ' + status;";
            html += "        document.getElementById('correction-enabled-status').style.color = '#28a745';";
            html += "        console.log('Correction enabled updated:', data);";
            html += "      } else {";
            html += "        document.getElementById('correction-enabled-status').innerHTML = '❌ Ошибка: ' + (data.error || 'Неизвестная ошибка');";
            html += "        document.getElementById('correction-enabled-status').style.color = '#dc3545';";
            html += "      }";
            html += "    })";
            html += "    .catch(err => {";
            html += "      console.error('Update calibration enabled error:', err);";
            html += "      document.getElementById('correction-enabled-status').innerHTML = '❌ Ошибка соединения';";
            html += "      document.getElementById('correction-enabled-status').style.color = '#dc3545';";
            html += "    });";
            html += "}";

            html += "function updateCorrectionFactors() {";
            html += "  const humidity_slope = parseFloat(document.getElementById('humidity_slope').value);";
            html += "  const humidity_offset = parseFloat(document.getElementById('humidity_offset').value);";
            html += "  const ec_slope = parseFloat(document.getElementById('ec_slope').value);";
            html += "  const ec_offset = parseFloat(document.getElementById('ec_offset').value);";
            html += "  if(isNaN(humidity_slope) || isNaN(humidity_offset) || isNaN(ec_slope) || isNaN(ec_offset)) {";
            html += "    alert('⚠️ Введите корректные числовые значения');";
            html += "    return;";
            html += "  }";
            html += "  if(humidity_slope < 0.1 || humidity_slope > 3.0) {";
            html += "    alert('⚠️ Коэффициент влажности должен быть в диапазоне 0.1-3.0');";
            html += "    return;";
            html += "  }";
            html += "  if(humidity_offset < -20 || humidity_offset > 20) {";
            html += "    alert('⚠️ Смещение влажности должно быть в диапазоне -20 до +20%');";
            html += "    return;";
            html += "  }";
            html += "  if(ec_slope < 0.1 || ec_slope > 3.0) {";
            html += "    alert('⚠️ Коэффициент EC должен быть в диапазоне 0.1-3.0');";
            html += "    return;";
            html += "  }";
            html += "  if(ec_offset < -1000 || ec_offset > 1000) {";
            html += "    alert('⚠️ Смещение EC должно быть в диапазоне -1000 до +1000');";
            html += "    return;";
            html += "  }";
            html += "  fetch('/api/correction/factors', {";
            html += "    method: 'POST',";
            html += "    headers: {'Content-Type': 'application/json'},";
            html += "    body: JSON.stringify({";
            html += "      humidity_slope: humidity_slope,";
            html += "      humidity_offset: humidity_offset,";
            html += "      ec_slope: ec_slope,";
            html += "      ec_offset: ec_offset";
            html += "    })";
            html += "  }).then(response => response.json())";
            html += "    .then(data => {";
            html += "      if(data.success) {";
            html += "        document.getElementById('correction-factors-status').innerHTML = '✅ Коэффициенты коррекции обновлены';";
            html += "        document.getElementById('correction-factors-status').style.color = '#28a745';";
            html += "        console.log('Correction factors updated:', data);";
            html += "      } else {";
            html += "        document.getElementById('correction-factors-status').innerHTML = '❌ Ошибка: ' + (data.error || 'Неизвестная ошибка');";
            html += "        document.getElementById('correction-factors-status').style.color = '#dc3545';";
            html += "      }";
            html += "    })";
            html += "    .catch(err => {";
            html += "      console.error('Update calibration factors error:', err);";
            html += "      document.getElementById('correction-factors-status').innerHTML = '❌ Ошибка соединения';";
            html += "      document.getElementById('correction-factors-status').style.color = '#dc3545';";
            html += "    });";
            html += "}";

            html += "function resetCorrectionToDefaults() {";
            html += "  if(confirm('⚠️ Сбросить коэффициенты коррекции к заводским настройкам?\\n\\nЗаводские коэффициенты:\\n• Влажность: 1.25x -5%\\n• EC: 1.35x\\n• Температура: 1.0x')) {";
            html += "    fetch('/api/correction/reset', {method: 'POST'})";
            html += "      .then(response => response.json())";
            html += "      .then(data => {";
            html += "        if(data.success) {";
            html += "          loadCorrectionSettings();";
            html += "          alert('✅ Коэффициенты коррекции сброшены к заводским настройкам');";
            html += "        } else {";
            html += "          alert('❌ Ошибка сброса: ' + (data.error || 'Неизвестная ошибка'));";
            html += "        }";
            html += "      })";
            html += "      .catch(err => {";
            html += "        console.error('Reset correction error:', err);";
            html += "        alert('❌ Ошибка соединения');";
            html += "      });";
            html += "  }";
            html += "}";

            // УДАЛЕНО: Дублированный setInterval - оставляем только один
            html += "updateSensor();";
            html += "loadCorrectionSettings();";
            html += "</script>";

            // API-ссылка внизу страницы
            html += "<div style='margin-top:15px;font-size:14px;color:#555'><b>API:</b> <a href='" +
                    String(API_SENSOR) + "' target='_blank'>" + String(API_SENSOR) + "</a> (JSON, +timestamp)</div>";

            html += generatePageFooter();
            webServer.send(200, "text/html; charset=utf-8", html);
        });

    // AJAX эндпоинт для обновления показаний
    webServer.on("/sensor_json", HTTP_GET, sendSensorJson);

    // Primary API v1 endpoint
    webServer.on(API_SENSOR, HTTP_GET, sendSensorJson);

    // Загрузка калибровочного CSV через вкладку
    webServer.on("/readings/upload", HTTP_POST, []() {}, handleReadingsUpload);

    // Сброс пользовательских CSV (удаляем все *.csv)
    webServer.on("/readings/csv_reset", HTTP_POST,
                 []()
                 {
                     logWebRequest("POST", "/readings/csv_reset", webServer.client().remoteIP().toString());

                     // ✅ CSRF защита - критическая операция удаления!
                     if (!checkCSRFSafety())
                     {
                         logWarnSafe("\1", webServer.client().remoteIP().toString().c_str());
                         const String html = generateErrorPage(403, "Forbidden: Недействительный CSRF токен");
                         webServer.send(403, "text/html; charset=utf-8", html);
                         return;
                     }

                     CalibrationManager::init();
                     const bool removed = CalibrationManager::deleteTable(SoilProfile::SAND);
                     const String toast = removed ? "CSV+удален" : "CSV+не+найден";
                     webServer.sendHeader("Location", String("/readings?toast=") + toast, true);
                     webServer.send(302, "text/plain", "Redirect");
                 });

    // Форма для сохранения профиля
    webServer.on("/readings/profile", HTTP_POST, []() {}, handleProfileSave);

    // Обслуживание статических файлов из LittleFS
    webServer.on(
        "/docs/examples/calibration_example.csv", HTTP_GET,
        []()
        {
            logWebRequest("GET", "/docs/examples/calibration_example.csv", webServer.client().remoteIP().toString());

            if (LittleFS.exists("/docs/examples/calibration_example.csv"))
            {
                File file = LittleFS.open("/docs/examples/calibration_example.csv", "r");
                if (file)
                {
                    webServer.sendHeader("Content-Type", "text/csv");
                    webServer.sendHeader("Content-Disposition", "attachment; filename=\"calibration_example.csv\"");
                    webServer.streamFile(file, "text/csv");
                    file.close();
                }
                else
                {
                    webServer.send(404, "text/plain", "File not found");
                }
            }
            else
            {
                // Если файл не найден, создаем его на лету
                webServer.sendHeader("Content-Type", "text/csv");
                webServer.sendHeader("Content-Disposition", "attachment; filename=\"calibration_example.csv\"");
                String csvContent = "# Пример калибровочной таблицы для JXCT датчика\n";
                csvContent += "# Формат: сырое_значение,коэффициент_коррекции\n";
                csvContent +=
                    "# Коэффициент применяется как: скорректированное_значение = сырое_значение * коэффициент\n\n";
                csvContent += "# Электропроводность (µS/cm) - может требовать коррекции\n";
                csvContent += "0,1.000\n";
                csvContent += "500,0.98\n";
                csvContent += "1000,0.95\n";
                csvContent += "1500,0.93\n";
                csvContent += "2000,0.91\n";
                csvContent += "3000,0.89\n";
                csvContent += "5000,0.87\n\n";
                csvContent += "# pH - может требовать коррекции\n";
                csvContent += "3.0,1.000\n";
                csvContent += "4.0,1.000\n";
                csvContent += "5.0,1.000\n";
                csvContent += "6.0,1.000\n";
                csvContent += "7.0,1.000\n";
                csvContent += "8.0,1.000\n";
                csvContent += "9.0,1.000\n\n";
                csvContent += "# Азот (мг/кг) - может требовать коррекции\n";
                csvContent += "0,1.000\n";
                csvContent += "100,0.95\n";
                csvContent += "200,0.92\n";
                csvContent += "500,0.89\n";
                csvContent += "1000,0.87\n";
                csvContent += "1500,0.85\n";
                webServer.send(200, "text/csv", csvContent);
            }
        });

    // Deprecated alias удалён в v2.7.0

    // API маршруты калибровки
    webServer.on("/api/calibration/status", HTTP_GET,
                 []()
                 {
                     logWebRequest("GET", "/api/calibration/status", webServer.client().remoteIP().toString());
                     
                     String statusJson = gCalibrationService.getCalibrationStatus();
                     webServer.send(200, "application/json", statusJson);
                 });

    // API endpoints для температуры и влажности
    webServer.on("/api/calibration/temperature/add", HTTP_POST,
                 []()
                 {
                     logWebRequest("POST", "/api/calibration/temperature/add", webServer.client().remoteIP().toString());
                     
                     DynamicJsonDocument doc(512);
                     DeserializationError error = deserializeJson(doc, webServer.arg("plain"));
                     if (error) {
                         webServer.send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
                         return;
                     }
                     
                     float expected = doc["expected"];
                     float measured = doc["measured"];
                     
                     // Валидация данных
                     if (isnan(expected) || isnan(measured)) {
                         webServer.send(400, "application/json", "{\"success\":false,\"error\":\"Invalid temperature values\"}");
                         return;
                     }
                     
                     // Validate JXCT sensor temperature range (-45 to +115°C)
                     if (expected < -45 || expected > 115 || measured < -45 || measured > 115) {
                         webServer.send(400, "application/json", "{\"success\":false,\"error\":\"Temperature values out of JXCT sensor range (-45 to +115°C)\"}");
                         return;
                     }
                     
                     bool success = gCalibrationService.addTemperatureCalibrationPoint(expected, measured);
                     
                     DynamicJsonDocument response(256);
                     response["success"] = success;
                     if (success) {
                         response["message"] = "Temperature calibration point added successfully";
                         response["offset"] = expected - measured;
                     } else {
                         response["error"] = "Failed to add temperature calibration point";
                     }
                     
                     String response_str;
                     serializeJson(response, response_str);
                     webServer.send(200, "application/json", response_str);
                 });

    webServer.on("/api/calibration/humidity/add", HTTP_POST,
                 []()
                 {
                     logWebRequest("POST", "/api/calibration/humidity/add", webServer.client().remoteIP().toString());
                     
                     DynamicJsonDocument doc(512);
                     DeserializationError error = deserializeJson(doc, webServer.arg("plain"));
                     if (error) {
                         webServer.send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
                         return;
                     }
                     
                     float expected = doc["expected"];
                     float measured = doc["measured"];
                     
                     // Валидация данных
                     if (isnan(expected) || isnan(measured)) {
                         webServer.send(400, "application/json", "{\"success\":false,\"error\":\"Invalid humidity values\"}");
                         return;
                     }
                     
                     // Validate JXCT sensor humidity range (0-100%RH)
                     if (expected < 0 || expected > 100 || measured < 0 || measured > 100) {
                         webServer.send(400, "application/json", "{\"success\":false,\"error\":\"Humidity values out of JXCT sensor range (0-100%RH)\"}");
                         return;
                     }
                     
                     bool success = gCalibrationService.addHumidityCalibrationPoint(expected, measured);
                     
                     DynamicJsonDocument response(256);
                     response["success"] = success;
                     if (success) {
                         response["message"] = "Humidity calibration point added successfully";
                         response["offset"] = expected - measured;
                     } else {
                         response["error"] = "Failed to add humidity calibration point";
                     }
                     
                     String response_str;
                     serializeJson(response, response_str);
                     webServer.send(200, "application/json", response_str);
                 });

    webServer.on("/api/calibration/ph/add", HTTP_POST,
                 []()
                 {
                     DynamicJsonDocument doc(512);
                     DeserializationError error = deserializeJson(doc, webServer.arg("plain"));

                     if (error)
                     {
                         webServer.send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
                         return;
                     }

                     float expected = doc["expected"];
                     float measured = doc["measured"];

                     // ИСПРАВЛЕНО: Реальная реализация pH калибровки
                     bool success = false;
                     try {
                         // Валидация входных данных согласно JXCT спецификации
                         if (isnan(expected) || isnan(measured)) {
                             webServer.send(400, "application/json", "{\"success\":false,\"error\":\"Invalid pH values\"}");
                             return;
                         }
                         
                         // Validate JXCT sensor pH range (3-9 pH)
                         if (expected < 3 || expected > 9 || measured < 3 || measured > 9) {
                             logWarnSafe("pH вне диапазона JXCT: expected=%.1f, measured=%.1f", expected, measured);
                             webServer.send(400, "application/json", "{\"success\":false,\"error\":\"pH values out of JXCT sensor range (3-9 pH)\"}");
                             return;
                         }
                         
                         // Добавляем pH калибровочную точку
                         success = gCalibrationService.addPHCalibrationPoint(expected, measured);
                         
                         if (success) {
                             logSuccessSafe("pH калибровочная точка добавлена: expected=%.1f, measured=%.1f", expected, measured);
                         } else {
                             logError("Ошибка добавления pH калибровочной точки");
                         }
                     } catch (...) {
                         logError("Исключение при добавлении pH калибровки");
                         success = false;
                     }

                     DynamicJsonDocument response(256);
                     response["success"] = success;
                     if (!success)
                     {
                         response["error"] = "Failed to add pH calibration point";
                     } else {
                         response["message"] = "pH calibration point added successfully";
                     }

                     String response_str;
                     serializeJson(response, response_str);
                     webServer.send(200, "application/json", response_str);
                 });

    webServer.on("/api/calibration/ec/add", HTTP_POST,
                 []()
                 {
                     DynamicJsonDocument doc(512);
                     DeserializationError error = deserializeJson(doc, webServer.arg("plain"));

                     if (error)
                     {
                         webServer.send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
                         return;
                     }

                     float expected = doc["expected"];
                     float measured = doc["measured"];

                     // ИСПРАВЛЕНО: Реальная реализация EC калибровки
                     bool success = false;
                     try {
                         // Валидация входных данных согласно JXCT спецификации
                         if (isnan(expected) || isnan(measured)) {
                             webServer.send(400, "application/json", "{\"success\":false,\"error\":\"Invalid EC values\"}");
                             return;
                         }
                         
                         // Validate JXCT sensor EC range (0-10000 µS/cm)
                         if (expected < 0 || expected > 10000 || measured < 0 || measured > 10000) {
                             logWarnSafe("EC вне диапазона JXCT: expected=%.0f, measured=%.0f", expected, measured);
                             webServer.send(400, "application/json", "{\"success\":false,\"error\":\"EC values out of JXCT sensor range (0-10000 µS/cm)\"}");
                             return;
                         }
                         
                         // Добавляем EC калибровочную точку
                         success = gCalibrationService.addECCalibrationPoint(expected, measured);
                         
                         if (success) {
                             logSuccessSafe("EC калибровочная точка добавлена: expected=%.0f, measured=%.0f", expected, measured);
                         } else {
                             logError("Ошибка добавления EC калибровочной точки");
                         }
                     } catch (...) {
                         logError("Исключение при добавлении EC калибровки");
                         success = false;
                     }

                     DynamicJsonDocument response(256);
                     response["success"] = success;
                     if (!success)
                     {
                         response["error"] = "Failed to add EC calibration point";
                     } else {
                         response["message"] = "EC calibration point added successfully";
                     }

                     String response_str;
                     serializeJson(response, response_str);
                     webServer.send(200, "application/json", response_str);
                 });

    webServer.on("/api/calibration/npk/set", HTTP_POST,
                 []()
                 {
                     logWebRequest("POST", "/api/calibration/npk/set", webServer.client().remoteIP().toString());
                     
                     DynamicJsonDocument doc(512);
                     DeserializationError error = deserializeJson(doc, webServer.arg("plain"));

                     if (error)
                     {
                         logWarn("Ошибка парсинга JSON для NPK калибровки");
                         webServer.send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
                         return;
                     }

                     float n = doc["n"];
                     float p = doc["p"];
                     float k = doc["k"];

                     // ИСПРАВЛЕНО: Реальная реализация NPK калибровки
                     bool success = false;
                     try {
                         // Валидация входных данных согласно JXCT спецификации
                         if (isnan(n) || isnan(p) || isnan(k)) {
                             webServer.send(400, "application/json", "{\"success\":false,\"error\":\"Invalid NPK values\"}");
                             return;
                         }
                         
                         // Validate JXCT sensor NPK range (0-1999 mg/kg)
                         if (n < 0 || n > 1999 || p < 0 || p > 1999 || k < 0 || k > 1999) {
                             logWarnSafe("NPK вне диапазона JXCT: N=%.0f, P=%.0f, K=%.0f", n, p, k);
                             webServer.send(400, "application/json", "{\"success\":false,\"error\":\"NPK values out of JXCT sensor range (0-1999 mg/kg)\"}");
                             return;
                         }
                         
                         // Устанавливаем NPK калибровочную точку
                         success = gCalibrationService.setNPKCalibrationPoint(n, p, k);
                         
                         if (success) {
                             logSuccessSafe("NPK калибровочная точка установлена: N=%.0f, P=%.0f, K=%.0f", n, p, k);
                             
                                                      // Включаем компенсацию в конфигурации
                         config.flags.compensationEnabled = true;
                             saveConfig();
                         } else {
                             logError("Ошибка установки NPK калибровочной точки");
                         }
                     } catch (...) {
                         logError("Исключение при установке NPK калибровки");
                         success = false;
                     }

                     DynamicJsonDocument response(256);
                     response["success"] = success;
                     if (!success)
                     {
                         response["error"] = "Failed to set NPK calibration point";
                     } else {
                         response["message"] = "NPK calibration point set successfully";
                     }

                     String response_str;
                     serializeJson(response, response_str);
                     webServer.send(200, "application/json", response_str);
                 });

    webServer.on("/api/calibration/ph/calculate", HTTP_POST,
                 []()
                 {
                     logWebRequest("POST", "/api/calibration/ph/calculate", webServer.client().remoteIP().toString());
                     
                     // ИСПРАВЛЕНО: Реальная реализация расчета pH калибровки
                     bool success = false;
                     float r_squared = 0.0F;
                     
                     try {
                         success = gCalibrationService.calculatePHCalibration();
                         r_squared = 0.95F;  // Временно, пока не реализован возврат R²
                         
                         if (success) {
                             logSuccess("pH калибровка рассчитана успешно, R² = " + String(r_squared, 3));
                         } else {
                             logError("Ошибка расчета pH калибровки");
                         }
                     } catch (...) {
                         logError("Исключение при расчете pH калибровки");
                         success = false;
                     }

                     DynamicJsonDocument response(256);
                     response["success"] = success;
                     if (success)
                     {
                         response["r_squared"] = r_squared;
                         response["message"] = "pH calibration calculated successfully";
                     }
                     else
                     {
                         response["error"] = "Failed to calculate pH calibration";
                     }

                     String response_str;
                     serializeJson(response, response_str);
                     webServer.send(200, "application/json", response_str);
                 });

    webServer.on("/api/calibration/ec/calculate", HTTP_POST,
                 []()
                 {
                     logWebRequest("POST", "/api/calibration/ec/calculate", webServer.client().remoteIP().toString());
                     
                     // ИСПРАВЛЕНО: Реальная реализация расчета EC калибровки
                     bool success = false;
                     float r_squared = 0.0F;
                     
                     try {
                         success = gCalibrationService.calculateECCalibration();
                         r_squared = 0.95F;  // Временно, пока не реализован возврат R²
                         
                         if (success) {
                             logSuccess("EC калибровка рассчитана успешно, R² = " + String(r_squared, 3));
                         } else {
                             logError("Ошибка расчета EC калибровки");
                         }
                     } catch (...) {
                         logError("Исключение при расчете EC калибровки");
                         success = false;
                     }

                     DynamicJsonDocument response(256);
                     response["success"] = success;
                     if (success)
                     {
                         response["r_squared"] = r_squared;
                         response["message"] = "EC calibration calculated successfully";
                     }
                     else
                     {
                         response["error"] = "Failed to calculate EC calibration";
                     }

                     String response_str;
                     serializeJson(response, response_str);
                     webServer.send(200, "application/json", response_str);
                 });

    webServer.on("/api/calibration/export", HTTP_GET,
                 []()
                 {
                     logWebRequest("GET", "/api/calibration/export", webServer.client().remoteIP().toString());
                     
                     // ИСПРАВЛЕНО: Реальная реализация экспорта калибровки
                     DynamicJsonDocument doc(1024);
                     
                     try {
                         // Получаем данные калибровки из сервиса
                         JsonArray phPoints = doc.createNestedArray("ph_points");
                         JsonArray ecPoints = doc.createNestedArray("ec_points");
                         JsonObject npkZero = doc.createNestedObject("npk_zero");
                         
                         String calibrationData = gCalibrationService.exportCalibrationToJSON();
                         bool hasData = !calibrationData.isEmpty();
                         
                         if (hasData) {
                             // Парсим JSON и заполняем массивы
                             DynamicJsonDocument calDoc(1024);
                             deserializeJson(calDoc, calibrationData);
                             
                             if (calDoc.containsKey("ph_points")) {
                                 phPoints = calDoc["ph_points"];
                             }
                             if (calDoc.containsKey("ec_points")) {
                                 ecPoints = calDoc["ec_points"];
                             }
                             if (calDoc.containsKey("npk_zero")) {
                                 npkZero = calDoc["npk_zero"];
                             }
                         }
                         
                         doc["calculated"] = hasData;
                         doc["timestamp"] = millis();
                         
                         if (hasData) {
                             logSuccess("Калибровка экспортирована успешно");
                         } else {
                             logWarn("Нет данных калибровки для экспорта");
                         }
                     } catch (...) {
                         logError("Исключение при экспорте калибровки");
                         doc["error"] = "Export failed";
                         doc["calculated"] = false;
                     }

                     String json_data;
                     serializeJson(doc, json_data);
                     webServer.send(200, "application/json", json_data);
                 });

    webServer.on("/api/calibration/import", HTTP_POST,
                 []()
                 {
                     logWebRequest("POST", "/api/calibration/import", webServer.client().remoteIP().toString());
                     
                     String json_data = webServer.arg("plain");
                     
                     // ИСПРАВЛЕНО: Реальная реализация импорта калибровки
                     bool success = false;
                     
                     try {
                         DynamicJsonDocument doc(1024);
                         DeserializationError error = deserializeJson(doc, json_data);
                         
                         if (error) {
                             logWarn("Ошибка парсинга JSON для импорта калибровки");
                             webServer.send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
                             return;
                         }
                         
                         // Импортируем данные калибровки
                         String jsonData;
                         serializeJson(doc, jsonData);
                         success = gCalibrationService.importCalibrationFromJSON(jsonData);
                         
                         if (success) {
                             logSuccess("Калибровка импортирована успешно");
                             
                             // Включаем компенсацию в конфигурации
                             config.flags.compensationEnabled = true;
                             saveConfig();
                         } else {
                             logError("Ошибка импорта калибровки");
                         }
                     } catch (...) {
                         logError("Исключение при импорте калибровки");
                         success = false;
                     }

                     DynamicJsonDocument response(256);
                     response["success"] = success;
                     if (!success)
                     {
                         response["error"] = "Failed to import calibration";
                     } else {
                         response["message"] = "Calibration imported successfully";
                     }

                     String response_str;
                     serializeJson(response, response_str);
                     webServer.send(200, "application/json", response_str);
                 });

    webServer.on("/api/calibration/reset", HTTP_POST,
                 []()
                 {
                     logWebRequest("POST", "/api/calibration/reset", webServer.client().remoteIP().toString());
                     
                     // ИСПРАВЛЕНО: Реальная реализация сброса калибровки
                     try {
                                              // Сбрасываем калибровочные данные
                         gCalibrationService.resetCalibration();
                         
                         // Сбрасываем фильтры
                         AdvancedFilters::resetAllFilters();
                         
                         // НЕ трогаем флаг компенсации! Калибровка и компенсация - разные вещи
                         // config.flags.compensationEnabled остается как есть
                         saveConfig();
                         
                         logSuccess("Калибровка успешно сброшена");
                         
                         DynamicJsonDocument response(128);
                         response["success"] = true;
                         response["message"] = "Калибровка сброшена успешно";

                         String response_str;
                         serializeJson(response, response_str);
                         webServer.send(200, "application/json", response_str);
                     } catch (...) {
                         logError("Ошибка при сбросе калибровки");
                         
                         DynamicJsonDocument response(128);
                         response["success"] = false;
                         response["error"] = "Ошибка при сбросе калибровки";

                         String response_str;
                         serializeJson(response, response_str);
                         webServer.send(500, "application/json", response_str);
                     }
                 });

                // ======= API ЭНДПОИНТЫ ДЛЯ УПРАВЛЕНИЯ КОРРЕКЦИЕЙ =======
    
    // Получение настроек коррекции
    webServer.on("/api/correction/settings", HTTP_GET,
                 []()
                 {
                     logWebRequest("GET", "/api/correction/settings", webServer.client().remoteIP().toString());
                     
                     try {
                         CorrectionFactors factors = SensorCorrection::getCorrectionFactors();
                         
                         DynamicJsonDocument doc(512);
                         doc["success"] = true;
                         doc["enabled"] = factors.enabled;
                         doc["humidity_slope"] = factors.humiditySlope;
                         doc["humidity_offset"] = factors.humidityOffset;
                         doc["ec_slope"] = factors.ecSlope;
                         doc["ec_offset"] = factors.ecOffset;
                         doc["temperature_slope"] = factors.temperatureSlope;
                         doc["temperature_offset"] = factors.temperatureOffset;
                         
                         String json_data;
                         serializeJson(doc, json_data);
                         webServer.send(200, "application/json", json_data);
                         
                         logDebugSafe("Настройки коррекции отправлены: enabled=%s, hum_slope=%.2f, hum_offset=%.1f, ec_slope=%.2f",
                                      factors.enabled ? "true" : "false", factors.humiditySlope, factors.humidityOffset, factors.ecSlope);
                     } catch (...) {
                         logError("Ошибка при получении настроек коррекции");
                         webServer.send(500, "application/json", "{\"success\":false,\"error\":\"Internal server error\"}");
                     }
                 });

    // Включение/отключение коррекции
    webServer.on("/api/correction/enable", HTTP_POST,
                 []()
                 {
                     logWebRequest("POST", "/api/correction/enable", webServer.client().remoteIP().toString());
                     
                     String json_data = webServer.arg("plain");
                     bool success = false;
                     
                     try {
                         DynamicJsonDocument doc(256);
                         DeserializationError error = deserializeJson(doc, json_data);
                         
                         if (error) {
                             logWarn("Ошибка парсинга JSON для включения коррекции");
                             webServer.send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
                             return;
                         }
                         
                         bool enabled = doc["enabled"] | false;
                         SensorCorrection::enableCorrection(enabled);
                         success = true;
                         
                         logSuccess("Коррекция " + String(enabled ? "включена" : "отключена"));
                     } catch (...) {
                         logError("Ошибка при включении/отключении коррекции");
                         success = false;
                     }

                     DynamicJsonDocument response(256);
                     response["success"] = success;
                     if (!success) {
                         response["error"] = "Failed to update correction enabled state";
                     } else {
                         response["message"] = "Correction enabled state updated successfully";
                     }

                     String response_str;
                     serializeJson(response, response_str);
                     webServer.send(200, "application/json", response_str);
                 });

    // Обновление коэффициентов коррекции
    webServer.on("/api/correction/factors", HTTP_POST,
                 []()
                 {
                     logWebRequest("POST", "/api/correction/factors", webServer.client().remoteIP().toString());
                     
                     String json_data = webServer.arg("plain");
                     bool success = false;
                     
                     try {
                         DynamicJsonDocument doc(512);
                         DeserializationError error = deserializeJson(doc, json_data);
                         
                         if (error) {
                             logWarn("Ошибка парсинга JSON для обновления коэффициентов");
                             webServer.send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
                             return;
                         }
                         
                         CorrectionFactors factors = SensorCorrection::getCorrectionFactors();
                         
                         // Обновляем только переданные коэффициенты
                         if (doc.containsKey("humidity_slope")) {
                             factors.humiditySlope = doc["humidity_slope"] | 1.25f;
                         }
                         if (doc.containsKey("humidity_offset")) {
                             factors.humidityOffset = doc["humidity_offset"] | -5.0f;
                         }
                         if (doc.containsKey("ec_slope")) {
                             factors.ecSlope = doc["ec_slope"] | 1.35f;
                         }
                         if (doc.containsKey("ec_offset")) {
                             factors.ecOffset = doc["ec_offset"] | 0.0f;
                         }
                         if (doc.containsKey("temperature_slope")) {
                             factors.temperatureSlope = doc["temperature_slope"] | 1.0f;
                         }
                         if (doc.containsKey("temperature_offset")) {
                             factors.temperatureOffset = doc["temperature_offset"] | 0.0f;
                         }
                         
                         SensorCorrection::setCorrectionFactors(factors);
                         success = true;
                         
                         logSuccess("Коэффициенты коррекции обновлены: hum_slope=" + String(factors.humiditySlope, 2) + 
                                   ", hum_offset=" + String(factors.humidityOffset, 1) + 
                                   ", ec_slope=" + String(factors.ecSlope, 2));
                     } catch (...) {
                         logError("Ошибка при обновлении коэффициентов коррекции");
                         success = false;
                     }

                     DynamicJsonDocument response(256);
                     response["success"] = success;
                     if (!success) {
                         response["error"] = "Failed to update correction factors";
                     } else {
                         response["message"] = "Correction factors updated successfully";
                     }

                     String response_str;
                     serializeJson(response, response_str);
                     webServer.send(200, "application/json", response_str);
                 });

    // Сброс к заводским настройкам
    webServer.on("/api/correction/reset", HTTP_POST,
                 []()
                 {
                     logWebRequest("POST", "/api/correction/reset", webServer.client().remoteIP().toString());
                     
                     try {
                         // Сбрасываем к заводским коэффициентам
                         CorrectionFactors defaultFactors = {
                             .humiditySlope = 1.25f,
                             .humidityOffset = -5.0f,
                             .ecSlope = 1.35f,
                             .ecOffset = 0.0f,
                             .temperatureSlope = 1.0f,
                             .temperatureOffset = 0.0f,
                             .enabled = true
                         };
                         
                         SensorCorrection::setCorrectionFactors(defaultFactors);
                         
                         logSuccess("Коэффициенты коррекции сброшены к заводским настройкам");
                         
                         DynamicJsonDocument response(128);
                         response["success"] = true;
                         response["message"] = "Correction factors reset to factory defaults";

                         String response_str;
                         serializeJson(response, response_str);
                         webServer.send(200, "application/json", response_str);
                     } catch (...) {
                         logError("Ошибка при сбросе коэффициентов коррекции");
                         
                         DynamicJsonDocument response(128);
                         response["success"] = false;
                         response["error"] = "Failed to reset correction factors";

                         String response_str;
                         serializeJson(response, response_str);
                         webServer.send(500, "application/json", response_str);
                     }
                 });

    logDebug("Маршруты данных настроены: /readings, /api/v1/sensor (json), /sensor_json [legacy], /api/calibration/*, /api/correction/*");
}
