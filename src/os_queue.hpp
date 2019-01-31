/**
 * @file	os_queue.hpp
 * @brief	Implementation of a queue which works like FreeRTOS Queue and allows to use C++ classes.
 * @author	Kacper Kowalski - kacper.s.kowalski@gmail.com
 */
#ifndef OS_QUEUE_HPP
#define OS_QUEUE_HPP

#include "FreeRTOS.h"
#include "neither/neither.hpp"
#include "os_lockguard.hpp"
#include "semphr.h"
#include <cassert>
#include <stdexcept>
#include <vector>

template <typename T, size_t N> class os_queue
{
  public:
    explicit os_queue();

    ~os_queue();

    //! Returns true when the element has been sent correctly to the queue, false otherwise.
    template <typename... U> bool send(U &&... u);

    //! Overwrites the element in the queue. Is only enabled when the queuen size is equal to one.
    template <size_t dim = N, class = typename std::enable_if_t<dim == 1>, typename... U> void overwrite(U &&... u);

    //! Either returns an element from the queue or returns false when timeout occured while awaiting for an element.
    neither::Either<T, bool> receive(TickType_t timeout);

  private:
    //! This vector contains elements of the queue. It's used to handle non-trivial types.
    std::vector<T> m_queue;

    //! Mutex which guards acces to the queue.
    SemaphoreHandle_t m_mux;

    //! Counting semaphore used to count how many elements are in the queue.
    SemaphoreHandle_t m_queue_num_elems_sem;
};

template <typename T, size_t N> os_queue<T, N>::os_queue()
{
    // The size is reserved once and the queue's capacity must not be changed any more.
    m_queue.reserve(N);
    m_mux = xSemaphoreCreateMutex();
    m_queue_num_elems_sem = xSemaphoreCreateCounting(N, 0);
}

template <typename T, size_t N> os_queue<T, N>::~os_queue()
{
    vSemaphoreDelete(m_mux);
    vSemaphoreDelete(m_queue_num_elems_sem);
}

template <typename T, size_t N> template <typename... U> bool os_queue<T, N>::send(U &&... u)
{
    bool is_full = false;

    {
        os_lockguard guard(m_mux);
        if (m_queue.size() == N)
            is_full = true;
        else
            m_queue.emplace_back(std::forward<U>(u)...);
    }

    if (!is_full)
        assert(xSemaphoreGive(m_queue_num_elems_sem) == pdTRUE);

    return !is_full;
}

template <typename T, size_t N> template <size_t dim, class, typename... U> void os_queue<T, N>::overwrite(U &&... u)
{
    {
        os_lockguard guard(m_mux);
        if (m_queue.size() == 0)
        {
            m_queue.emplace_back(std::forward<U>(u)...);
        }
        else
        {
            T t(std::forward<U>(u)...);
            m_queue[0] = std::move(t);
        }
    }
    xSemaphoreGive(m_queue_num_elems_sem);
}

template <typename T, size_t N> neither::Either<T, bool> os_queue<T, N>::receive(TickType_t timeout)
{
    if (xSemaphoreTake(m_queue_num_elems_sem, timeout) == pdFALSE)
        return neither::right(false);
    else
    {
        os_lockguard guard(m_mux);
        auto res = std::move(m_queue[m_queue.size() - 1]);
        m_queue.pop_back();
        return neither::left(static_cast<T>(res));
    }
}

#endif /* OS_QUEUE_HPP */

