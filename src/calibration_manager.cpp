#include "calibration_manager.h"
#include "logger.h"
#include "sensor_types.h"

namespace CalibrationManager
{
namespace
{
bool _initialized = false;
}

// Упрощенная версия без файлов - просто возвращаем исходное значение
const char* profileToFilename(SoilProfile /*profile*/)  // NOLINT(misc-use-internal-linkage)
{
    return "";  // Убираем файл калибровки
}

bool init()  // NOLINT(misc-use-internal-linkage)
{
    if (_initialized)
    {
        return true;
    }

    _initialized = true;
    logSuccess(String("Калибровка инициализирована (упрощенный режим)"));
    return true;
}

bool saveCsv(SoilProfile profile, Stream& fileStream)  // NOLINT(misc-use-internal-linkage)
{
    // Упрощенная версия - ничего не сохраняем
    logInfoSafe("Калибровка: сохранение отключено для профиля %d", static_cast<int>(profile));
    return true;
}

bool loadTable(SoilProfile profile, CalibrationEntry* outBuffer, size_t maxEntries,
               size_t& outCount)  // NOLINT(misc-use-internal-linkage)
{
    // Упрощенная версия - не загружаем файлы
    outCount = 0;
    logInfoSafe("Калибровка: загрузка отключена для профиля %d", static_cast<int>(profile));
    return false;
}

bool hasTable(SoilProfile profile)  // NOLINT(misc-use-internal-linkage)
{
    // Упрощенная версия - всегда false, чтобы не пытаться загружать файлы
    return false;
}

bool deleteTable(SoilProfile profile)  // NOLINT(misc-use-internal-linkage)
{
    // Упрощенная версия - ничего не удаляем
    logInfoSafe("Калибровка: удаление отключено для профиля %d", static_cast<int>(profile));
    return true;
}

float applyCalibration(float rawValue,
                       SoilProfile profile)  // NOLINT(misc-use-internal-linkage, bugprone-easily-swappable-parameters)
{
    // Упрощенная версия - просто возвращаем исходное значение
    // Убираем всю логику с файлами
    return rawValue;
}
}  // namespace CalibrationManager
