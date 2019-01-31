/**
 * @file	string_buf_rx.hpp
 * @brief	Defines a class which is a buffer with strings, which can be pushed by byte and popped by strings.
 * @author	Kacper Kowalski - kacper.s.kowalski@gmail.com
 */

#ifndef STRING_BUF_RX_HPP
#define STRING_BUF_RX_HPP

#include "cyclic_buf.hpp"
#include <array>
#include <memory>
#include <string>

//! Calculates the length of span between two indexes in the cyclic buffer.
static inline unsigned calc_len_in_circular_buffer(unsigned beg_idx, unsigned end_idx, unsigned cyclic_buf_size);

/**
 * \brief Push a single byte to it and pop whole strings. Useful when receiving messages using interrupts.
 *
 * This implementation assumes that the commands are popped in a short time after pushing them. Otherwise the
 * behaviour is undefined.
 *
 * The typical usage is that: push single bytes to it (e.g. on an interrupt on byte received), pop a command when
 * a whole command has been received.
 *
 * \todo	Make the cyclic buffers resizeable.
 * \todo	Keep track of the commands on overflow, because when an overflow in one of the buffers occurs the data
 *	        between the buffers is inconsistent: the indexes to the end of the buffers may not properlypoint
 *              to the real ends of commands.
 * \todo	Implement pushing multiple elements.
 */
template <size_t ImmediateBufferSize> class string_buf_rx
{
  public:
    /**
     * Takes string which contains exceptional characters which should be treated as a whole strings even when a
     * string terminator hasn't arrived. E.g. when you want to treat single '>' as a full string then pass it
     * in the argument. It is allowed to treat various characters in this manner.
     */
    string_buf_rx(std::string exceptional_chars = "");

    /**
     * Push a byte and check whether the byte terminates the string.
     * By default the string terminators are: CR, LF and '\0'
     */
    bool push_byte_and_is_string_end(char c);

    //! Pop a whole string. When there is no string this returns an empty string.
    std::unique_ptr<std::string> pop_string();

    bool is_empty();

  private:
    //! The default size of the buffer where the indexes with commands' ends are held.
    static constexpr size_t ends_indexes_cb_def_size = 16;

    //! This is a helper object which holds the indexes of the commands' ends. @todo Allocator pvPortMalloc
    cyclic_buf<unsigned int, ends_indexes_cb_def_size> m_end_indexes_cb;

    //! The cyclic buffer where the commands are held.
    cyclic_buf<char, ImmediateBufferSize> m_cb;

    //! Contains the exceptional characters which should be treated as whole strings.
    const std::string m_exceptional_chars;

    //! Last head index in the immediate buffer.
    unsigned m_last_end_idx = 0;
};

template <size_t ImmediateBufferSize>
string_buf_rx<ImmediateBufferSize>::string_buf_rx(std::string exceptional_chars)
    : m_exceptional_chars(exceptional_chars)
{
}

template <size_t ImmediateBufferSize> bool string_buf_rx<ImmediateBufferSize>::push_byte_and_is_string_end(char c)
{
    // Treat the carriage return, line feed or null terminating character as the end of command.
    if (c == '\n' || c == '\r' || c == '\0')
    {
        // When received a command of length 0 then do nothing.
        if (m_last_end_idx == m_cb.head)
            return false;
        else
        {
            m_end_indexes_cb.push_elem(m_cb.head);
            m_last_end_idx = m_cb.head;
            return true;
        }
    }

    // After receiving the exceptional character:
    if (m_exceptional_chars.find(c) != std::string::npos)
    {
        // Exceptional characters work only when they are received alone.
        if (m_last_end_idx == m_cb.head)
        {
            m_cb.push_elem(c);
            m_end_indexes_cb.push_elem(m_cb.head);
            m_last_end_idx = m_cb.head;
            return true;
        }
    }

    // Push any other character to the buffer.
    m_cb.push_elem(c);

    return false;
}

template <size_t ImmediateBufferSize> std::unique_ptr<std::string> string_buf_rx<ImmediateBufferSize>::pop_string()
{
    // Firstly check whether there are lines in the buffer. If not then return immediately.
    if (is_empty())
        return std::make_unique<std::string>("");

    auto beg = m_cb.tail;
    auto end = m_end_indexes_cb.pop_elem();
    unsigned int len = calc_len_in_circular_buffer(beg, end, ImmediateBufferSize);

    // Get the string to be returned.
    std::string s(len, '\0');
    m_cb.pop_nelems(&s.front(), len);

    return std::make_unique<std::string>(std::move(s));
}

template <size_t ImmediateBufferSize> bool string_buf_rx<ImmediateBufferSize>::is_empty()
{
    return m_end_indexes_cb.is_empty();
}

static inline unsigned calc_len_in_circular_buffer(unsigned beg_idx, unsigned end_idx, unsigned cyclic_buf_size)
{
    unsigned int len;
    // Check whether there will be a swing.
    if (beg_idx > end_idx)
        len = cyclic_buf_size - beg_idx + end_idx;
    else
        len = end_idx - beg_idx;
    return len;
}
#endif /* STRING_BUF_RX_HPP */
