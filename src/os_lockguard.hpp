/**
 * @file	os_lockguard.hpp
 * @brief	Declaration of FreeRTOS lockguard.
 * @author	Kacper Kowalski - kacper.s.kowalski@gmail.com
 */
#ifndef OS_LOCKGUARD_HPP
#define OS_LOCKGUARD_HPP

#include "FreeRTOS.h"
#include "semphr.h"

//! Implements RAII for semaphore/mutex locking.
class os_lockguard
{
  public:
	//! The semaphore/mutex must be created before this constructor is used.
	os_lockguard(SemaphoreHandle_t m) noexcept;
	os_lockguard(const os_lockguard &) = delete;
	os_lockguard &operator=(const os_lockguard &) = delete;
	os_lockguard(os_lockguard &&) = delete;
	os_lockguard &operator=(os_lockguard &&) = delete;
	~os_lockguard();

  private:
	SemaphoreHandle_t mux;
};

#endif /* OS_LOCKGUARD_HPP */
