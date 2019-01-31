/**
 * @file	unit_tests_rtos_main.cpp
 * @brief	Definition of main() for unit tests run under FreeRTOS.
 * @author	Kacper Kowalski - kacper.s.kowalski@gmail.com
 */
#include "FreeRTOS.h"
#include "task.h"
#include "unity.h"
#include <iostream>

extern void test_at();

//! Here all the tests are run.
static void testing_task(void *params);

int main()
{
	UNITY_BEGIN();

	xTaskCreate(testing_task, "rtos_test", 1024, NULL, 1, NULL);

	vTaskStartScheduler();

	return UNITY_END();
}

static void testing_task(void *params)
{
	(void)params;

	test_at();

	vTaskEndScheduler();
}
