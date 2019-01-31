/**
 * @file	unit_tests_main.cpp
 * @brief	Definition of main() for unit tests.
 * @author	Kacper Kowalski - kacper.s.kowalski@gmail.com
 */
#include "unity.h"

extern void test_at_cmd_handler();

int main()
{
    UNITY_BEGIN();

    test_at_cmd_handler();

    return UNITY_END();
}
