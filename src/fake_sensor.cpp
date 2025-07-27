/**
 * @file fake_sensor.cpp
 * @brief Эмуляция работы датчика для тестирования
 * @details Генерация тестовых данных для отладки и демонстрации работы системы без реального оборудования.
 */
#include "fake_sensor.h"
#include <Arduino.h>
#include "debug.h"  // ✅ Добавляем систему условной компиляции
#include "jxct_config_vars.h"
#include "logger.h"  // ✅ Добавляем для logDebugSafe
#include "modbus_sensor.h"
#include "business/sensor_compensation_service.h"
#include "business/sensor_calibration_service.h"
#include "sensor_types.h"  // Для SoilProfile
#include "sensor_processing.h"    // Общая логика обработки
// Глобальные экземпляры бизнес-сервисов объявлены в business_instances.cpp
extern SensorCalibrationService gCalibrationService;
extern SensorCompensationService gCompensationService;

namespace
{
void fakeSensorTask(void* parameters)
{
    (void)parameters;                                        // Suppress unused parameter warning
    const TickType_t taskDelay = 1000 / portTICK_PERIOD_MS;  // 1 секунда
    const uint32_t dataGenerationInterval = 10;              // Генерация данных каждые 10 итераций
    uint32_t iterationCounter = 0;

    for (;;)
    {
        // Генерируем данные только каждые 10 секунд
        if (iterationCounter >= dataGenerationInterval)
        {
            sensorData.temperature = 22.0F + static_cast<float>(random(-50, 50)) / 10.0F;  // 17.0..27.0
            sensorData.humidity = 50.0F + static_cast<float>(random(-200, 200)) / 10.0F;   // 30..70
            sensorData.ec = 1000 + static_cast<float>(random(-200, 200));                  // 800..1200
            sensorData.ph = 6.5F + static_cast<float>(random(-20, 20)) / 10.0F;            // 4.5..8.5

            // NPK в мг/кг (как в реальном датчике JXCT)
            // ИСПРАВЛЕНО: Генерируем данные в соответствии с агрономическими нормами
            // Рекомендуемые диапазоны для основных культур:
            // N: 100-300 мг/кг, P: 30-150 мг/кг, K: 100-400 мг/кг
            // Используем более узкие диапазоны для лучшего соответствия рекомендациям
            const float nitrogen_mgkg = 175.0F + static_cast<float>(random(-25, 25));   // 150..200 мг/кг
            const float phosphorus_mgkg = 75.0F + static_cast<float>(random(-15, 15));  // 60..90 мг/кг
            const float potassium_mgkg = 250.0F + static_cast<float>(random(-50, 50));  // 200..300 мг/кг

            // Данные уже в мг/кг, конверсия не нужна
            NPKReferences npk{nitrogen_mgkg, phosphorus_mgkg, potassium_mgkg};

            sensorData.valid = true;
            sensorData.last_update = millis();  // ✅ Обновляем timestamp

            // RAW значения до компенсации
            sensorData.raw_temperature = sensorData.temperature;
            sensorData.raw_humidity = sensorData.humidity;
            sensorData.raw_ec = sensorData.ec;
            sensorData.raw_ph = sensorData.ph;
            // RAW до компенсации, но уже в правильных единицах (мг/кг)
            sensorData.raw_nitrogen = npk.nitrogen;
            sensorData.raw_phosphorus = npk.phosphorus;
            sensorData.raw_potassium = npk.potassium;

            // КРИТИЧЕСКОЕ ИСПРАВЛЕНИЕ: всегда сохраняем NPK данные в sensorData
            sensorData.nitrogen = npk.nitrogen;
            sensorData.phosphorus = npk.phosphorus;
            sensorData.potassium = npk.potassium;

            // Применяем единую логику обработки данных датчика
            SensorProcessing::processSensorData(sensorData, config);



            DEBUG_PRINTLN("[fakeSensorTask] Сгенерированы тестовые данные датчика");
            iterationCounter = 0;  // Сброс счетчика
        }

        iterationCounter++;

        // ✅ Более частые, но короткие задержки для отзывчивости
        vTaskDelay(taskDelay);
    }
}

// forward declaration; реализация глобальная ниже
void startFakeSensorTask();
}  // anonymous namespace

// Определение-обёртка с внешним связыванием
void startFakeSensorTask()
{
    xTaskCreate(fakeSensorTask, "FakeSensor", 4096, nullptr, 1, nullptr);
}
