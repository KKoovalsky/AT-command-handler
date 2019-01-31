/**
 * @file	string_buf_tx.cpp
 * @brief	Implements a buffer with strings which can be pushed by string and popped by byte.
 * @author	Kacper Kowalski - kacper.s.kowalski@gmail.com
 */
#include "string_buf_tx.hpp"

// --------------------------------------------------------------------------------------------------------------------
// DEFINITION OF PUBLIC MEMBER FUNCTIONS
// --------------------------------------------------------------------------------------------------------------------
void string_buf_tx::push_string(std::unique_ptr<std::string> &&s)
{
    auto was_empty_list_of_strings = m_strings.empty();
    auto was_empty = is_empty();

    m_strings.emplace_back(std::move(s));

    if (was_empty_list_of_strings)
        m_strings_it = m_strings.begin();
    // When all the strings were popped but no cleaning up occured.
    else if(was_empty)
    {
        m_strings_it = m_strings.end();
        m_strings_it--;
    }
}

char string_buf_tx::pop_byte()
{
    if (is_empty())
        return '\0';

    const auto &current_str = *m_strings_it;

    if (m_byte_it == nullptr)
        m_byte_it = current_str->c_str();

    char result = *m_byte_it++;

    // When the whole string has been popped then switch to another one.
    if (m_byte_it == current_str->c_str() + current_str->length())
    {
        m_byte_it = nullptr;
        m_strings_it++;
    }

    return result;
}

bool string_buf_tx::is_empty()
{
    return m_strings_it == m_strings.end();
}

void string_buf_tx::clean()
{
    m_strings.erase(m_strings.begin(), m_strings_it);
}
