/**
 * @file	os.h
 * @brief	Declaration of FreeRTOS wrappers and helpers.
 * @author	Kacper Kowalski - kacper.kowalski@lerta.energy
 */
#ifndef OS_H
#define OS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include <stdbool.h>

//! RAII like mutex locking. One does not need to unlock the mutex as this macro handles that on its own.
#define with_mutex(_mux)                                                                                               \
    for (BaseType_t _b = xSemaphoreTake(_mux, portMAX_DELAY); _b == pdTRUE; _b = (xSemaphoreGive(_mux), pdFALSE))

//! Notify a task from interrupt and perform context switch if higher priority task has been woken.
void notify_from_isr(TaskHandle_t hdl);

/**
 *	\brief	Ask the predicate to return true value.
 *
 *	Tests the function for 'true' return value for 'timeout' time and calls it with 'delay_between_tests' delay.
 *	When the predicate returns true then thefunction exits immediately with true result. If after the timeout
 *	true value hasn't been returned by the function then this function returns false.
 */
bool ask_fun_return_true(bool (*predicate)(void), TickType_t timeout, TickType_t delay_between_tests);

//! Hangs the task by making it waiting indefinitely.
void os_wait_indefinitely();

#ifdef __cplusplus
}
#endif

#endif /* OS_H */
