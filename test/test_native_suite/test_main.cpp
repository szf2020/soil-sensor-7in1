#include <unity.h>

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

static void test_basic_math()
{
    TEST_ASSERT_EQUAL_INT(4, 2 + 2);
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_basic_math);
    return UNITY_END();
}


