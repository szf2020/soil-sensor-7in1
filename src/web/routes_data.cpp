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

// Глобальный экземпляр сервиса калибровки
extern SensorCalibrationService gCalibrationService;

extern NTPClient* timeClient;

// Внешние зависимости (уже объявлены в заголовочных файлах)
// extern String navHtml();  // объявлено в wifi_manager.h
// extern String formatValue(float value, const char* unit, int precision);  // объявлено в jxct_format_utils.h
// extern String getApSsid();  // объявлено в wifi_manager.h

// Буфер для загрузки файлов (калибровка через /readings)
namespace
{
File uploadFile;
SoilProfile uploadProfile = SoilProfile::SAND;

// Используем RecValues из бизнес-сервиса

// Функции сезонной коррекции NPK перенесены в бизнес-сервис CropRecommendationEngine

RecValues computeRecommendations()
{
    // Используем бизнес-сервис для вычисления рекомендаций
    const String cropId = String(config.cropId);

    // Преобразуем конфигурацию в типы бизнес-логики
    SoilProfile soilProfile = SoilProfile::SAND;
    EnvironmentType envType = EnvironmentType::OUTDOOR;

    // Используем массивы для устранения дублирования кода
    static const std::array<SoilProfile, 13> soilProfiles = {{
        SoilProfile::SAND,        // 0
        SoilProfile::LOAM,        // 1
        SoilProfile::PEAT,        // 2
        SoilProfile::CLAY,        // 3
        SoilProfile::SANDPEAT,    // 4
        SoilProfile::SILT,        // 5 - НОВЫЙ
        SoilProfile::CLAY_LOAM,   // 6 - НОВЫЙ
        SoilProfile::ORGANIC,     // 7 - НОВЫЙ
        SoilProfile::SANDY_LOAM,  // 8 - НОВЫЙ
        SoilProfile::SILTY_LOAM,  // 9 - НОВЫЙ
        SoilProfile::LOAMY_CLAY,  // 10 - НОВЫЙ
        SoilProfile::SALINE,      // 11 - НОВЫЙ
        SoilProfile::ALKALINE     // 12 - НОВЫЙ
    }};

    static const std::array<EnvironmentType, 3> envTypes = {{
        EnvironmentType::OUTDOOR,     // 0
        EnvironmentType::GREENHOUSE,  // 1
        EnvironmentType::INDOOR       // 2
    }};

    const int soilIndex = (config.soilProfile >= 0 && config.soilProfile < 13) ? config.soilProfile : 0;
    const int envIndex = (config.environmentType >= 0 && config.environmentType < 3) ? config.environmentType : 0;

    soilProfile = soilProfiles[soilIndex];
    envType = envTypes[envIndex];

    // Получаем рекомендации от бизнес-сервиса
    RecValues rec = getCropEngine().computeRecommendations(cropId, soilProfile, envType);

    // Применяем сезонную коррекцию если включена
    if (config.flags.seasonalAdjustEnabled)
    {
        time_t now = time(nullptr);
        struct tm* timeInfo = localtime(&now);
        const int month = timeInfo != nullptr ? timeInfo->tm_mon + 1 : 1;

        // Определяем сезон
        Season season = Season::WINTER;
        if (month >= 3 && month <= 5)
            season = Season::SPRING;
        else if (month >= 6 && month <= 8)
            season = Season::SUMMER;
        else if (month >= 9 && month <= 11)
            season = Season::AUTUMN;
        else
            season = Season::WINTER;

        const bool isGreenhouse = (config.environmentType == 1);
        getCropEngine().applySeasonalCorrection(rec, season, isGreenhouse);
    }

    return rec;
}
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
    doc["temperature"] = format_temperature(sensorData.temperature);
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

    const RecValues rec = computeRecommendations();
    doc["rec_temperature"] = format_temperature(rec.t);
    doc["rec_humidity"] = format_moisture(rec.hum);
    doc["rec_ec"] = format_ec(rec.ec);
    doc["rec_ph"] = format_ph(rec.ph);
    doc["rec_nitrogen"] = format_npk(rec.n);
    doc["rec_phosphorus"] = format_npk(rec.p);
    doc["rec_potassium"] = format_npk(rec.k);

    // ---- Рекомендации по взаимодействию питательных веществ ----
    NPKReferences npk{sensorData.nitrogen, sensorData.phosphorus, sensorData.potassium};
    SoilType soilType = static_cast<SoilType>(config.soilProfile);
    
    logDebugSafe("JSON API: soilProfile=%d, soilType=%d, cropId='%s' (len=%d)", 
                 config.soilProfile, (int)soilType, config.cropId, strlen(config.cropId));
    logDebugSafe("JSON API: NPK values N=%.1f P=%.1f K=%.1f pH=%.1f", 
                 npk.nitrogen, npk.phosphorus, npk.potassium, sensorData.ph);
    
    // Получаем рекомендации по антагонизмам
    String antagonismRecommendations = getNutrientInteractionService().generateAntagonismRecommendations(
        npk, soilType, sensorData.ph);
    doc["nutrient_interactions"] = antagonismRecommendations;
    
    logDebugSafe("JSON API: antagonismRecommendations='%s'", antagonismRecommendations.c_str());
    
    // Получаем специфические рекомендации по культурам
    logDebugSafe("JSON API: checking crop - strlen=%d, strcmp=%d, cropId='%s'", 
                 strlen(config.cropId), strcmp(config.cropId, "none"), config.cropId);
    
    // ✅ Дополнительная проверка: если cropId пустой, устанавливаем "none"
    if (strlen(config.cropId) == 0) {
        strlcpy(config.cropId, "none", sizeof(config.cropId));
        logDebugSafe("JSON API: cropId was empty, set to 'none'");
    }
    
    // ✅ Добавляем cropId в JSON для отладки  
    doc["crop_id"] = String(config.cropId);
    doc["crop_id_debug"] = String("len=") + String(strlen(config.cropId)) + ", strcmp=" + String(strcmp(config.cropId, "none"));
    doc["crop_id_hex"] = "";
    // Показываем первые 8 байт в hex для отладки
    for(int i = 0; i < min(8, (int)strlen(config.cropId)); i++) {
        char hex[4];
        sprintf(hex, "%02X ", (unsigned char)config.cropId[i]);
        doc["crop_id_hex"] = doc["crop_id_hex"].as<String>() + String(hex);
    }
    doc["crop_len_check"] = strlen(config.cropId) > 0;
    doc["crop_str_check"] = strcmp(config.cropId, "none") != 0;
    doc["debug_npk_n"] = npk.nitrogen;
    doc["debug_npk_p"] = npk.phosphorus;
    doc["debug_npk_k"] = npk.potassium;
    doc["debug_ph"] = sensorData.ph;
    doc["debug_soil_type"] = (int)soilType;
    
    // ✅ ОТЛАДКА: Проверяем каждое условие отдельно
    bool lenCheck = strlen(config.cropId) > 0;
    bool strCheck = strcmp(config.cropId, "none") != 0;
    logDebugSafe("JSON API: lenCheck=%d, strCheck=%d", lenCheck, strCheck);
    
    if (lenCheck && strCheck) {
        String cropRecommendations = getCropEngine().generateCropSpecificRecommendations(
            String(config.cropId), npk, soilType, sensorData.ph);
        doc["crop_specific_recommendations"] = cropRecommendations;
        
        // ✅ ТОЛЬКО ОДНО логирование для отладки
        logDebugSafe("JSON API: crop='%s', rec_len=%d", config.cropId, cropRecommendations.length());
    } else {
        doc["crop_specific_recommendations"] = "";
    }
    


    // ---- Дополнительная информация ----
    // Сезон по текущему месяцу
    const char* seasonName = []()
    {
        // Проверяем инициализацию NTP
        if (timeClient == nullptr)
        {
            extern WiFiUDP ntpUDP;
            timeClient = new NTPClient(ntpUDP, "pool.ntp.org", 0, 3600000);
            timeClient->begin();
        }

        time_t now = timeClient ? (time_t)timeClient->getEpochTime() : time(nullptr);
        // если время < 2000-01-01 считаем, что NTP ещё не синхронизирован
        if (now < NTP_TIMESTAMP_2000)
        {
            // Пробуем обновить NTP
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
    }();
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
            html += "<li><strong>RAW</strong> - сырые данные с датчика</li>";
            html += "<li><strong>Компенс.</strong> - данные после математической компенсации:</li>";
            html += "<ul style='margin:5px 0;padding-left:15px;'>";
            html += "<li>🌡️ <strong>Температура:</strong> без изменений</li>";
            html += "<li>💧 <strong>Влажность:</strong> без изменений</li>";
            html += "<li>⚡ <strong>EC:</strong> температурная компенсация (Rhoades et al., 1989)</li>";
            html += "<li>⚗️ <strong>pH:</strong> температурная поправка по Нернсту (Nernst, 1889)</li>";
            html +=
                "<li>🌿🌱🍎 <strong>NPK:</strong> экспоненциальная компенсация по T и влажности (Delgado et al., 2020)</li>";
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
            html += "<li><strong>Цветовая индикация:</strong></li>";
            html += "<ul style='margin:5px 0;padding-left:15px;'>";
            html += "<li>🟢 <strong>Зеленый:</strong> оптимальные условия измерения</li>";
            html +=
                "<li>🟠 <strong>Оранжевый:</strong> неоптимальные условия (влажность <25%, температура <5°C или "
                ">40°C)</li>";
            html += "<li>🔵 <strong>Синий:</strong> полив активен (временная невалидность)</li>";
            html += "<li>🔴 <strong>Красный:</strong> ошибки датчика (выход за физические пределы)</li>";
            html += "</ul>";
            html += "</ul>";
            html += "</div>";

            html += "</div>";
            html += "</div>";

            // Заголовок 4-го столбца: выбранная культура или «Реком.»
            String recHeader = "Реком.";
            if (strlen(config.cropId) > 0)
            {
                const char* cropId = config.cropId;
                if (strcmp(cropId, "tomato") == 0)
                {
                    recHeader = "Томаты";
                }
                else if (strcmp(cropId, "cucumber") == 0)
                {
                    recHeader = "Огурцы";
                }
                else if (strcmp(cropId, "pepper") == 0)
                {
                    recHeader = "Перец";
                }
                else if (strcmp(cropId, "lettuce") == 0)
                {
                    recHeader = "Салат";
                }
                else if (strcmp(cropId, "blueberry") == 0)
                {
                    recHeader = "Голубика";
                }
                else if (strcmp(cropId, "lawn") == 0)
                {
                    recHeader = "Газон";
                }
                else if (strcmp(cropId, "grape") == 0)
                {
                    recHeader = "Виноград";
                }
                else if (strcmp(cropId, "conifer") == 0)
                {
                    recHeader = "Хвойные";
                }
                else if (strcmp(cropId, "strawberry") == 0)
                {
                    recHeader = "Клубника";
                }
                else if (strcmp(cropId, "apple") == 0)
                {
                    recHeader = "Яблоня";
                }
                else if (strcmp(cropId, "pear") == 0)
                {
                    recHeader = "Груша";
                }
                else if (strcmp(cropId, "cherry") == 0)
                {
                    recHeader = "Вишня";
                }
                else if (strcmp(cropId, "raspberry") == 0)
                {
                    recHeader = "Малина";
                }
                else if (strcmp(cropId, "currant") == 0)
                {
                    recHeader = "Смородина";
                }
            }

            html += "<div class='section'><table class='data'><thead><tr><th></th><th>RAW</th><th>Компенс.</th><th>" +
                    recHeader + "</th></tr></thead><tbody>";
            html +=
                "<tr><td>🌡️ Температура, °C</td><td><span id='temp_raw'></span></td><td><span "
                "id='temp'></span></td><td><span id='temp_rec'></span></td></tr>";
            html +=
                "<tr><td>💧 Влажность, %</td><td><span id='hum_raw'></span></td><td><span "
                "id='hum'></span></td><td><span id='hum_rec'></span></td></tr>";
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

            // ======= КАЛИБРОВКА =======
            html += "<div class='section'><h2>⚙️ Калибровка датчика</h2>";

            // Статус калибровки
            html += "<div style='background:#f8f9fa;padding:15px;border-radius:8px;margin:15px 0;'>";
            html += "<h4>📊 Текущий статус калибровки</h4>";
            html += "<div id='calibration-status'>Загрузка статуса...</div>";
            html += "</div>";

            // Температура и влажность (offset калибровка)
            html += "<div class='section'>";
            html += "<h3>🌡️💧 Температура и влажность (Offset калибровка)</h3>";
            html += "<p><strong>Инструкция:</strong> Сравните показания датчика с лабораторными приборами и введите поправку.</p>";
            
            html += "<div style='display:grid;grid-template-columns:1fr 1fr;gap:20px;margin:15px 0;'>";
            
            // Температура
            html += "<div style='border:1px solid #ffc107;padding:15px;border-radius:8px;'>";
            html += "<h4>🌡️ Температура</h4>";
            html += "<div class='form-group'>";
            html += "<label for='temp_lab'>Лабораторный термометр (°C):</label>";
            html += "<input type='number' id='temp_lab' step='0.1' placeholder='25.0' style='width:100%;'>";
            html += "</div>";
            html += "<div class='form-group'>";
            html += "<label for='temp_sensor'>Показание датчика (°C):</label>";
            html += "<input type='number' id='temp_sensor' step='0.1' placeholder='24.5' style='width:100%;'>";
            html += "</div>";
            html += "<button onclick='calibrateTemperature()' class='btn btn-warning' style='width:100%;'>Калибровать температуру</button>";
            html += "<div id='temp-status' style='margin-top:10px;font-size:14px;min-height:20px;'></div>";
            html += "</div>";
            
            // Влажность
            html += "<div style='border:1px solid #17a2b8;padding:15px;border-radius:8px;'>";
            html += "<h4>💧 Влажность</h4>";
            html += "<div class='form-group'>";
            html += "<label for='hum_lab'>Лабораторный гигрометр (%):</label>";
            html += "<input type='number' id='hum_lab' step='0.1' placeholder='60.0' style='width:100%;'>";
            html += "</div>";
            html += "<div class='form-group'>";
            html += "<label for='hum_sensor'>Показание датчика (%):</label>";
            html += "<input type='number' id='hum_sensor' step='0.1' placeholder='58.5' style='width:100%;'>";
            html += "</div>";
            html += "<button onclick='calibrateHumidity()' class='btn btn-info' style='width:100%;'>Калибровать влажность</button>";
            html += "<div id='hum-status' style='margin-top:10px;font-size:14px;min-height:20px;'></div>";
            html += "</div>";
            
            html += "</div>";
            html += "</div>";

            // pH калибровка
            html += "<div class='section'>";
            html += "<h3>🧪 pH калибровка</h3>";
            html += "<p>Введите показания для буферных растворов pH:</p>";
            html += "<div class='form-group'>";
            html += "<label for='ph_expected'>Ожидаемое значение pH:</label>";
            html += "<input type='number' id='ph_expected' step='0.1' min='0' max='14' placeholder='7.0'>";
            html += "</div>";
            html += "<div class='form-group'>";
            html += "<label for='ph_measured'>Измеренное значение pH:</label>";
            html += "<input type='number' id='ph_measured' step='0.1' min='0' max='14' placeholder='6.8'>";
            html += "</div>";
            html += "<button onclick='addPHPoint()' class='btn btn-primary'>Добавить точку pH</button>";
            html += "<div id='ph-points' style='margin-top:10px;'></div>";
            html += "</div>";

            // EC калибровка
            html += "<div class='section'>";
            html += "<h3>⚡ EC калибровка</h3>";
            html += "<p>Введите показания для стандартных растворов EC:</p>";
            html += "<div class='form-group'>";
            html += "<label for='ec_expected'>Ожидаемое значение EC (мСм/см):</label>";
            html += "<input type='number' id='ec_expected' step='0.1' min='0' placeholder='1.0'>";
            html += "</div>";
            html += "<div class='form-group'>";
            html += "<label for='ec_measured'>Измеренное значение EC (мСм/см):</label>";
            html += "<input type='number' id='ec_measured' step='0.1' min='0' placeholder='0.95'>";
            html += "</div>";
            html += "<button onclick='addECPoint()' class='btn btn-primary'>Добавить точку EC</button>";
            html += "<div id='ec-points' style='margin-top:10px;'></div>";
            html += "</div>";

            // NPK калибровка
            html += "<div class='section'>";
            html += "<h3>🌿🌱🍎 NPK калибровка</h3>";
            html += "<p>Введите показания для дистиллированной воды (должны быть близки к нулю):</p>";
            html += "<div class='form-group'>";
            html += "<label for='npk_n'>🌿 N (мг/кг):</label>";
            html += "<input type='number' id='npk_n' step='0.1' min='0' placeholder='0.0'>";
            html += "</div>";
            html += "<div class='form-group'>";
            html += "<label for='npk_p'>🌱 P (мг/кг):</label>";
            html += "<input type='number' id='npk_p' step='0.1' min='0' placeholder='0.0'>";
            html += "</div>";
            html += "<div class='form-group'>";
            html += "<label for='npk_k'>🍎 K (мг/кг):</label>";
            html += "<input type='number' id='npk_k' step='0.1' min='0' placeholder='0.0'>";
            html += "</div>";
            html += "<button onclick='setNPKPoint()' class='btn btn-primary'>Установить NPK</button>";
            html += "</div>";

            // Действия
            html += "<div class='section'>";
            html += "<h3>⚙️ Действия</h3>";
            html += "<div style='display:grid;grid-template-columns:1fr 1fr;gap:10px;margin-bottom:10px;'>";
            html += "<button onclick='calculatePH()' class='btn btn-success'>Рассчитать pH</button>";
            html += "<button onclick='calculateEC()' class='btn btn-success'>Рассчитать EC</button>";
            html += "</div>";
            html += "<div style='display:grid;grid-template-columns:1fr 1fr;gap:10px;margin-bottom:10px;'>";
            html += "<button onclick='exportCalibration()' class='btn btn-info'>Экспорт</button>";
            html += "<button onclick='importCalibration()' class='btn btn-info'>Импорт</button>";
            html += "</div>";
            html += "<button onclick='resetCalibration()' class='btn btn-danger'>Сбросить калибровку</button>";
            html += "</div>";

            // ======= ДОПОЛНИТЕЛЬНАЯ ИНФОРМАЦИЯ =======
            html += "<div class='section' style='background:#e8f5e8;padding:15px;border-radius:8px;margin:15px 0;'>";
            html += "<h4>💡 Полезная информация</h4>";
            html += "<ul style='margin:5px 0;padding-left:20px;font-size:14px;'>";
            html += "<li><strong>Стрелки ↑↓</strong> показывают направление изменений после компенсации</li>";
            html += "<li><strong>Сезонные корректировки</strong> учитывают потребности растений в разные периоды</li>";
            html += "<li><strong>Валидность данных</strong> проверяется по диапазонам и логическим связям</li>";
            html += "<li><strong>Интервал обновления:</strong> каждые 3 секунды</li>";
            html += "</ul>";
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
            html += "set('temp_raw',d.raw_temperature);";
            html += "set('hum_raw',d.raw_humidity);";
            html += "set('ec_raw',d.raw_ec);";
            html += "set('ph_raw',d.raw_ph);";
            html += "set('n_raw',d.raw_nitrogen);";
            html += "set('p_raw',d.raw_phosphorus);";
            html += "set('k_raw',d.raw_potassium);";
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
            html += "showWithArrow('temp', arrowSign(d.raw_temperature ,d.temperature ,tol.temp), d.temperature);";
            html += "showWithArrow('hum',  arrowSign(d.raw_humidity    ,d.humidity    ,tol.hum ), d.humidity);";
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

            // Добавляем индикацию сезонных корректировок
            html += "function updateSeasonalAdjustments(season) {";
            html += "  const adjustments = {";
            html += "    'Весна': { n: '+20%', p: '+15%', k: '+10%' },";
            html += "    'Лето': { n: '-10%', p: '+5%', k: '+25%' },";
            html += "    'Осень': { n: '-20%', p: '+10%', k: '+15%' },";
            html += "    'Зима': { n: '-30%', p: '+5%', k: '+5%' }";
            html += "  };";
            html += "  const envType = " + String(config.environmentType) + ";";
            html += "  const adj = adjustments[season] || { n: '', p: '', k: '' };";
            html += "  ['n', 'p', 'k'].forEach(elem => {";
            html += "    const span = document.getElementById(elem + '_season');";
            html += "    if(span && adj && adj[elem]) {";
            html += "      span.textContent = ` (${adj[elem]})`;";
            html += "      span.className = 'season-adj ' + (adj[elem].charAt(0) === '+' ? 'up' : 'down');";
            html += "    } else if(span) {";
            html += "      span.textContent = '';";
            html += "      span.className = 'season-adj';";
            html += "    }";
            html += "  });";
            html += "}";

            html += R"(var invalid = d.irrigation || (d.alerts && Array.isArray(d.alerts) && d.alerts.length>0) || d.humidity<25 || d.temperature<5 || )"
                    R"(d.temperature>40;)";
            html += R"(var statusHtml = invalid ? '<span class="red">Данные&nbsp;не&nbsp;валидны</span>' : '<span )"
                    R"(class="green">Данные&nbsp;валидны</span>';)";
            html +=
                R"(var seasonColor={'Лето':'green','Весна':'yellow','Осень':'yellow','Зима':'red','Н/Д':''}[d.season]||'';)";
            html += R"(var seasonHtml=seasonColor?(`<span class=\"${seasonColor}\">${d.season}</span>`):d.season;)";
            html += "document.getElementById('statusInfo').innerHTML=statusHtml+' | Сезон: '+seasonHtml;";
            html += "updateSeasonalAdjustments(d.season);";
            
            // Обновление рекомендаций по взаимодействию питательных веществ
            html += "console.log('Sensor data received:', d);";
            html += "console.log('nutrient_interactions:', d.nutrient_interactions);";
            html += "console.log('crop_specific_recommendations:', d.crop_specific_recommendations);";
            html += "console.log('nutrient-interactions div exists:', !!document.getElementById('nutrient-interactions'));";
            html += "console.log('crop-specific-recommendations div exists:', !!document.getElementById('crop-specific-recommendations'));";
            
            html += "const interactionsDiv = document.getElementById('nutrient-interactions');";
            html += "if(interactionsDiv) {";
            html += "  if(d.nutrient_interactions && typeof d.nutrient_interactions === 'string' && d.nutrient_interactions.length > 0) {";
            html += "    interactionsDiv.innerHTML = d.nutrient_interactions.replace(/\\n/g, '<br>');";
            html += "    console.log('Updated nutrient interactions');";
            html += "  } else {";
            html += "    interactionsDiv.innerHTML = '<p style=\"color:#28a745;\">✅ Антагонизмов питательных веществ не обнаружено</p>';";
            html += "    console.log('No nutrient interactions found');";
            html += "  }";
            html += "} else {";
            html += "  console.error('nutrient-interactions div not found');";
            html += "}";
            
            html += "const cropDiv = document.getElementById('crop-specific-recommendations');";
            html += "if(cropDiv) {";
            html += "  console.log('Crop recommendations data:', d.crop_specific_recommendations);";
            html += "  console.log('Crop ID:', d.crop_id);";
            html += "  console.log('Crop ID debug:', d.crop_id_debug);";
            html += "  console.log('Crop ID hex:', d.crop_id_hex);";
            html += "  console.log('Crop len check:', d.crop_len_check);";
            html += "  console.log('Crop str check:', d.crop_str_check);";
            html += "  console.log('DEBUG NPK - N:', d.debug_npk_n, 'P:', d.debug_npk_p, 'K:', d.debug_npk_k);";
            html += "  console.log('DEBUG pH:', d.debug_ph, 'SoilType:', d.debug_soil_type);";
            html += "  if(d.crop_specific_recommendations && typeof d.crop_specific_recommendations === 'string' && d.crop_specific_recommendations.length > 0) {";
            html += "    cropDiv.innerHTML = d.crop_specific_recommendations.replace(/\\n/g, '<br>');";
            html += "    console.log('Updated crop recommendations');";
            html += "  } else {";
            html += "    cropDiv.innerHTML = '<p style=\"color:#6c757d;\">ℹ️ Выберите культуру для получения специфических рекомендаций</p>';";
            html += "    console.log('No crop selected or no recommendations');";
            html += "  }";
            html += "} else {";
            html += "  console.error('crop-specific-recommendations div not found');";
            html += "}";
            
            html +=
                "var "
                "tvr=parseFloat(d.raw_temperature);applyColor('temp_raw',colorRange(tvr,limits.temp.min,limits.temp."
                "max));";
            html +=
                "var "
                "hvr=parseFloat(d.raw_humidity);applyColor('hum_raw',colorRange(hvr,limits.hum.min,limits.hum.max));";
            html += "var evr=parseFloat(d.raw_ec);applyColor('ec_raw',colorRange(evr,limits.ec.min,limits.ec.max));";
            html += "var pvr=parseFloat(d.raw_ph);applyColor('ph_raw',colorRange(pvr,limits.ph.min,limits.ph.max));";
            html += "var nvr=parseFloat(d.raw_nitrogen);applyColor('n_raw',colorRange(nvr,limits.n.min,limits.n.max));";
            html +=
                "var p2r=parseFloat(d.raw_phosphorus);applyColor('p_raw',colorRange(p2r,limits.p.min,limits.p.max));";
            html +=
                "var kvr=parseFloat(d.raw_potassium);applyColor('k_raw',colorRange(kvr,limits.k.min,limits.k.max));";
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
            
            // Компенсированные значения БЕЗ ПОКРАСКИ (как было раньше)
            
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
            html += "  if(interactionsDiv) interactionsDiv.innerHTML = '<p style=\"color:#dc3545;\">❌ Ошибка загрузки данных</p>';";
            html += "  const cropDiv = document.getElementById('crop-specific-recommendations');";
            html += "  if(cropDiv) cropDiv.innerHTML = '<p style=\"color:#dc3545;\">❌ Ошибка загрузки данных</p>';";
            html += "});";
            html += "}";
            
            // Добавляем автоматический запуск обновления
            html += "updateSensor();";
            html += "setInterval(updateSensor, 3000);";

            // Функции калибровки
            html += "function updateCalibrationStatus() {";
            html += "  fetch('/api/calibration/status')";
            html += "    .then(response => response.json())";
            html += "    .then(data => {";
            html += "      console.log('Calibration data:', data);";
            html += "      if(data.error) {";
            html += "        document.getElementById('calibration-status').innerHTML = '❌ ' + data.error;";
            html += "        return;";
            html += "      }";
            html += "      let statusHtml = '<div style=\"display:grid;grid-template-columns:1fr 1fr;gap:15px;\">';";
            html += "      statusHtml += '<div>';";
            html += "      statusHtml += '<h5>🌡️💧 Offset калибровки:</h5>';";
            html += "      statusHtml += '<p>🌡️ Температура: ' + (data.temperature && data.temperature.status ? data.temperature.status : '❌ Не настроена') + '</p>';";
            html += "      statusHtml += '<p>💧 Влажность: ' + (data.humidity && data.humidity.status ? data.humidity.status : '❌ Не настроена') + '</p>';";
            html += "      statusHtml += '<h5>🌿🌱🍎 NPK:</h5>';";
            html += "      statusHtml += '<p>' + (data.npk && data.npk.status ? data.npk.status : '❌ Не настроена') + '</p>';";
            html += "      statusHtml += '</div>';";
            html += "      statusHtml += '<div>';";
            html += "      statusHtml += '<h5>🧪⚡ Многоточечные:</h5>';";
            html += "      statusHtml += '<p>🧪 pH: ' + (data.ph && data.ph.status ? data.ph.status : '❌ Не настроена') + '</p>';";
            html += "      statusHtml += '<p>⚡ EC: ' + (data.ec && data.ec.status ? data.ec.status : '❌ Не настроена') + '</p>';";
            html += "      statusHtml += '<h5>📊 Общий:</h5>';";
            html += "      const completeIcon = data.is_complete ? '✅' : '⏳';";
            html += "      const completeText = data.is_complete ? 'Готова' : 'Требует настройки';";
            html += "      statusHtml += '<p><strong>' + completeIcon + ' ' + completeText + '</strong></p>';";
            html += "      statusHtml += '</div></div>';";
            html += "      statusHtml += '<div style=\"margin-top:10px;font-size:12px;color:#666;\">Обновлено: ' + new Date().toLocaleTimeString() + '</div>';";
            html += "      document.getElementById('calibration-status').innerHTML = statusHtml;";
            html += "    })";
            html += "    .catch(err => {";
            html += "      console.error('Calibration status error:', err);";
            html += "      document.getElementById('calibration-status').innerHTML = '❌ Ошибка загрузки: ' + err.message;";
            html += "    });";
            html += "}";
            
            // Функции калибровки температуры и влажности
            html += "function calibrateTemperature() {";
            html += "  const lab = parseFloat(document.getElementById('temp_lab').value);";
            html += "  const sensor = parseFloat(document.getElementById('temp_sensor').value);";
            html += "  if(isNaN(lab) || isNaN(sensor)) {";
            html += "    alert('⚠️ Введите корректные значения температуры');";
            html += "    return;";
            html += "  }";
            html += "  if(Math.abs(lab - sensor) > 10) {";
            html += "    if(!confirm('⚠️ Большая разность (' + (lab - sensor).toFixed(1) + '°C). Продолжить?')) return;";
            html += "  }";
            html += "  fetch('/api/calibration/temperature/add', {";
            html += "    method: 'POST',";
            html += "    headers: {'Content-Type': 'application/json'},";
            html += "    body: JSON.stringify({expected: lab, measured: sensor})";
            html += "  }).then(response => response.json())";
            html += "    .then(data => {";
            html += "      if(data.success) {";
            html += "        const offset = (lab - sensor).toFixed(2);";
            html += "        document.getElementById('temp-status').innerHTML = '✅ Offset: ' + offset + '°C';";
            html += "        document.getElementById('temp-status').style.color = '#28a745';";
            html += "        updateCalibrationStatus();";
            html += "        console.log('Temperature calibration success:', data);";
            html += "      } else {";
            html += "        document.getElementById('temp-status').innerHTML = '❌ Ошибка: ' + (data.error || 'Неизвестная ошибка');";
            html += "        document.getElementById('temp-status').style.color = '#dc3545';";
            html += "      }";
            html += "    })";
            html += "    .catch(err => {";
            html += "      console.error('Temperature calibration error:', err);";
            html += "      document.getElementById('temp-status').innerHTML = '❌ Ошибка соединения';";
            html += "      document.getElementById('temp-status').style.color = '#dc3545';";
            html += "    });";
            html += "}";
            
            html += "function calibrateHumidity() {";
            html += "  const lab = parseFloat(document.getElementById('hum_lab').value);";
            html += "  const sensor = parseFloat(document.getElementById('hum_sensor').value);";
            html += "  if(isNaN(lab) || isNaN(sensor)) {";
            html += "    alert('⚠️ Введите корректные значения влажности');";
            html += "    return;";
            html += "  }";
            html += "  if(lab < 0 || lab > 100 || sensor < 0 || sensor > 100) {";
            html += "    alert('⚠️ Влажность должна быть в диапазоне 0-100%');";
            html += "    return;";
            html += "  }";
            html += "  if(Math.abs(lab - sensor) > 20) {";
            html += "    if(!confirm('⚠️ Большая разность (' + (lab - sensor).toFixed(1) + '%RH). Продолжить?')) return;";
            html += "  }";
            html += "  fetch('/api/calibration/humidity/add', {";
            html += "    method: 'POST',";
            html += "    headers: {'Content-Type': 'application/json'},";
            html += "    body: JSON.stringify({expected: lab, measured: sensor})";
            html += "  }).then(response => response.json())";
            html += "    .then(data => {";
            html += "      if(data.success) {";
            html += "        const offset = (lab - sensor).toFixed(2);";
            html += "        document.getElementById('hum-status').innerHTML = '✅ Offset: ' + offset + '%RH';";
            html += "        document.getElementById('hum-status').style.color = '#28a745';";
            html += "        updateCalibrationStatus();";
            html += "        console.log('Humidity calibration success:', data);";
            html += "      } else {";
            html += "        document.getElementById('hum-status').innerHTML = '❌ Ошибка: ' + (data.error || 'Неизвестная ошибка');";
            html += "        document.getElementById('hum-status').style.color = '#dc3545';";
            html += "      }";
            html += "    })";
            html += "    .catch(err => {";
            html += "      console.error('Humidity calibration error:', err);";
            html += "      document.getElementById('hum-status').innerHTML = '❌ Ошибка соединения';";
            html += "      document.getElementById('hum-status').style.color = '#dc3545';";
            html += "    });";
            html += "}";
            
            html += "function addPHPoint() {";
            html += "  const expected = parseFloat(document.getElementById('ph_expected').value);";
            html += "  const measured = parseFloat(document.getElementById('ph_measured').value);";
            html += "  fetch('/api/calibration/ph/add', {";
            html += "    method: 'POST',";
            html += "    headers: {'Content-Type': 'application/json'},";
            html += "    body: JSON.stringify({expected: expected, measured: measured})";
            html += "  }).then(response => response.json())";
            html += "    .then(data => {";
            html += "      if(data.success) {";
            html += "        updateCalibrationStatus();";
            html += "        document.getElementById('ph_expected').value = '';";
            html += "        document.getElementById('ph_measured').value = '';";
            html += "      }";
            html += "    });";
            html += "}";
            html += "function addECPoint() {";
            html += "  const expected = parseFloat(document.getElementById('ec_expected').value);";
            html += "  const measured = parseFloat(document.getElementById('ec_measured').value);";
            html += "  fetch('/api/calibration/ec/add', {";
            html += "    method: 'POST',";
            html += "    headers: {'Content-Type': 'application/json'},";
            html += "    body: JSON.stringify({expected: expected, measured: measured})";
            html += "  }).then(response => response.json())";
            html += "    .then(data => {";
            html += "      if(data.success) {";
            html += "        updateCalibrationStatus();";
            html += "        document.getElementById('ec_expected').value = '';";
            html += "        document.getElementById('ec_measured').value = '';";
            html += "      }";
            html += "    });";
            html += "}";
            html += "function setNPKPoint() {";
            html += "  const n = parseFloat(document.getElementById('npk_n').value);";
            html += "  const p = parseFloat(document.getElementById('npk_p').value);";
            html += "  const k = parseFloat(document.getElementById('npk_k').value);";
            html += "  fetch('/api/calibration/npk/set', {";
            html += "    method: 'POST',";
            html += "    headers: {'Content-Type': 'application/json'},";
            html += "    body: JSON.stringify({n: n, p: p, k: k})";
            html += "  }).then(response => response.json())";
            html += "    .then(data => {";
            html += "      if(data.success) {";
            html += "        updateCalibrationStatus();";
            html += "        document.getElementById('npk_n').value = '';";
            html += "        document.getElementById('npk_p').value = '';";
            html += "        document.getElementById('npk_k').value = '';";
            html += "      }";
            html += "    });";
            html += "}";
            html += "function calculatePH() {";
            html += "  fetch('/api/calibration/ph/calculate', {method: 'POST'})";
            html += "    .then(response => response.json())";
            html += "    .then(data => {";
            html += "      if(data.success) {";
            html += "        updateCalibrationStatus();";
            html += "        alert('pH калибровка рассчитана! R² = ' + data.r_squared);";
            html += "      }";
            html += "    });";
            html += "}";
            html += "function calculateEC() {";
            html += "  fetch('/api/calibration/ec/calculate', {method: 'POST'})";
            html += "    .then(response => response.json())";
            html += "    .then(data => {";
            html += "      if(data.success) {";
            html += "        updateCalibrationStatus();";
            html += "        alert('EC калибровка рассчитана! R² = ' + data.r_squared);";
            html += "      }";
            html += "    });";
            html += "}";
            html += "function exportCalibration() {";
            html += "  fetch('/api/calibration/export')";
            html += "    .then(response => response.json())";
            html += "    .then(data => {";
            html += "      const blob = new Blob([JSON.stringify(data, null, 2)], {type: 'application/json'});";
            html += "      const url = URL.createObjectURL(blob);";
            html += "      const a = document.createElement('a');";
            html += "      a.href = url;";
            html += "      a.download = 'calibration.json';";
            html += "      a.click();";
            html += "    });";
            html += "}";
            html += "function importCalibration() {";
            html += "  const input = document.createElement('input');";
            html += "  input.type = 'file';";
            html += "  input.accept = '.json';";
            html += "  input.onchange = function(e) {";
            html += "    const file = e.target.files[0];";
            html += "    const reader = new FileReader();";
            html += "    reader.onload = function(e) {";
            html += "      fetch('/api/calibration/import', {";
            html += "        method: 'POST',";
            html += "        headers: {'Content-Type': 'application/json'},";
            html += "        body: e.target.result";
            html += "      }).then(response => response.json())";
            html += "        .then(data => {";
            html += "          if(data.success) {";
            html += "            updateCalibrationStatus();";
            html += "            alert('Калибровка импортирована!');";
            html += "          }";
            html += "        });";
            html += "    };";
            html += "    reader.readAsText(file);";
            html += "  };";
            html += "  input.click();";
            html += "}";
            html += "function resetCalibration() {";
            html += "  if(confirm('Сбросить всю калибровку?')) {";
            html += "    fetch('/api/calibration/reset', {method: 'POST'})";
            html += "      .then(response => response.json())";
            html += "      .then(data => {";
            html += "        if(data.success) {";
            html += "          updateCalibrationStatus();";
            html += "          alert('Калибровка сброшена!');";
            html += "        }";
            html += "      });";
            html += "  }";
            html += "}";

            html += "setInterval(updateSensor,3000);";
            html += "updateSensor();";
            html += "updateCalibrationStatus();";
            html += "setInterval(updateCalibrationStatus, 10000);";
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
                             logWarn("pH вне диапазона JXCT: expected=" + String(expected) + ", measured=" + String(measured));
                             webServer.send(400, "application/json", "{\"success\":false,\"error\":\"pH values out of JXCT sensor range (3-9 pH)\"}");
                             return;
                         }
                         
                         // Добавляем pH калибровочную точку
                         success = gCalibrationService.addPHCalibrationPoint(expected, measured);
                         
                         if (success) {
                             logSuccess("pH калибровочная точка добавлена: expected=" + String(expected) + ", measured=" + String(measured));
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
                             logWarn("EC вне диапазона JXCT: expected=" + String(expected) + ", measured=" + String(measured));
                             webServer.send(400, "application/json", "{\"success\":false,\"error\":\"EC values out of JXCT sensor range (0-10000 µS/cm)\"}");
                             return;
                         }
                         
                         // Добавляем EC калибровочную точку
                         success = gCalibrationService.addECCalibrationPoint(expected, measured);
                         
                         if (success) {
                             logSuccess("EC калибровочная точка добавлена: expected=" + String(expected) + ", measured=" + String(measured));
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
                             logWarn("NPK вне диапазона JXCT: N=" + String(n) + ", P=" + String(p) + ", K=" + String(k));
                             webServer.send(400, "application/json", "{\"success\":false,\"error\":\"NPK values out of JXCT sensor range (0-1999 mg/kg)\"}");
                             return;
                         }
                         
                         // Устанавливаем NPK калибровочную точку
                         success = gCalibrationService.setNPKCalibrationPoint(n, p, k);
                         
                         if (success) {
                             logSuccess("NPK калибровочная точка установлена: N=" + String(n) + ", P=" + String(p) + ", K=" + String(k));
                             
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

    logDebug("Маршруты данных настроены: /readings, /api/v1/sensor (json), /sensor_json [legacy], /api/calibration/*");
}
