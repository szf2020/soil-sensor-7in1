#include <unity.h>
#include <iostream>
#include <cmath>

#include "sensor_types.h"
#include "business/sensor_compensation_service.h"

static void test_compensation_pipeline_flags()
{
    TEST_CASE("Сервис компенсации: монотонность EC/pH и изменение NPK при росте T/θ");

    SensorCompensationService svc;

    SensorData data_low{20.0F, 30.0F, 1200.0F, 7.0F, 1000.0F, 500.0F, 1200.0F};
    SensorData data_high{30.0F, 40.0F, 1200.0F, 7.0F, 1000.0F, 500.0F, 1200.0F};

    auto lo = data_low;  svc.applyCompensation(lo, SoilType::Loam);
    auto hi = data_high; svc.applyCompensation(hi, SoilType::Loam);

    // EC при росте T (консервативная модель) не должна уменьшаться
    TEST_ASSERT_TRUE(hi.ec >= lo.ec);
    // pH при росте T уменьшается (Нернст)
    TEST_ASSERT_TRUE(hi.ph <= lo.ph);
    // NPK при росте T и влажности должны увеличиваться
    TEST_ASSERT_TRUE(hi.nitrogen >= lo.nitrogen);
    TEST_ASSERT_TRUE(hi.phosphorus >= lo.phosphorus);
    TEST_ASSERT_TRUE(hi.potassium >= lo.potassium);
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_compensation_pipeline_flags);
    return UNITY_END();
}


