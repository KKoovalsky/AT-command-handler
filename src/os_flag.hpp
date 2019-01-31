/**
 * @file	os_flag.hpp
 * @brief	Implements one-to-many RTOS flag.
 * @author	Kacper Kowalski - kacper.s.kowalski@gmail.com
 */
#ifndef OS_FLAG_HPP
#define OS_FLAG_HPP

#include "FreeRTOS.h"
#include "event_groups.h"

/**
 * \brief Implements single-setter-multiple-awaiters RTOS flag.
 *
 * This flag will put a task to the blocked state when it is being awaited. It is thread-safe. Only one task is
 * allowed to call set() method.
 * The flag is reset by default.
 */
class os_flag
{
  public:
	os_flag();
	~os_flag();

	void wait_set();
	void set();
	void reset();
	bool is_set();

	os_flag(const os_flag &) = delete;
	os_flag(os_flag &&) = delete;
	os_flag &operator=(const os_flag &) = delete;
	os_flag &operator=(os_flag &&) = delete;

  private:
	EventGroupHandle_t event_group;
};

#endif /* OS_FLAG_HPP */
