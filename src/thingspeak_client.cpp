#include "thingspeak_client.h"
#include <NTPClient.h>
#include <ThingSpeak.h>
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <array>
#include <cctype>
#include <cmath>
#include "jxct_config_vars.h"
#include "jxct_device_info.h"
#include "jxct_format_utils.h"
#include "logger.h"
#include "modbus_sensor.h"
#include "wifi_manager.h"
#include "business/sensor_compensation_service.h"
#include "sensor_processing.h"
extern NTPClient* timeClient;

namespace
{
// URL для отправки данных в ThingSpeak
const char* THINGSPEAK_API_URL = "https://api.thingspeak.com/update";

unsigned long lastTsPublish = 0;
unsigned long lastFailTime = 0;  // ✅ ДОБАВЛЕНО: Время последней ошибки
unsigned long nextThingSpeakTry = 0;  // ✅ ДОБАВЛЕНО: Время следующей попытки (исправляет переполнение)
int consecutiveFailCount = 0;  // счётчик подряд неудач

// ✅ ДОБАВЛЕНО: Функция валидации данных датчика
bool validateSensorData(const SensorData& data)
{
    // Проверяем на NaN и Inf
    if (std::isnan(data.temperature) || std::isinf(data.temperature) ||
        std::isnan(data.humidity) || std::isinf(data.humidity) ||
        std::isnan(data.ec) || std::isinf(data.ec) ||
        std::isnan(data.ph) || std::isinf(data.ph) ||
        std::isnan(data.nitrogen) || std::isinf(data.nitrogen) ||
        std::isnan(data.phosphorus) || std::isinf(data.phosphorus) ||
        std::isnan(data.potassium) || std::isinf(data.potassium)) {
        return false;
    }

    // Проверяем диапазоны значений (разрешаем отрицательные температуры)
    if (data.temperature < -40 || data.temperature > 85 ||
        data.humidity < 0 || data.humidity > 100 ||
        data.ec < 0 || data.ec > 10000 ||
        data.ph < 0 || data.ph > 14 ||
        data.nitrogen < 0 || data.nitrogen > 9999 ||
        data.phosphorus < 0 || data.phosphorus > 9999 ||
        data.potassium < 0 || data.potassium > 9999) {
        return false;
    }

    return true;
}

// Утилита для обрезки пробелов в начале/конце строки C
void trim(char* str)
{
    if (str == nullptr)
    {
        return;
    }
    // Trim leading
    char* ptr = str;
    while (*ptr && isspace((unsigned char)*ptr))
    {
        ++ptr;
    }
    if (ptr != str)
    {
        memmove(str, ptr, strlen(ptr) + 1);
    }

    // Trim trailing
    size_t len = strlen(str);
    while (len > 0 && isspace((unsigned char)str[len - 1]))
    {
        str[--len] = '\0';
    }
}

// ✅ Заменяем String на статические буферы
std::array<char, 32> thingSpeakLastPublishBuffer = {"0"};
// Увеличиваем буфер ошибки, чтобы не обрезать текст причины (ранее 64)
std::array<char, 128> thingSpeakLastErrorBuffer = {""};

// Используем отдельный WiFiClient для ThingSpeak, чтобы не конфликтовать с MQTT
static WiFiClient thingSpeakClient;
}  // namespace

// ✅ ДОБАВЛЕНО: Функция принудительного сброса блокировки ThingSpeak
void resetThingSpeakBlock()
{
    consecutiveFailCount = 0;
    lastFailTime = 0;
    nextThingSpeakTry = 0;  // Сбрасываем время следующей попытки
    thingSpeakLastErrorBuffer[0] = '\0';
    logSuccess("ThingSpeak: Блокировка принудительно сброшена");
}

// ✅ ДОБАВЛЕНО: Функция диагностики состояния ThingSpeak
void diagnoseThingSpeakStatus()
{
    const unsigned long now = millis();
    const unsigned long timeSinceLastFail = (lastFailTime > 0) ? (now - lastFailTime) : 0;
    const unsigned long timeSinceLastPublish = (lastTsPublish > 0) ? (now - lastTsPublish) : 0;
    
    logSystem("=== ДИАГНОСТИКА THINGSPEAK ===");
    logSystemSafe("Включен: %s", config.flags.thingSpeakEnabled ? "ДА" : "НЕТ");
    logSystemSafe("WiFi статус: %s", wifiConnected ? "ПОДКЛЮЧЕН" : "ОТКЛЮЧЕН");
    logSystemSafe("Данные валидны: %s", sensorData.valid ? "ДА" : "НЕТ");
    logSystemSafe("Счетчик ошибок: %d", consecutiveFailCount);
    logSystemSafe("Время последней ошибки: %lu мс назад", timeSinceLastFail);
    logSystemSafe("Время последней публикации: %lu мс назад", timeSinceLastPublish);
    logSystemSafe("Интервал отправки: %lu мс", (unsigned long)config.thingSpeakInterval);
    
    if (consecutiveFailCount >= 5) {
        const unsigned long remainingBlockTime = 1800000UL - timeSinceLastFail;
        logWarnSafe("БЛОКИРОВКА АКТИВНА! Осталось: %lu мс (%lu мин)", 
                   remainingBlockTime, remainingBlockTime / 60000);
    }
    
    if (strlen(thingSpeakLastErrorBuffer.data()) > 0) {
        logWarnSafe("Последняя ошибка: %s", thingSpeakLastErrorBuffer.data());
    }
    
    logSystem("================================");
}

// ✅ ДОБАВЛЕНО: Получение диагностики ThingSpeak в JSON формате
String getThingSpeakDiagnosticsJson()
{
    const unsigned long now = millis();
    const unsigned long timeSinceLastFail = (lastFailTime > 0) ? (now - lastFailTime) : 0;
    const unsigned long timeSinceLastPublish = (lastTsPublish > 0) ? (now - lastTsPublish) : 0;
    const unsigned long remainingBlockTime = (consecutiveFailCount >= 5) ? (1800000UL - timeSinceLastFail) : 0;
    
    String json = "{";
    json += "\"enabled\":" + String(config.flags.thingSpeakEnabled ? "true" : "false") + ",";
    json += "\"wifi_connected\":" + String(wifiConnected ? "true" : "false") + ",";
    json += "\"data_valid\":" + String(sensorData.valid ? "true" : "false") + ",";
    json += "\"consecutive_fail_count\":" + String(consecutiveFailCount) + ",";
    json += "\"time_since_last_fail_ms\":" + String(timeSinceLastFail) + ",";
    json += "\"time_since_last_publish_ms\":" + String(timeSinceLastPublish) + ",";
    json += "\"interval_ms\":" + String((unsigned long)config.thingSpeakInterval) + ",";
    json += "\"blocked\":" + String((consecutiveFailCount >= 5 && timeSinceLastFail < 1800000UL) ? "true" : "false") + ",";
    json += "\"remaining_block_time_ms\":" + String(remainingBlockTime) + ",";
    json += "\"remaining_block_time_min\":" + String(remainingBlockTime / 60000) + ",";
    json += "\"last_error\":\"" + String(thingSpeakLastErrorBuffer.data()) + "\",";
    json += "\"last_publish\":\"" + String(thingSpeakLastPublishBuffer.data()) + "\"";
    json += "}";
    
    return json;
}

// Геттеры для совместимости с внешним кодом
const char* getThingSpeakLastPublish()
{
    return thingSpeakLastPublishBuffer.data();
}
const char* getThingSpeakLastError()
{
    return thingSpeakLastErrorBuffer.data();
}

// ✅ ДОБАВЛЕНО: Функция проверки возможности отправки в ThingSpeak
bool canSendToThingSpeak()
{
    // Проверки базовых условий
    if (!config.flags.thingSpeakEnabled) {
        return false;
    }
    if (!wifiConnected) {
        return false;
    }
    if (!sensorData.valid) {
        return false;
    }

    const unsigned long now = millis();
    
    // ✅ ДОБАВЛЕНО: Автоматический сброс блокировки при восстановлении WiFi
    if (consecutiveFailCount >= 5 && (now - lastFailTime) >= 1800000UL) {
        logSuccess("ThingSpeak: Блокировка автоматически сброшена (прошло 30 минут)");
        consecutiveFailCount = 0;
        lastFailTime = 0;
        nextThingSpeakTry = 0;  // Сбрасываем время следующей попытки
        thingSpeakLastErrorBuffer[0] = '\0';
    }
    
    // ✅ ДОБАВЛЕНО: Сброс блокировки при стабильном WiFi соединении
    if (consecutiveFailCount >= 5 && WiFi.status() == WL_CONNECTED && (now - lastFailTime) >= 300000UL) {
        logSuccess("ThingSpeak: Блокировка сброшена (стабильное WiFi, прошло 5 мин)");
        consecutiveFailCount = 0;
        lastFailTime = 0;
        nextThingSpeakTry = 0;  // Сбрасываем время следующей попытки
        thingSpeakLastErrorBuffer[0] = '\0';
    }
    
    // Проверяем ограничение на 30 минут при множественных ошибках
    if (consecutiveFailCount >= 5 && (now - lastFailTime) < 1800000UL) {
        return false;
    }

    // ✅ ДОБАВЛЕНО: Проверка времени следующей попытки (wrap-safe)
    if (nextThingSpeakTry && (long)(now - nextThingSpeakTry) < 0) {
        return false;
    }

    // ✅ Единая проверка интервала с минимумом 20с (wrap-safe)
    unsigned long effectiveInterval = config.thingSpeakInterval;
    if (effectiveInterval < 20000UL) {
        effectiveInterval = 20000UL;
    }
    if ((unsigned long)(now - lastTsPublish) < effectiveInterval) {
        return false;
    }

    return true;
}

void setupThingSpeak(WiFiClient& /*client*/)  // NOLINT(misc-use-internal-linkage)
{
    // Инициализируем библиотеку на отдельном клиенте, чтобы исключить конкуренцию с MQTT
    ThingSpeak.begin(thingSpeakClient);
}

bool sendDataToThingSpeak()
{
    // ✅ ДОБАВЛЕНО: Подробная диагностика входа в функцию
    logDebug("ThingSpeak: Попытка отправки данных");
    logDebugSafe("ThingSpeak: enabled=%d, wifi=%d, data_valid=%d", 
                 (int)config.flags.thingSpeakEnabled, wifiConnected, sensorData.valid);
    
    // Проверки
    if (!config.flags.thingSpeakEnabled)
    {
        logDebug("ThingSpeak: Отключен в настройках");
        return false;
    }
    if (!wifiConnected)
    {
        logDebug("ThingSpeak: WiFi не подключен");
        return false;
    }
    if (!sensorData.valid)
    {
        logDebug("ThingSpeak: Данные датчика невалидны");
        return false;
    }

    const unsigned long now = millis();
    // Частотные ограничения проверяются в canSendToThingSpeak()

    std::array<char, 25> apiKeyBuf;
    std::array<char, 16> channelBuf;
    strlcpy(apiKeyBuf.data(), config.thingSpeakApiKey, apiKeyBuf.size());
    strlcpy(channelBuf.data(), config.thingSpeakChannelId, channelBuf.size());
    trim(apiKeyBuf.data());
    trim(channelBuf.data());

    const unsigned long channelId = strtoul(channelBuf.data(), nullptr, 10);

    // ✅ ДОБАВЛЕНО: Диагностика настроек
    logDebugSafe("ThingSpeak: Channel ID: '%s' -> %lu, API Key: '%s' (длина: %zu)", 
                 channelBuf.data(), channelId, apiKeyBuf.data(), strlen(apiKeyBuf.data()));

    // Проверяем корректность ID и API ключа - если неверные, молча пропускаем
    if (channelId == 0 || strlen(apiKeyBuf.data()) < 16)
    {
        // Не логируем ошибку каждый раз, просто пропускаем отправку
        if (strlen(thingSpeakLastErrorBuffer.data()) == 0)  // логируем только первый раз
        {
            logWarnSafe("ThingSpeak: Неверные настройки - Channel ID: '%s', API Key длина: %zu", channelBuf.data(), strlen(apiKeyBuf.data()));
            strlcpy(thingSpeakLastErrorBuffer.data(), "Настройки не заданы", thingSpeakLastErrorBuffer.size());
        }
        return false;
    }

    // ✅ ДОБАВЛЕНО: Валидация данных перед отправкой
    if (!validateSensorData(sensorData)) {
        logWarn("ThingSpeak: Данные датчика невалидны, пропускаем отправку");
        return false;
    }

    // ✅ ДОБАВЛЕНО: Проверка стабильности WiFi соединения
    if (WiFi.status() != WL_CONNECTED) {
        logWarn("ThingSpeak: WiFi соединение нестабильно, пропускаем отправку");
        return false;
    }

    // ✅ Проверка DNS перед отправкой (разрешаем оба хоста)
    IPAddress thingSpeakIP;
    if (!WiFi.hostByName("api.thingspeak.com", thingSpeakIP) && !WiFi.hostByName("thingspeak.com", thingSpeakIP)) {
        logWarn("ThingSpeak: DNS ошибка - не удается разрешить api.thingspeak.com/thingspeak.com");
        strlcpy(thingSpeakLastErrorBuffer.data(), "DNS Error", thingSpeakLastErrorBuffer.size());
        return false;
    }

    // ✅ ДОБАВЛЕНО: Увеличенный таймаут для ThingSpeak (на отдельном клиенте)
    thingSpeakClient.setTimeout(30000);  // 30 секунд вместо стандартных 5

    // ✅ Диагностика данных перед отправкой + User-Agent через фейковое поле 8
    logDebugSafe("ThingSpeak: Данные для отправки - T:%.2f, H:%.2f, EC:%.2f, pH:%.2f, N:%d, P:%d, K:%d", 
                 sensorData.temperature, sensorData.humidity, sensorData.ec, sensorData.ph,
                 (int)sensorData.nitrogen, (int)sensorData.phosphorus, (int)sensorData.potassium);

    // Обнуляем поля перед заполнением, чтобы избежать унаследованных значений
    for (unsigned f = 1; f <= 8; ++f) { ThingSpeak.setField(f, ""); }

    // Формируем данные для отправки (влажность как ASM, остальные компенсированные)
    ThingSpeak.setField(1, sensorData.temperature);
    {
        SensorCompensationService compensationService;
        const SoilType soil = SensorProcessing::getSoilType(config.soilProfile);
        const float vwcFraction = sensorData.humidity / 100.0F;
        const float asmPercent = compensationService.vwcToAsm(vwcFraction, soil);
        ThingSpeak.setField(2, asmPercent); // ASM
    }
    ThingSpeak.setField(3, sensorData.ec);
    ThingSpeak.setField(4, sensorData.ph);
    ThingSpeak.setField(5, (long)sensorData.nitrogen);
    ThingSpeak.setField(6, (long)sensorData.phosphorus);
    ThingSpeak.setField(7, (long)sensorData.potassium);

    // Уникальный идентификатор для избежания HTTP 304 (исправлено: используем строку вместо float)
    char buf[12];
    snprintf(buf, sizeof(buf), "%lu", millis());
    ThingSpeak.setField(8, buf);

    // ✅ ДОБАВЛЕНО: Улучшенная обработка ошибок с детальной диагностикой
    // Важно: ThingSpeak.update ожидает до 8 полей; используем writeFields с Channel ID и API ключом
    const int res = ThingSpeak.writeFields(channelId, apiKeyBuf.data());
    
         if (res == 200)  // ✅ HTTP 200 - настоящий успех
     {
         logSuccess("ThingSpeak: данные отправлены (HTTP 200)");
         lastTsPublish = millis();
         snprintf(thingSpeakLastPublishBuffer.data(), thingSpeakLastPublishBuffer.size(), "%lu", lastTsPublish);
         thingSpeakLastErrorBuffer[0] = '\0';  // Очистка ошибки
         consecutiveFailCount = 0;             // обнуляем при успехе
         lastFailTime = 0;                     // Сбрасываем время ошибки
         nextThingSpeakTry = 0;                // Сбрасываем время следующей попытки
         return true;
     }
    else if (res == TS_ERR_TIMEOUT || res == TS_ERR_BAD_RESPONSE || res == TS_ERR_CONNECT_FAILED)
    {
        // Сетевые таймауты/ошибки парсинга/соединения — поддержим короткий повтор без блокировки
        consecutiveFailCount++;
        lastFailTime = millis();
        logWarnSafe("ThingSpeak: временная сетевая ошибка (%d), будет повтор", res);
        
        // Исправляем логику ретраев: избегаем переполнения и конфликтов
        unsigned long retryDelay = 10000; // 10 секунд
        if (config.thingSpeakInterval > retryDelay) {
            // Если основной интервал больше ретрая, используем его
            retryDelay = config.thingSpeakInterval;
        }
        
        // Безопасное вычисление времени следующей попытки (исправлено: используем отдельную переменную)
        unsigned long nextAttempt = millis() + retryDelay;
        if (nextAttempt < millis()) {
            // Защита от переполнения
            nextAttempt = millis() + 30000; // 30 секунд как fallback
        }
        
        nextThingSpeakTry = nextAttempt;  // Используем отдельную переменную вместо манипуляции с lastTsPublish
        return false;
    }
    else if (res == 304 || res == -304)  // ✅ HTTP 304 - данные не изменились, но это НЕ ошибка
    {
        logSuccess("ThingSpeak: данные отправлены (HTTP 304 - не изменились)");
        lastTsPublish = millis();
        snprintf(thingSpeakLastPublishBuffer.data(), thingSpeakLastPublishBuffer.size(), "%lu", lastTsPublish);
        nextThingSpeakTry = 0;  // сбрасываем отложенную попытку
        // НЕ сбрасываем счетчик ошибок и НЕ очищаем ошибку - это не настоящий успех
        return true;
    }
    else
    {
        // ✅ ДОБАВЛЕНО: Детальная диагностика ошибок
        char errorMsg[96];
                 switch (res) {
             case 200:
                 strlcpy(errorMsg, "HTTP 200 (успех)", sizeof(errorMsg));
                 break;
             case 304:
             case -304:
                 strlcpy(errorMsg, "HTTP 304 (данные не изменились)", sizeof(errorMsg));
                 break;
                         case 400:
                 strlcpy(errorMsg, "HTTP 400 (неверный запрос - проверьте API ключ/Channel ID)", sizeof(errorMsg));
                 break;
             case 429:
                 strlcpy(errorMsg, "HTTP 429 (слишком часто)", sizeof(errorMsg));
                 break;
            case -301:
                strlcpy(errorMsg, "Timeout -301 (увеличьте интервал)", sizeof(errorMsg));
                break;
            case -302:
                strlcpy(errorMsg, "DNS Error -302", sizeof(errorMsg));
                break;
            case -303:
                strlcpy(errorMsg, "Connection Failed -303", sizeof(errorMsg));
                break;
            case 0:
                strlcpy(errorMsg, "HTTP 0 (проверьте WiFi)", sizeof(errorMsg));
                break;
            default:
                snprintf(errorMsg, sizeof(errorMsg), "HTTP %d", res);
                break;
        }
        
        logWarnSafe("ThingSpeak: ошибка отправки: %s", errorMsg);
        
        // 🔁 Fallback: прямой HTTP POST без channelId при HTTP 400 (проверяем связку api_key+fields)
        if (res == 400) {
            HTTPClient http;
            String url = String("http://api.thingspeak.com/update");
            if (http.begin(url)) {
                http.addHeader("Content-Type", "application/x-www-form-urlencoded");
                String body;
                body.reserve(200);
                body += "api_key="; body += apiKeyBuf.data();
                body += "&field1="; body += String(sensorData.temperature, 2);
                {
                    SensorCompensationService compensationService;
                    const SoilType soil = SensorProcessing::getSoilType(config.soilProfile);
                    const float vwcFraction = sensorData.humidity / 100.0F;
                    const float asmPercent = compensationService.vwcToAsm(vwcFraction, soil);
                    body += "&field2="; body += String(asmPercent, 2); // ASM
                }
                body += "&field3="; body += String(sensorData.ec, 2);
                body += "&field4="; body += String(sensorData.ph, 2);
                body += "&field5="; body += String((int)sensorData.nitrogen);
                body += "&field6="; body += String((int)sensorData.phosphorus);
                body += "&field7="; body += String((int)sensorData.potassium);
                body += "&field8="; body += String(millis());

                int httpCode = http.POST(body);
                String resp = http.getString();
                http.end();

                if (httpCode == 200 && resp.length() > 0 && resp != "0") {
                    logSuccessSafe("ThingSpeak fallback: данные отправлены (entry_id=%s)", resp.c_str());
                    lastTsPublish = millis();
                    snprintf(thingSpeakLastPublishBuffer.data(), thingSpeakLastPublishBuffer.size(), "%lu", lastTsPublish);
                    thingSpeakLastErrorBuffer[0] = '\0';
                    consecutiveFailCount = 0;
                    lastFailTime = 0;
                    nextThingSpeakTry = 0;
                    return true;
                } else {
                    logWarnSafe("ThingSpeak fallback: HTTP %d, resp='%s'", httpCode, resp.c_str());
                }
            }
        }
        strlcpy(thingSpeakLastErrorBuffer.data(), errorMsg, thingSpeakLastErrorBuffer.size());
        
        consecutiveFailCount++;
        lastFailTime = millis();  // ✅ ДОБАВЛЕНО: Записываем время ошибки

        // ✅ ДОБАВЛЕНО: Улучшенная логика повторных попыток
        if (res == -301 || res == -302 || res == -303) {
            // Сетевые ошибки - повторяем с экспоненциальной задержкой
            if (consecutiveFailCount < 5) {
                unsigned long retryDelay = 10000 * (1 << (consecutiveFailCount - 1));  // 10, 20, 40, 80 сек
                
                // Ограничиваем максимальную задержку
                if (retryDelay > 300000) { // 5 минут максимум
                    retryDelay = 300000;
                }
                
                logDebugSafe("ThingSpeak: Повторная попытка через %lu секунд", retryDelay / 1000);
                
                // Безопасное вычисление времени следующей попытки (исправлено: используем отдельную переменную)
                unsigned long nextAttempt = millis() + retryDelay;
                if (nextAttempt < millis()) {
                    // Защита от переполнения
                    nextAttempt = millis() + 30000; // 30 секунд как fallback
                }
                
                nextThingSpeakTry = nextAttempt;  // Используем отдельную переменную вместо манипуляции с lastTsPublish
                return false;
            }
        }

                 // Если слишком много ошибок подряд, временно отключаем на 30 минут
         if (consecutiveFailCount >= 5)
         {
             logWarnSafe("ThingSpeak: Отключён на 30 минут (много ошибок: %d)", consecutiveFailCount);
             lastTsPublish = millis();  // устанавливаем время последней попытки
             
             // ✅ ИСПРАВЛЕНО: Сохраняем И блокировку, И реальную ошибку
             char combinedError[128];
              snprintf(combinedError, sizeof(combinedError), "Блокировка 30 мин (%d ошибок) | Последняя: %s", 
                     consecutiveFailCount, errorMsg);
              strlcpy(thingSpeakLastErrorBuffer.data(), combinedError, thingSpeakLastErrorBuffer.size());
         }
        
        return false;
    }
}
