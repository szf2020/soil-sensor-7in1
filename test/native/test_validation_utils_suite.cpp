#include <unity.h>
#include <iostream>

#include "validation_utils.h"
#include "sensor_types.h"

void test_temperature_validation()
{
    TEST_CASE("Валидация температуры: граничные значения и NaN");

    auto ok = validateTemperature(25.0F);
    TEST_ASSERT_TRUE(ok.isValid);

    auto low = validateTemperature(-60.0F);
    TEST_ASSERT_FALSE(low.isValid);
    TEST_ASSERT_TRUE(low.message.length() > 0);

    auto high = validateTemperature(120.0F);
    TEST_ASSERT_FALSE(high.isValid);
}

void test_ph_validation()
{
    TEST_CASE("Валидация pH: диапазоны");
    auto ok = validatePH(7.0F);
    TEST_ASSERT_TRUE(ok.isValid);

    auto low = validatePH(-1.0F);
    TEST_ASSERT_FALSE(low.isValid);

    auto high = validatePH(15.0F);
    TEST_ASSERT_FALSE(high.isValid);
}

void test_full_sensor_validation_aggregates_errors()
{
    TEST_CASE("Агрегация ошибок validateFullSensorData");
    SensorData data;
    data.temperature = 200.0F; // ошибка
    data.humidity = -5.0F;     // ошибка
    data.ph = 20.0F;           // ошибка
    data.ec = -1.0F;           // ошибка
    data.nitrogen = -10.0F;    // ошибка
    data.phosphorus = -5.0F;   // ошибка
    data.potassium = -2.0F;    // ошибка

    auto res = validateFullSensorData(data);
    TEST_ASSERT_FALSE(res.isValid);
    TEST_ASSERT_TRUE(res.errors.size() >= 5);
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_temperature_validation);
    RUN_TEST(test_ph_validation);
    RUN_TEST(test_full_sensor_validation_aggregates_errors);
    return UNITY_END();
}


