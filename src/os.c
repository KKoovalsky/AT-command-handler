/**
 * @file	os.c
 * @brief	FreeRTOS wrappers and helpers.
 * @author	Kacper Kowalski - kacper.kowalski@lerta.energy
 */
#include "FreeRTOS.h"
#include "task.h"
#include "os.h"
#include <stdbool.h>

void notify_from_isr(TaskHandle_t hdl)
{
	BaseType_t higher_prior_task_woken = pdFALSE;

	vTaskNotifyGiveFromISR(hdl, &higher_prior_task_woken);

	// When higher priority task has been woken because of waking it up after sending the notification then
	// perform a context switch.
	portEND_SWITCHING_ISR(higher_prior_task_woken);
}

bool ask_fun_return_true(bool (*predicate)(void), TickType_t timeout, TickType_t delay_between_tests)
{
	TickType_t iters = timeout / delay_between_tests;
	TickType_t i = 0;
	for (; i < iters; ++i)
	{
		if (predicate())
			break;
		vTaskDelay(pdMS_TO_TICKS(delay_between_tests));
	}

	if (i == iters)
		return false;
	return true;
}

void os_wait_indefinitely()
{
	vTaskDelay(portMAX_DELAY);
}

void delay_1ms()
{
	vTaskDelay(pdMS_TO_TICKS(1));
}
