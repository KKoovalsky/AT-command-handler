/**
 * @file	at_cmd_gen.hpp
 * @brief	This header is used to generate AT commands' strings at compile time.
 * @author	Kacper Kowalski - kacper.s.kowalski@gmail.com
 */

#ifndef AT_CMD_GEN_HPP
#define AT_CMD_GEN_HPP

#include "at_cmd_config.hpp"
#include <array>
#include <string_view>

#define STRINGIFY(X...) #X
#define TO_STRING(X) STRINGIFY(X)

template <typename E> constexpr static auto to_u_type(E e) noexcept
{
    return static_cast<std::underlying_type_t<E>>(e);
}

//! Taken from cppreference.com as C++17's find_if() is not constexpr
template <class InputIt, class UnaryPredicate>
constexpr InputIt find_if(InputIt first, InputIt last, UnaryPredicate p) noexcept
{
    for (; first != last; ++first)
    {
        if (p(*first))
        {
            return first;
        }
    }
    return last;
}

template <typename T, typename U, std::size_t N> constexpr auto count(const T (&arr)[N], const U &val) noexcept
{
    auto cnt = 0;
    auto it(std::cbegin(arr));
    while (it != std::cend(arr))
    {
        if (*it == val)
            cnt++;
        it++;
    }
    return cnt;
}

constexpr bool is_alnum(char c) noexcept
{
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

template <std::size_t N> constexpr std::array<char, N> to_upper(const std::array<char, N> &in)
{
    std::array<char, N> out = {};
    for (unsigned i = 0; i < N; i++)
    {
        char c = in[i];
        if (c >= 'a' && c <= 'z')
            c = c - 'a' + 'A';
        out[i] = c;
    }
    return out;
}

template <std::size_t OutArrSize, std::size_t InArrSize>
constexpr std::array<std::string_view, OutArrSize> make_array_with_at_commands(const std::array<char, InArrSize> &in)
{
    std::array<std::string_view, OutArrSize> arr;

    // The first element is always empty for the simplest 'AT' command
    arr[0] = {""};

    auto it = find_if(std::begin(in), std::end(in), is_alnum);
    for (unsigned i = 1; i < OutArrSize; ++i)
    {
        const auto it_end(find_if(it, std::end(in), [](const auto &e) { return !is_alnum(e); }));
        const auto beg_offset(std::distance(std::begin(in), it));
        const unsigned str_size(std::distance(it, it_end));
        arr[i] = std::string_view{in.data() + beg_offset, str_size};
        it = find_if(it_end, std::end(in), is_alnum);
    }
    return arr;
}

#endif /* AT_CMD_GEN_HPP */
