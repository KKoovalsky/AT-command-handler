/**
 * @file	os_lockguard.cpp
 * @brief	FreeRTOS lockguard implementation.
 * @author	Kacper Kowalski - kacper.s.kowalski@gmail.com
 */
#include "os_lockguard.hpp"

os_lockguard::os_lockguard(SemaphoreHandle_t m) noexcept : mux(m) { xSemaphoreTake(mux, portMAX_DELAY); }

os_lockguard::~os_lockguard() { xSemaphoreGive(mux); }
