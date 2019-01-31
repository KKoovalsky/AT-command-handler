/**
 * @file	string_buf_tx.hpp
 * @brief	Defines a buffer with strings which can be pushed by string and popped by byte.
 * @author	Kacper Kowalski - kacper.s.kowalski@gmail.com
 */

#ifndef STRING_BUF_TX_HPP
#define STRING_BUF_TX_HPP

#include <list>
#include <memory>
#include <string>

/**
 * \brief       Easily push strings to buffer and pop single bytes from it (e.g. when transmitting some data and
 *              using interrupts).
 *
 * This structure must be cleaned up manually. It's implemented in that way because earlier it cleaned up itself when
 * popping bytes. The problem was that the pop_byte() function was called from an FreeRTOS ISR. This caused to call
 * vPortFree sometimes in the ISR what was breaking the application.
 *
 * This is not thread safe at all. The best way to keep this object fit is to clean it up in the same place as the
 * push_string() method is used.
 */
class string_buf_tx
{
  public:
    void push_string(std::unique_ptr<std::string> &&s);
    char pop_byte();
    bool is_empty();

    //! When using FreeRTOS call this from a task context. This mustn't be called from the ISR.
    void clean();

  private:
    //! Aggregates the strings.
    std::list<std::unique_ptr<std::string>> m_strings;

    //! Iterates over the strings.
    typename std::list<std::unique_ptr<std::string>>::iterator m_strings_it = m_strings.begin();

    //! Used to iterate over the strings when popping single bytes.
    const char *m_byte_it = nullptr;
};

#endif /* STRING_BUF_TX_HPP */
