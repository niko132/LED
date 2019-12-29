#include <unity.h>
#include <stdio.h>
#include <iostream>

#include "DeviceManager.h"
#include "DeviceManager.cpp"

void test_abc(void)
{
    DeviceManager a(20);
    a.begin();

    a.debug();

    std::cout << "Test" << std::endl;
    TEST_ASSERT_EQUAL(4, 5);
}

int main()
{
    UNITY_BEGIN();

    RUN_TEST(test_abc);

    UNITY_END();

    return 0;
}