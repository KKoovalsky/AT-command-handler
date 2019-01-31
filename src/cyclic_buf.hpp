/**
 * @file	cyclic_buf.hpp
 * @brief	Cyclic buffer structure definition and declaration of operations which can be used on the buffer.
 * @author	Kacper Kowalski - kacper.s.kowalski@gmail.com
 */

#ifndef CYCLIC_BUF_HPP
#define CYCLIC_BUF_HPP

#include <cstddef>
#include <algorithm>

//! Checks whether the number is a power of two. Might be used at compile time. @todo Export that to other file.
constexpr static bool is_power_of_two(unsigned int n)
{
	if (n == 0)
		return true;

	return (n & (n - 1)) == 0;
}

/**
 * \brief The cyclic buffer.
 *
 *	This structure doesn't handle overflows and the user must be careful to not to pop elements when there are no
 *	new elements in the buffer (when the head is the same as the tail). 
 *
 *	The size of the buffer must be a power of two to increment the tail and the head efficiently.
 */
template <typename T, size_t N> struct cyclic_buf
{
	// Do not allow to compile when the size isn't a power of two.
	static_assert(is_power_of_two(N), "The size of the cyclic buffer must be a power of two");

	//! The buffer where the data is stored.
	volatile T buf[N];

	//! The size of the buffer. Must be a power of two.
	const unsigned int size;

	//! The mask used to keep the head and the tail in boundaries.
	const unsigned int mask;

	//! The head of the buffer - used for incoming data.
	volatile unsigned int head;

	//! The tail of the buffer - used for outgoing data.
	volatile unsigned int tail;

	//! Constructor of the structure
	cyclic_buf();

	//! Pushes an element to the cyclic buffer.
	void push_elem(T val) noexcept;

	//! Pushes n bytes to the cyclic buffer from the array p.
	void push_nelems(const T *p, unsigned int n) noexcept;

	//! Pops an element from the buffer.
	T pop_elem() noexcept;

	//! Pops n elements from the buffer int to the specified array.
	void pop_nelems(T *p, unsigned int n) noexcept;

	//! Checks whether the cyclic buffer is empty.
	bool is_empty() const noexcept;

	//! Returns number of the elements in the buffer.
	unsigned int get_num_elems() const noexcept;
};

template <typename T, size_t N> cyclic_buf<T, N>::cyclic_buf() : size(N), mask(N - 1), head(0), tail(0) {}

template <typename T, size_t N> void cyclic_buf<T, N>::push_elem(T val) noexcept
{
	unsigned int h = head;
	buf[h] = val;
	head = (h + 1) & mask;
}

template <typename T, size_t N> void cyclic_buf<T, N>::push_nelems(const T *p, unsigned int n) noexcept
{
	if(p == NULL)
		return;

	if(n == 0)
		return;

	unsigned int h = head, s = size;
	volatile T *beg = &buf[h];

	// We must check whether there a swing of the buffer will occur. If yes then the data must be splitted
	// into two parts. The first one will be copied at the end of the buffer and the second one will be copied
	// into the beginning of the buffer.
	size_t size_to_end = s - h;
	if (size_to_end < n)
	{
		std::copy(p, p + size_to_end, beg);
		std::copy(p + size_to_end, p + n, buf);
	}
	// If there is enough space at the end of the buffer then perform a simple copy.
	else
	{
		std::copy(p, p + n, beg);
	}

	head = (h + n) & mask;
}

template <typename T, size_t N> T cyclic_buf<T, N>::pop_elem() noexcept
{
	unsigned int t = tail;
	tail = (t + 1) & mask;
	return buf[t];
}

template <typename T, size_t N> bool cyclic_buf<T, N>::is_empty() const noexcept
{
	return head == tail;
}

template <typename T, size_t N> unsigned int cyclic_buf<T, N>::get_num_elems() const noexcept
{
	unsigned int t = tail, h = head;
	if(t > h)
		return size - t + h;
	else
		return h - t;
}

template <typename T, size_t N> void cyclic_buf<T, N>::pop_nelems(T *p, unsigned int n) noexcept
{
	if(p == NULL)
		return;

	if(n == 0)
		return;

	unsigned int t = tail, s = size;
	volatile T *beg = &buf[t];

	// We must check whether there a swing of the buffer will occur. If yes then the data must be splitted
	// into two parts. The first one will be copied from the end of the buffer and the second one will be copied
	// from the beginning of the buffer.
	size_t size_to_end = s - t;
	if (size_to_end < n)
	{
		std::copy(beg, beg + size_to_end, p);
		std::copy(buf, buf + n - size_to_end, p + size_to_end);
	}
	// If a swing won't occur while copying then perform a simple copy.
	else
	{
		std::copy(beg, beg + n, p);
	}

	tail = (t + n) & mask;
}

#endif /* CYCLIC_BUF_HPP */
