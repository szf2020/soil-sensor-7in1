/**
 * @file logger.h
 * @brief Система логгирования с красивым форматированием
 */

#ifndef LOGGER_H
#define LOGGER_H

#ifdef TEST_BUILD
#include "../test/stubs/esp32_stubs.h"
#else
#include <Arduino.h>
#endif
#include <array>
#include <utility>
#include <type_traits>
#include <cstdint>
#include <cstdio>

// Уровни логгирования
enum LogLevel : std::uint8_t
{
    LOG_ERROR = 0,
    LOG_WARN = 1,
    LOG_INFO = 2,
    LOG_DEBUG = 3
};

// Цвета для заголовков логгирования
enum class LogColor : std::uint8_t
{
    RED = 0,
    GREEN = 1,
    YELLOW = 2,
    BLUE = 3,
    MAGENTA = 4,
    CYAN = 5,
    WHITE = 6,
    CYAN_DEFAULT = 7  // Уникальное значение
};

// Текущий уровень логгирования (можно менять)
extern LogLevel currentLogLevel;

// Символы для разных типов сообщений
#define LOG_SYMBOL_ERROR "❌"
#define LOG_SYMBOL_WARN "⚠️ "
#define LOG_SYMBOL_INFO "ℹ️ "
#define LOG_SYMBOL_DEBUG "🔧"
#define LOG_SYMBOL_SUCCESS "✅"
#define LOG_SYMBOL_SENSOR "📡"
#define LOG_SYMBOL_WIFI "📶"
#define LOG_SYMBOL_MQTT "🌐"
#define LOG_SYMBOL_HTTP "🌍"

// Цвета ANSI для терминала
#define COLOR_RED "\033[31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_BLUE "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN "\033[36m"
#define COLOR_WHITE "\033[37m"
#define COLOR_RESET "\033[0m"

// Безопасные helper функции для форматирования
String formatLogMessage(const String& message);

// Основные функции логгирования (String версии)
void logError(const String& message);
void logWarn(const String& message);
void logInfo(const String& message);
void logDebug(const String& message);
void logSuccess(const String& message);
void logSensor(const String& message);
void logWiFi(const String& message);
void logMQTT(const String& message);
void logHTTP(const String& message);
void logSystem(const String& message);
void logData(const String& message);

// Современная C++ альтернатива с template parameter pack
template <typename... Args>
String formatLogMessageSafe(const char* format, Args&&... args)
{
    // Define once to avoid duplication
    auto toStringAny = [](const auto& value) -> String {
        using T = typename std::decay<decltype(value)>::type;
        if constexpr (std::is_same<T, String>::value)
        {
            return value;
        }
        else if constexpr (std::is_same<T, const char*>::value || std::is_same<T, char*>::value)
        {
            return String(value ? value : "");
        }
        else if constexpr (std::is_same<T, bool>::value)
        {
            return value ? String("true") : String("false");
        }
        else if constexpr (std::is_arithmetic<T>::value)
        {
            return String(value);
        }
        else if constexpr (std::is_pointer<T>::value)
        {
            // Render pointer address as hex
            uintptr_t addr = reinterpret_cast<uintptr_t>(value);
            char buf[3 + (sizeof(uintptr_t) * 2)];
            // 0x + hex digits, ensure null-termination
            int n = snprintf(buf, sizeof(buf), "0x%llx", static_cast<unsigned long long>(addr));
            (void)n;
            return String(buf);
        }
        else
        {
            return String(value);
        }
    };

    // If format is null, just join arguments
    if (format == nullptr)
    {
        String out;

        auto appendArg = [&](const auto& v) {
            if (out.length() > 0) out += " ";
            out += toStringAny(v);
        };
        (appendArg(std::forward<Args>(args)), ...);
        return out;
    }

    // Count real printf-style placeholders, ignoring escaped percent signs ("%%").
    auto countPrintfSpecifiers = [](const char* fmt) -> int {
        int count = 0;
        for (const char* p = fmt; *p; ++p)
        {
            if (*p != '%')
            {
                continue;
            }

            // Handle escaped percent: "%%" → skip both
            if (*(p + 1) == '%')
            {
                ++p; // skip the second '%'
                continue;
            }

            // We are at start of a conversion specifier. Skip flags/width/precision/length.
            ++p;
            // Flags: -+ #0 '
            while (*p == '-' || *p == '+' || *p == ' ' || *p == '#' || *p == '0' || *p == '\'')
            {
                ++p;
            }
            // Width: number or '*'
            if (*p == '*')
            {
                ++p;
            }
            else
            {
                while (*p >= '0' && *p <= '9') ++p;
            }
            // Precision: .number or .*
            if (*p == '.')
            {
                ++p;
                if (*p == '*')
                {
                    ++p;
                }
                else
                {
                    while (*p >= '0' && *p <= '9') ++p;
                }
            }
            // Length modifiers (subset sufficient for our targets)
            if ((*p == 'h' && *(p + 1) == 'h') || (*p == 'l' && *(p + 1) == 'l'))
            {
                p += 2;
            }
            else if (*p == 'h' || *p == 'l' || *p == 'j' || *p == 'z' || *p == 't' || *p == 'L')
            {
                ++p;
            }
            // Conversion specifier
            if (*p == '\0')
            {
                break;
            }
            const char c = *p;
            switch (c)
            {
                case 'd': case 'i': case 'u': case 'o': case 'x': case 'X':
                case 'f': case 'F': case 'e': case 'E': case 'g': case 'G': case 'a': case 'A':
                case 'c': case 's': case 'p': case 'n':
                    ++count;
                    break;
                default:
                    // Unknown specifier; treat as non-specifier (do not increment)
                    break;
            }
        }
        return count;
    };

    const int placeholderCount = countPrintfSpecifiers(format);

    // If there are no placeholders, or argument count mismatches, prefer safe concatenation
    if (placeholderCount == 0 || placeholderCount != static_cast<int>(sizeof...(args)))
    {
        // Special sentinel: "\1" often used to mean "no base text, just args"
        // In C/C++ the literal "\1" is a single char with value 1
        const bool isSentinel = (format[0] == '\x01' && format[1] == '\0');
        String out = isSentinel ? String("") : String(format);

        if constexpr (sizeof...(args) > 0)
        {
            if (out.length() > 0) out += " ";
            auto appendArg = [&](const auto& v) {
                if (out.length() > 0 && out[out.length() - 1] != ' ') out += " ";
                out += toStringAny(v);
            };
            (appendArg(std::forward<Args>(args)), ...);
        }
        return out;
    }

    std::array<char, 512> buffer{};
    int result = snprintf(buffer.data(), buffer.size(), format, std::forward<Args>(args)...);
    if (result < 0)
    {
        return String("LOG FORMAT ERROR");
    }
    if (result < static_cast<int>(buffer.size()))
    {
        return String(buffer.data());
    }
    buffer[buffer.size() - 1] = '\0';
    return String(buffer.data());
}

template <typename... Args>
void logErrorSafe(const char* format, Args&&... args)
{
    logError(formatLogMessageSafe(format, std::forward<Args>(args)...));
}

template <typename... Args>
void logWarnSafe(const char* format, Args&&... args)
{
    logWarn(formatLogMessageSafe(format, std::forward<Args>(args)...));
}

template <typename... Args>
void logInfoSafe(const char* format, Args&&... args)
{
    logInfo(formatLogMessageSafe(format, std::forward<Args>(args)...));
}

template <typename... Args>
void logDebugSafe(const char* format, Args&&... args)
{
    logDebug(formatLogMessageSafe(format, std::forward<Args>(args)...));
}

template <typename... Args>
void logSuccessSafe(const char* format, Args&&... args)
{
    logSuccess(formatLogMessageSafe(format, std::forward<Args>(args)...));
}

template <typename... Args>
void logSensorSafe(const char* format, Args&&... args)
{
    logSensor(formatLogMessageSafe(format, std::forward<Args>(args)...));
}

template <typename... Args>
void logWiFiSafe(const char* format, Args&&... args)
{
    logWiFi(formatLogMessageSafe(format, std::forward<Args>(args)...));
}

template <typename... Args>
void logMQTTSafe(const char* format, Args&&... args)
{
    logMQTT(formatLogMessageSafe(format, std::forward<Args>(args)...));
}

template <typename... Args>
void logHTTPSafe(const char* format, Args&&... args)
{
    logHTTP(formatLogMessageSafe(format, std::forward<Args>(args)...));
}

template <typename... Args>
void logSystemSafe(const char* format, Args&&... args)
{
    logSystem(formatLogMessageSafe(format, std::forward<Args>(args)...));
}

template <typename... Args>
void logDataSafe(const char* format, Args&&... args)
{
    logData(formatLogMessageSafe(format, std::forward<Args>(args)...));
}

// Специальные функции
void logSeparator();
void logNewline();
void logMemoryUsage();
void logSystemInfo();
void logUptime();
void logWiFiStatus();

// Функции для работы с цветами
void setLogColor(LogColor color);
void resetLogColor();
const char* getColorCode(LogColor color);

// Утилиты
void printHeader(const String& title, LogColor color = LogColor::CYAN_DEFAULT);
void printSubHeader(const String& title, LogColor color = LogColor::BLUE);
void printTimeStamp();

// Алиасы для обратной совместимости
#define logPrintHeader printHeader
#define logPrintSeparator(symbol, length) logSeparator()
#define logPrintBanner(text) printHeader(String(text), LogColor::CYAN)

#endif  // LOGGER_H
