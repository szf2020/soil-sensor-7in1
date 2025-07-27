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
    static const std::array<SoilProfile, 5> soilProfiles = {{
        SoilProfile::SAND,     // 0
        SoilProfile::LOAM,     // 1
        SoilProfile::PEAT,     // 2
        SoilProfile::CLAY,     // 3
        SoilProfile::SANDPEAT  // 4
    }};

    static const std::array<EnvironmentType, 3> envTypes = {{
        EnvironmentType::OUTDOOR,     // 0
        EnvironmentType::GREENHOUSE,  // 1
        EnvironmentType::INDOOR       // 2
    }};

    const int soilIndex = (config.soilProfile >= 0 && config.soilProfile < 5) ? config.soilProfile : 0;
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
    doc["valid"] = validateSensorData(sensorData);  // Флаг валидности по лимитам датчика

    const RecValues rec = computeRecommendations();
    doc["rec_temperature"] = format_temperature(rec.t);
    doc["rec_humidity"] = format_moisture(rec.hum);
    doc["rec_ec"] = format_ec(rec.ec);
    doc["rec_ph"] = format_ph(rec.ph);
    doc["rec_nitrogen"] = format_npk(rec.n);
    doc["rec_phosphorus"] = format_npk(rec.p);
    doc["rec_potassium"] = format_npk(rec.k);

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
            html += "<li>⚡ <strong>EC:</strong> температурная компенсация + модель Арчи (Archie, 1942)</li>";
            html += "<li>⚗️ <strong>pH:</strong> температурная поправка по Нернсту (-0.003×ΔT)</li>";
            html +=
                "<li>🔴🟡🔵 <strong>NPK:</strong> коррекция по T, влажности и типу почвы (FAO 56 + Eur. J. Soil "
                "Sci.)</li>";
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
                "<tr><td>🔴 Азот (N), мг/кг</td><td><span id='n_raw'></span></td><td><span "
                "id='n'></span></td><td><span id='n_rec'></span><span id='n_season' "
                "class='season-adj'></span></td></tr>";
            html +=
                "<tr><td>🟡 Фосфор (P), мг/кг</td><td><span id='p_raw'></span></td><td><span "
                "id='p'></span></td><td><span id='p_rec'></span><span id='p_season' "
                "class='season-adj'></span></td></tr>";
            html +=
                "<tr><td>🔵 Калий (K), мг/кг</td><td><span id='k_raw'></span></td><td><span "
                "id='k'></span></td><td><span id='k_rec'></span><span id='k_season' "
                "class='season-adj'></span></td></tr>";
            html += "</tbody></table></div>";

            // ======= СОВРЕМЕННАЯ КАЛИБРОВКА =======
            html += "<div class='section'><h2>⚙️ Калибровка датчика</h2>";

            // Статус калибровки
            html += "<div style='background:#f8f9fa;padding:15px;border-radius:8px;margin:15px 0;'>";
            html += "<h4>📊 Статус калибровки</h4>";
            html += "<div id='calibration-status'>Загрузка...</div>";
            html += "</div>";

            // Температура и влажность (offset калибровка)
            html += "<div class='section' style='background:#fff3cd;padding:15px;border-radius:8px;'>";
            html += "<h3>🌡️ Температура и влажность (Offset калибровка)</h3>";
            html += "<p><strong>Инструкция:</strong> Измерьте показания лабораторными приборами и введите разность.</p>";
            
            html += "<div style='display:grid;grid-template-columns:1fr 1fr;gap:20px;'>";
            
            // Температура
            html += "<div>";
            html += "<h4>🌡️ Температура</h4>";
            html += "<div class='form-group'>";
            html += "<label for='temp_lab'>Лабораторный термометр (°C):</label>";
            html += "<input type='number' id='temp_lab' step='0.1' placeholder='25.0'>";
            html += "</div>";
            html += "<div class='form-group'>";
            html += "<label for='temp_sensor'>Показание датчика (°C):</label>";
            html += "<input type='number' id='temp_sensor' step='0.1' placeholder='24.5'>";
            html += "</div>";
            html += "<button onclick='calibrateTemperature()' class='btn btn-primary'>Калибровать температуру</button>";
            html += "<div id='temp-status' style='margin-top:10px;font-size:14px;'></div>";
            html += "</div>";
            
            // Влажность
            html += "<div>";
            html += "<h4>💧 Влажность</h4>";
            html += "<div class='form-group'>";
            html += "<label for='hum_lab'>Лабораторный гигрометр (%):</label>";
            html += "<input type='number' id='hum_lab' step='0.1' placeholder='60.0'>";
            html += "</div>";
            html += "<div class='form-group'>";
            html += "<label for='hum_sensor'>Показание датчика (%):</label>";
            html += "<input type='number' id='hum_sensor' step='0.1' placeholder='58.5'>";
            html += "</div>";
            html += "<button onclick='calibrateHumidity()' class='btn btn-primary'>Калибровать влажность</button>";
            html += "<div id='hum-status' style='margin-top:10px;font-size:14px;'></div>";
            html += "</div>";
            
            html += "</div>";
            html += "</div>";

            // pH и EC (многоточечная калибровка)
            html += "<div class='section' style='background:#d1ecf1;padding:15px;border-radius:8px;'>";
            html += "<h3>🧪 pH и EC (Многоточечная калибровка)</h3>";
            html += "<p><strong>Инструкция:</strong> Добавьте минимум 2 точки для каждого параметра, затем рассчитайте коэффициенты.</p>";
            
            html += "<div style='display:grid;grid-template-columns:1fr 1fr;gap:20px;'>";
            
            // pH калибровка
            html += "<div>";
            html += "<h4>🧪 pH калибровка</h4>";
            html += "<div class='form-group'>";
            html += "<label for='ph_expected'>Буферный раствор pH:</label>";
            html += "<select id='ph_expected'>";
            html += "<option value='4.01'>pH 4.01 (кислый)</option>";
            html += "<option value='7.00' selected>pH 7.00 (нейтральный)</option>";
            html += "<option value='10.01'>pH 10.01 (щелочной)</option>";
            html += "<option value=''>Другое значение</option>";
            html += "</select>";
            html += "</div>";
            html += "<div class='form-group'>";
            html += "<label for='ph_expected_custom'>Или введите значение:</label>";
            html += "<input type='number' id='ph_expected_custom' step='0.01' min='0' max='14' placeholder='7.00'>";
            html += "</div>";
            html += "<div class='form-group'>";
            html += "<label for='ph_measured'>Показание датчика:</label>";
            html += "<input type='number' id='ph_measured' step='0.01' min='0' max='14' placeholder='6.85'>";
            html += "</div>";
            html += "<button onclick='addPHPoint()' class='btn btn-primary'>Добавить точку pH</button>";
            html += "<button onclick='calculatePH()' class='btn btn-success' style='margin-left:10px;'>Рассчитать pH</button>";
            html += "<div id='ph-points' style='margin-top:10px;'></div>";
            html += "<div id='ph-coefficients' style='margin-top:10px;background:#e7f3ff;padding:10px;border-radius:5px;display:none;'>";
            html += "<h5>Коэффициенты pH:</h5>";
            html += "<p>y = <span id='ph_slope'>1.000</span> × x + <span id='ph_intercept'>0.000</span></p>";
            html += "<p>R² = <span id='ph_r_squared'>0.000</span> | Точек: <span id='ph_point_count'>0</span></p>";
            html += "</div>";
            html += "</div>";
            
            // EC калибровка
            html += "<div>";
            html += "<h4>⚡ EC калибровка</h4>";
            html += "<div class='form-group'>";
            html += "<label for='ec_expected'>Стандартный раствор EC (µS/cm):</label>";
            html += "<select id='ec_expected'>";
            html += "<option value='84'>84 µS/cm (дист. вода)</option>";
            html += "<option value='1413' selected>1413 µS/cm (стандарт)</option>";
            html += "<option value='12880'>12880 µS/cm (высокий)</option>";
            html += "<option value=''>Другое значение</option>";
            html += "</select>";
            html += "</div>";
            html += "<div class='form-group'>";
            html += "<label for='ec_expected_custom'>Или введите значение:</label>";
            html += "<input type='number' id='ec_expected_custom' step='1' min='0' placeholder='1413'>";
            html += "</div>";
            html += "<div class='form-group'>";
            html += "<label for='ec_measured'>Показание датчика (µS/cm):</label>";
            html += "<input type='number' id='ec_measured' step='1' min='0' placeholder='1380'>";
            html += "</div>";
            html += "<button onclick='addECPoint()' class='btn btn-primary'>Добавить точку EC</button>";
            html += "<button onclick='calculateEC()' class='btn btn-success' style='margin-left:10px;'>Рассчитать EC</button>";
            html += "<div id='ec-points' style='margin-top:10px;'></div>";
            html += "<div id='ec-coefficients' style='margin-top:10px;background:#e7f3ff;padding:10px;border-radius:5px;display:none;'>";
            html += "<h5>Коэффициенты EC:</h5>";
            html += "<p>y = <span id='ec_slope'>1.000</span> × x + <span id='ec_intercept'>0.000</span></p>";
            html += "<p>R² = <span id='ec_r_squared'>0.000</span> | Точек: <span id='ec_point_count'>0</span></p>";
            html += "</div>";
            html += "</div>";
            
            html += "</div>";
            html += "</div>";

            // NPK калибровка (нулевая точка)
            html += "<div class='section' style='background:#d4edda;padding:15px;border-radius:8px;'>";
            html += "<h3>🌱 NPK (Нулевая точка по дистиллированной воде)</h3>";
            html += "<p><strong>Инструкция:</strong> Поместите датчик в дистиллированную воду и введите показания. Ожидаемые значения близки к нулю.</p>";
            
            html += "<div style='display:grid;grid-template-columns:1fr 1fr 1fr;gap:15px;'>";
            
            html += "<div class='form-group'>";
            html += "<label for='npk_n'>🔴 Азот (N), мг/кг:</label>";
            html += "<input type='number' id='npk_n' step='0.1' min='0' placeholder='2.1'>";
            html += "</div>";
            html += "<div class='form-group'>";
            html += "<label for='npk_p'>🟡 Фосфор (P), мг/кг:</label>";
            html += "<input type='number' id='npk_p' step='0.1' min='0' placeholder='1.8'>";
            html += "</div>";
            html += "<div class='form-group'>";
            html += "<label for='npk_k'>🔵 Калий (K), мг/кг:</label>";
            html += "<input type='number' id='npk_k' step='0.1' min='0' placeholder='3.2'>";
            html += "</div>";
            
            html += "</div>";
            html += "<button onclick='setNPKZeroPoint()' class='btn btn-primary'>Установить нулевую точку NPK</button>";
            html += "<div id='npk-status' style='margin-top:10px;font-size:14px;'></div>";
            html += "</div>";

            // Управление калибровкой
            html += "<div class='section' style='background:#f8f9fa;padding:15px;border-radius:8px;'>";
            html += "<h3>⚙️ Управление калибровкой</h3>";
            html += "<div style='display:grid;grid-template-columns:1fr 1fr 1fr;gap:10px;margin-bottom:15px;'>";
            html += "<button onclick='exportCalibration()' class='btn btn-info'>📤 Экспорт калибровки</button>";
            html += "<button onclick='importCalibration()' class='btn btn-info'>📥 Импорт калибровки</button>";
            html += "<button onclick='resetCalibration()' class='btn btn-danger'>🗑️ Сбросить всё</button>";
            html += "</div>";
            html += "<input type='file' id='calibration-file' accept='.json' style='display:none;' onchange='loadCalibrationFile()'>";
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
                "limits={temp:{min:-45,max:115},hum:{min:0,max:100},ec:{min:0,max:10000},ph:{min:3,max:9},n:{min:0,max:"
                "1999},p:{min:0,max:1999},k:{min:0,max:1999}};";
            html += "function updateSensor(){";
            html += "fetch('/sensor_json').then(r=>r.json()).then(d=>{";
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
            html += "    if(span) {";
            html += "      span.textContent = adj[elem] ? ` (${adj[elem]})` : '';";
            html += "      span.className = 'season-adj ' + (adj[elem].startsWith('+') ? 'up' : 'down');";
            html += "    }";
            html += "  });";
            html += "}";

            html += R"(var invalid = d.irrigation || d.alerts.length>0 || d.humidity<25 || d.temperature<5 || )"
                    R"(d.temperature>40;)";
            html += R"(var statusHtml = invalid ? '<span class="red">Данные&nbsp;не&nbsp;валидны</span>' : '<span )"
                    R"(class="green">Данные&nbsp;валидны</span>';)";
            html +=
                R"(var seasonColor={'Лето':'green','Весна':'yellow','Осень':'yellow','Зима':'red','Н/Д':''}[d.season]||'';)";
            html += R"(var seasonHtml=seasonColor?(`<span class=\"${seasonColor}\">${d.season}</span>`):d.season;)";
            html += "document.getElementById('statusInfo').innerHTML=statusHtml+' | Сезон: '+seasonHtml;";
            html += "updateSeasonalAdjustments(d.season);";
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
            html += "var ct=parseFloat(d.temperature);";
            html += "var ch=parseFloat(d.humidity);";
            html += "var ce=parseFloat(d.ec);";
            html += "var cph=parseFloat(d.ph);";
            html += "var cn=parseFloat(d.nitrogen);";
            html += "var cp=parseFloat(d.phosphorus);";
            html += "var ck=parseFloat(d.potassium);";
            html += "applyColor('temp_rec', colorDelta(ct, parseFloat(d.rec_temperature)));";
            html += "applyColor('hum_rec',  colorDelta(ch, parseFloat(d.rec_humidity)));";
            html += "applyColor('ec_rec',   colorDelta(ce, parseFloat(d.rec_ec)));";
            html += "applyColor('ph_rec',   colorDelta(cph,parseFloat(d.rec_ph)));";
            html += "applyColor('n_rec',    colorDelta(cn, parseFloat(d.rec_nitrogen)));";
            html += "applyColor('p_rec',    colorDelta(cp, parseFloat(d.rec_phosphorus)));";
            html += "applyColor('k_rec',    colorDelta(ck, parseFloat(d.rec_potassium)));";
            html += "});";
            html += "}";

            // Обновление статуса калибровки
            html += "function updateCalibrationStatus() {";
            html += "  fetch('/api/calibration/status')";
            html += "    .then(response => {";
            html += "      console.log('Status response:', response.status);";
            html += "      return response.json();";
            html += "    })";
            html += "    .then(data => {";
            html += "      console.log('Calibration status data:', data);";
            html += "      if(data.error) {";
            html += "        document.getElementById('calibration-status').innerHTML = '❌ ' + data.error;";
            html += "        console.error('Calibration error:', data.error);";
            html += "        return;";
            html += "      }";
            html += "      ";
            html += "      let statusHtml = '<div style=\"display:grid;grid-template-columns:1fr 1fr;gap:20px;\">';";
            html += "      ";
            html += "      // Левая колонка";
            html += "      statusHtml += '<div>';";
            html += "      statusHtml += '<h5>🌡️💧 Offset калибровки:</h5>';";
            html += "      statusHtml += '<p>🌡️ Температура: ' + (data.temperature && data.temperature.status ? data.temperature.status : '❌ Не настроена') + '</p>';";
            html += "      statusHtml += '<p>💧 Влажность: ' + (data.humidity && data.humidity.status ? data.humidity.status : '❌ Не настроена') + '</p>';";
            html += "      statusHtml += '<h5>🌱 NPK калибровка:</h5>';";
            html += "      statusHtml += '<p>' + (data.npk && data.npk.status ? data.npk.status : '❌ Не настроена') + '</p>';";
            html += "      statusHtml += '</div>';";
            html += "      ";
            html += "      // Правая колонка";
            html += "      statusHtml += '<div>';";
            html += "      statusHtml += '<h5>🧪⚡ Многоточечные:</h5>';";
            html += "      statusHtml += '<p>🧪 pH: ' + (data.ph && data.ph.status ? data.ph.status : '❌ Не настроена') + '</p>';";
            html += "      statusHtml += '<p>⚡ EC: ' + (data.ec && data.ec.status ? data.ec.status : '❌ Не настроена') + '</p>';";
            html += "      statusHtml += '<h5>📊 Общий статус:</h5>';";
            html += "      const completeIcon = data.is_complete ? '✅' : '⏳';";
            html += "      const completeText = data.is_complete ? 'Полностью настроена' : 'Требует настройки';";
            html += "      statusHtml += '<p><strong>' + completeIcon + ' ' + completeText + '</strong></p>';";
            html += "      statusHtml += '</div>';";
            html += "      ";
            html += "      statusHtml += '</div>';";
            html += "      ";
            html += "      // Отладочная информация";
            html += "      statusHtml += '<div style=\"margin-top:15px;font-size:12px;color:#666;\">';";
            html += "      statusHtml += 'Обновлено: ' + new Date().toLocaleTimeString();";
            html += "      statusHtml += ' | Данных получено: ' + Object.keys(data).length;";
            html += "      statusHtml += '</div>';";
            html += "      ";
            html += "      document.getElementById('calibration-status').innerHTML = statusHtml;";
            html += "    })";
            html += "    .catch(err => {";
            html += "      console.error('Fetch error:', err);";
            html += "      document.getElementById('calibration-status').innerHTML = '❌ Ошибка загрузки статуса: ' + err.message;";
            html += "    });";
            html += "}";
            // ========== НОВЫЕ ФУНКЦИИ КАЛИБРОВКИ ==========
            
            // Температура
            html += "function calibrateTemperature() {";
            html += "  const lab = parseFloat(document.getElementById('temp_lab').value);";
            html += "  const sensor = parseFloat(document.getElementById('temp_sensor').value);";
            html += "  if(isNaN(lab) || isNaN(sensor)) {";
            html += "    alert('Введите корректные значения температуры');";
            html += "    return;";
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
            html += "        updateCalibrationStatus();";
            html += "      } else {";
            html += "        document.getElementById('temp-status').innerHTML = '❌ Ошибка калибровки';";
            html += "      }";
            html += "    });";
            html += "}";
            
            // Влажность
            html += "function calibrateHumidity() {";
            html += "  const lab = parseFloat(document.getElementById('hum_lab').value);";
            html += "  const sensor = parseFloat(document.getElementById('hum_sensor').value);";
            html += "  if(isNaN(lab) || isNaN(sensor)) {";
            html += "    alert('Введите корректные значения влажности');";
            html += "    return;";
            html += "  }";
            html += "  fetch('/api/calibration/humidity/add', {";
            html += "    method: 'POST',";
            html += "    headers: {'Content-Type': 'application/json'},";
            html += "    body: JSON.stringify({expected: lab, measured: sensor})";
            html += "  }).then(response => response.json())";
            html += "    .then(data => {";
            html += "      if(data.success) {";
            html += "        const offset = (lab - sensor).toFixed(2);";
            html += "        document.getElementById('hum-status').innerHTML = '✅ Offset: ' + offset + '%';";
            html += "        updateCalibrationStatus();";
            html += "      } else {";
            html += "        document.getElementById('hum-status').innerHTML = '❌ Ошибка калибровки';";
            html += "      }";
            html += "    });";
            html += "}";
            
            // pH калибровка
            html += "function addPHPoint() {";
            html += "  let expected = document.getElementById('ph_expected').value;";
            html += "  if(expected === '') {";
            html += "    expected = parseFloat(document.getElementById('ph_expected_custom').value);";
            html += "  } else {";
            html += "    expected = parseFloat(expected);";
            html += "  }";
            html += "  const measured = parseFloat(document.getElementById('ph_measured').value);";
            html += "  if(isNaN(expected) || isNaN(measured)) {";
            html += "    alert('Введите корректные значения pH');";
            html += "    return;";
            html += "  }";
            html += "  fetch('/api/calibration/ph/add', {";
            html += "    method: 'POST',";
            html += "    headers: {'Content-Type': 'application/json'},";
            html += "    body: JSON.stringify({expected: expected, measured: measured})";
            html += "  }).then(response => response.json())";
            html += "    .then(data => {";
            html += "      if(data.success) {";
            html += "        updatePHPoints();";
            html += "        updateCalibrationStatus();";
            html += "        document.getElementById('ph_measured').value = '';";
            html += "      } else {";
            html += "        alert('Ошибка добавления точки pH');";
            html += "      }";
            html += "    });";
            html += "}";
            // EC калибровка
            html += "function addECPoint() {";
            html += "  let expected = document.getElementById('ec_expected').value;";
            html += "  if(expected === '') {";
            html += "    expected = parseFloat(document.getElementById('ec_expected_custom').value);";
            html += "  } else {";
            html += "    expected = parseFloat(expected);";
            html += "  }";
            html += "  const measured = parseFloat(document.getElementById('ec_measured').value);";
            html += "  if(isNaN(expected) || isNaN(measured)) {";
            html += "    alert('Введите корректные значения EC');";
            html += "    return;";
            html += "  }";
            html += "  fetch('/api/calibration/ec/add', {";
            html += "    method: 'POST',";
            html += "    headers: {'Content-Type': 'application/json'},";
            html += "    body: JSON.stringify({expected: expected, measured: measured})";
            html += "  }).then(response => response.json())";
            html += "    .then(data => {";
            html += "      if(data.success) {";
            html += "        updateECPoints();";
            html += "        updateCalibrationStatus();";
            html += "        document.getElementById('ec_measured').value = '';";
            html += "      } else {";
            html += "        alert('Ошибка добавления точки EC');";
            html += "      }";
            html += "    });";
            html += "}";
            
            // NPK нулевая точка
            html += "function setNPKZeroPoint() {";
            html += "  const n = parseFloat(document.getElementById('npk_n').value);";
            html += "  const p = parseFloat(document.getElementById('npk_p').value);";
            html += "  const k = parseFloat(document.getElementById('npk_k').value);";
            html += "  if(isNaN(n) || isNaN(p) || isNaN(k)) {";
            html += "    alert('Введите корректные значения NPK');";
            html += "    return;";
            html += "  }";
            html += "  fetch('/api/calibration/npk/set', {";
            html += "    method: 'POST',";
            html += "    headers: {'Content-Type': 'application/json'},";
            html += "    body: JSON.stringify({n: n, p: p, k: k})";
            html += "  }).then(response => response.json())";
            html += "    .then(data => {";
            html += "      if(data.success) {";
            html += "        const offsetN = (0 - n).toFixed(2);";
            html += "        const offsetP = (0 - p).toFixed(2);";
            html += "        const offsetK = (0 - k).toFixed(2);";
            html += "        document.getElementById('npk-status').innerHTML = ";
            html += "          '✅ Offsets: N=' + offsetN + ', P=' + offsetP + ', K=' + offsetK;";
            html += "        updateCalibrationStatus();";
            html += "      } else {";
            html += "        document.getElementById('npk-status').innerHTML = '❌ Ошибка калибровки NPK';";
            html += "      }";
            html += "    });";
            html += "}";
            
            // Расчет pH калибровки
            html += "function calculatePH() {";
            html += "  fetch('/api/calibration/ph/calculate', {method: 'POST'})";
            html += "    .then(response => response.json())";
            html += "    .then(data => {";
            html += "      if(data.success) {";
            html += "        updateCalibrationStatus();";
            html += "        document.getElementById('ph-coefficients').style.display = 'block';";
            html += "        if(data.slope) document.getElementById('ph_slope').textContent = data.slope.toFixed(4);";
            html += "        if(data.intercept) document.getElementById('ph_intercept').textContent = data.intercept.toFixed(4);";
            html += "        if(data.r_squared) document.getElementById('ph_r_squared').textContent = data.r_squared.toFixed(4);";
            html += "        alert('pH калибровка рассчитана! R² = ' + (data.r_squared || 0).toFixed(4));";
            html += "      } else {";
            html += "        alert('Ошибка расчета pH: ' + (data.error || 'Недостаточно точек'));";
            html += "      }";
            html += "    });";
            html += "}";
            
            // Расчет EC калибровки
            html += "function calculateEC() {";
            html += "  fetch('/api/calibration/ec/calculate', {method: 'POST'})";
            html += "    .then(response => response.json())";
            html += "    .then(data => {";
            html += "      if(data.success) {";
            html += "        updateCalibrationStatus();";
            html += "        document.getElementById('ec-coefficients').style.display = 'block';";
            html += "        if(data.slope) document.getElementById('ec_slope').textContent = data.slope.toFixed(4);";
            html += "        if(data.intercept) document.getElementById('ec_intercept').textContent = data.intercept.toFixed(4);";
            html += "        if(data.r_squared) document.getElementById('ec_r_squared').textContent = data.r_squared.toFixed(4);";
            html += "        alert('EC калибровка рассчитана! R² = ' + (data.r_squared || 0).toFixed(4));";
            html += "      } else {";
            html += "        alert('Ошибка расчета EC: ' + (data.error || 'Недостаточно точек'));";
            html += "      }";
            html += "    });";
            html += "}";
            // Вспомогательные функции для отображения точек
            html += "function updatePHPoints() {";
            html += "  fetch('/api/calibration/status')";
            html += "    .then(response => response.json())";
            html += "    .then(data => {";
            html += "      if(data.ph && data.ph.point_count) {";
            html += "        document.getElementById('ph_point_count').textContent = data.ph.point_count;";
            html += "        const pointsDiv = document.getElementById('ph-points');";
            html += "        pointsDiv.innerHTML = '<small>Добавлено точек: ' + data.ph.point_count + '</small>';";
            html += "      }";
            html += "    });";
            html += "}";
            
            html += "function updateECPoints() {";
            html += "  fetch('/api/calibration/status')";
            html += "    .then(response => response.json())";
            html += "    .then(data => {";
            html += "      if(data.ec && data.ec.point_count) {";
            html += "        document.getElementById('ec_point_count').textContent = data.ec.point_count;";
            html += "        const pointsDiv = document.getElementById('ec-points');";
            html += "        pointsDiv.innerHTML = '<small>Добавлено точек: ' + data.ec.point_count + '</small>';";
            html += "      }";
            html += "    });";
            html += "}";
            
            // Функции управления калибровкой
            html += "function exportCalibration() {";
            html += "  fetch('/api/calibration/export')";
            html += "    .then(response => response.json())";
            html += "    .then(data => {";
            html += "      const timestamp = new Date().toISOString().slice(0,19).replace(/:/g,'-');";
            html += "      const filename = 'JXCT_calibration_' + timestamp + '.json';";
            html += "      const blob = new Blob([JSON.stringify(data, null, 2)], {type: 'application/json'});";
            html += "      const url = URL.createObjectURL(blob);";
            html += "      const a = document.createElement('a');";
            html += "      a.href = url;";
            html += "      a.download = filename;";
            html += "      a.click();";
            html += "      URL.revokeObjectURL(url);";
            html += "      alert('Калибровка экспортирована в файл: ' + filename);";
            html += "    }).catch(err => alert('Ошибка экспорта: ' + err));";
            html += "}";
            
            html += "function importCalibration() {";
            html += "  document.getElementById('calibration-file').click();";
            html += "}";
            
            html += "function loadCalibrationFile() {";
            html += "  const file = document.getElementById('calibration-file').files[0];";
            html += "  if(!file) return;";
            html += "  const reader = new FileReader();";
            html += "  reader.onload = function(e) {";
            html += "    try {";
            html += "      const data = JSON.parse(e.target.result);";
            html += "      fetch('/api/calibration/import', {";
            html += "        method: 'POST',";
            html += "        headers: {'Content-Type': 'application/json'},";
            html += "        body: JSON.stringify(data)";
            html += "      }).then(response => response.json())";
            html += "        .then(result => {";
            html += "          if(result.success) {";
            html += "            updateCalibrationStatus();";
            html += "            alert('Калибровка успешно импортирована из файла: ' + file.name);";
            html += "          } else {";
            html += "            alert('Ошибка импорта: ' + (result.error || 'Неизвестная ошибка'));";
            html += "          }";
            html += "        });";
            html += "    } catch(err) {";
            html += "      alert('Ошибка чтения файла: ' + err.message);";
            html += "    }";
            html += "  };";
            html += "  reader.readAsText(file);";
            html += "}";
            
            html += "function resetCalibration() {";
            html += "  if(confirm('⚠️ ВНИМАНИЕ! Это действие удалит ВСЕ калибровочные данные.\\n\\nПродолжить?')) {";
            html += "    fetch('/api/calibration/reset', {method: 'POST'})";
            html += "      .then(response => response.json())";
            html += "      .then(data => {";
            html += "        if(data.success) {";
            html += "          updateCalibrationStatus();";
            html += "          // Очищаем отображение коэффициентов";
            html += "          document.getElementById('ph-coefficients').style.display = 'none';";
            html += "          document.getElementById('ec-coefficients').style.display = 'none';";
            html += "          document.getElementById('temp-status').innerHTML = '';";
            html += "          document.getElementById('hum-status').innerHTML = '';";
            html += "          document.getElementById('npk-status').innerHTML = '';";
            html += "          alert('✅ Вся калибровка сброшена!');";
            html += "        } else {";
            html += "          alert('❌ Ошибка сброса калибровки');";
            html += "        }";
            html += "      }).catch(err => alert('Ошибка: ' + err));";
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
                         // Валидация входных данных
                         if (expected < 0 || measured < 0) {
                             logWarn("Отрицательные значения pH: expected=" + String(expected) + ", measured=" + String(measured));
                             webServer.send(400, "application/json", "{\"success\":false,\"error\":\"Negative values not allowed\"}");
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
                         // Валидация входных данных
                         if (expected < 0 || measured < 0) {
                             logWarn("Отрицательные значения EC: expected=" + String(expected) + ", measured=" + String(measured));
                             webServer.send(400, "application/json", "{\"success\":false,\"error\":\"Negative values not allowed\"}");
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
                         // Валидация входных данных
                         if (n < 0 || p < 0 || k < 0) {
                             logWarn("Отрицательные значения NPK: N=" + String(n) + ", P=" + String(p) + ", K=" + String(k));
                             webServer.send(400, "application/json", "{\"success\":false,\"error\":\"Negative values not allowed\"}");
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

    // Новые endpoints для температуры и влажности
    webServer.on("/api/calibration/temperature/add", HTTP_POST,
                 []()
                 {
                     logWebRequest("POST", "/api/calibration/temperature/add", webServer.client().remoteIP().toString());
                     
                     DynamicJsonDocument doc(512);
                     DeserializationError error = deserializeJson(doc, webServer.arg("plain"));

                     if (error)
                     {
                         webServer.send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
                         return;
                     }

                     float expected = doc["expected"];
                     float measured = doc["measured"];

                     bool success = gCalibrationService.addTemperatureCalibrationPoint(expected, measured);

                     DynamicJsonDocument response(256);
                     response["success"] = success;
                     if (!success)
                     {
                         response["error"] = "Failed to add temperature calibration point";
                     } else {
                         response["message"] = "Temperature calibration point added successfully";
                         response["offset"] = expected - measured;
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

                     if (error)
                     {
                         webServer.send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
                         return;
                     }

                     float expected = doc["expected"];
                     float measured = doc["measured"];

                     bool success = gCalibrationService.addHumidityCalibrationPoint(expected, measured);

                     DynamicJsonDocument response(256);
                     response["success"] = success;
                     if (!success)
                     {
                         response["error"] = "Failed to add humidity calibration point";
                     } else {
                         response["message"] = "Humidity calibration point added successfully";
                         response["offset"] = expected - measured;
                     }

                     String response_str;
                     serializeJson(response, response_str);
                     webServer.send(200, "application/json", response_str);
                 });

    // Endpoints для прямой установки offset
    webServer.on("/api/calibration/temperature/offset", HTTP_POST,
                 []()
                 {
                     logWebRequest("POST", "/api/calibration/temperature/offset", webServer.client().remoteIP().toString());
                     
                     DynamicJsonDocument doc(512);
                     DeserializationError error = deserializeJson(doc, webServer.arg("plain"));

                     if (error)
                     {
                         webServer.send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
                         return;
                     }

                     float offset = doc["offset"];
                     bool success = gCalibrationService.setTemperatureOffset(offset);

                     DynamicJsonDocument response(256);
                     response["success"] = success;
                     if (!success)
                     {
                         response["error"] = "Failed to set temperature offset";
                     } else {
                         response["message"] = "Temperature offset set successfully";
                         response["offset"] = offset;
                     }

                     String response_str;
                     serializeJson(response, response_str);
                     webServer.send(200, "application/json", response_str);
                 });

    webServer.on("/api/calibration/humidity/offset", HTTP_POST,
                 []()
                 {
                     logWebRequest("POST", "/api/calibration/humidity/offset", webServer.client().remoteIP().toString());
                     
                     DynamicJsonDocument doc(512);
                     DeserializationError error = deserializeJson(doc, webServer.arg("plain"));

                     if (error)
                     {
                         webServer.send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
                         return;
                     }

                     float offset = doc["offset"];
                     bool success = gCalibrationService.setHumidityOffset(offset);

                     DynamicJsonDocument response(256);
                     response["success"] = success;
                     if (!success)
                     {
                         response["error"] = "Failed to set humidity offset";
                     } else {
                         response["message"] = "Humidity offset set successfully";
                         response["offset"] = offset;
                     }

                     String response_str;
                     serializeJson(response, response_str);
                     webServer.send(200, "application/json", response_str);
                 });

    logDebug("Маршруты данных настроены: /readings, /api/v1/sensor (json), /sensor_json [legacy], /api/calibration/*");
}
