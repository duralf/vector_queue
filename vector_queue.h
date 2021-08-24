#pragma once
/*
Copyright (c) 2021 Christian Olsson

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/


#include <type_traits>
#include <memory>
#include <iterator>
#include <algorithm>

template <class T, class Alloc = std::allocator<T>>
struct vector_queue
{
private:
	struct dummy_value {};
	union array_type
	{
		T value;
		dummy_value dummy;
		array_type() : dummy{} {}
		~array_type() {}
	};
public:
	using allocator_type = typename std::allocator_traits<Alloc>::template rebind_alloc<array_type>;

private:
	static constexpr size_t smallest_alloc = sizeof(size_t) * 4; // no point in allocating tiny areas
	static constexpr size_t initial_capacity = std::max(size_t(4), smallest_alloc / sizeof(T));
	array_type* array;
	size_t _size;
	size_t _capacity;
	size_t start;
	[[no_unique_address]] allocator_type alloc;


	template <class... Args>
	void emplace_front_no_grow(Args&&... args)
	{
		if (start == 0) {
			std::construct_at(&array[capacity() - 1].value, std::forward<Args>(args)...); // wrap around
			start = capacity();
		}
		else
			std::construct_at(&array[start - 1].value, std::forward<Args>(args)...);
		--start;
		++_size;
	}


	template <class... Args>
	void emplace_back_no_grow(Args&&... args)
	{
		std::construct_at(&(*this)[_size], std::forward<Args>(args)...);
		++_size;
	}

	template <class Iter>
	void insert_n_front(size_t n, Iter first)
	{
		for(size_t i = 0; i < n; ++i)
		{
#ifndef VECTOR_QUEUE_NO_EXCEPTIONS
			try {
#endif
				size_t index;
				if (start + i < n)
					index = start + capacity() - n + i;
				else
					index = start - n + 1;
				std::construct_at(&array[index].value, *first);
				++first;
#ifndef VECTOR_QUEUE_NO_EXCEPTIONS
			} catch (...)
			{
				for (size_t j = 0; j < i; ++j)
				{
					size_t index;
					if (start + j < i)
						index = start + capacity() - i + j;
					else
						index = start - i + 1;
					std::destroy_at(&array[index].value);
				}
				throw;
			}
#endif
		}
		_size += n;
		if (start < n)
			start = capacity() - start - n;
		else
			start -= n;
	}

	template <class V>
	struct iter_templ
	{
		typedef ptrdiff_t difference_type;
		typedef T value_type;
		typedef V* pointer;
		typedef V& reference;
		typedef std::random_access_iterator_tag iterator_category;
		using container_type = std::conditional_t<std::is_const_v<V>, const vector_queue<T>, vector_queue<T>>;

		V& operator*() { return (*container)[index]; }
		V* operator->() { return &(*container)[index]; }

		iter_templ<V>& operator++()
		{
			++index;
			return *this;
		}

		iter_templ<V> operator++(int)
		{
			return {index++, *container};
		}

		iter_templ<V>& operator--()
		{
			--index;
			return *this;
		}

		iter_templ<V> operator--(int)
		{
			return { index--, *container };
		}

		bool operator!=(const iter_templ<V>& other) const
		{
			return index != other.index;
		}

		bool operator==(const iter_templ<V>& other) const = default;

		ptrdiff_t operator-(const iter_templ<V>& other) const
		{
			return ptrdiff_t(index - other.index);
		}

		iter_templ<V>& operator+=(ptrdiff_t diff)
		{
			index += diff;
			return *this;
		}

		iter_templ<V>& operator-=(ptrdiff_t diff)
		{
			index -= diff;
			return *this;
		}

		iter_templ<V> operator+(ptrdiff_t diff) const
		{
			auto tmp = *this;
			return tmp += diff;
		}

		iter_templ<V> operator-(ptrdiff_t diff) const
		{
			auto tmp = *this;
			return tmp -= diff;
		}

		std::strong_ordering operator<=>(const iter_templ& other) const
		{
			return index <=> other.index;
		}

		//template <class = std::enable_if_t<!std::is_const_v<V>>>
		operator iter_templ<const T>() const
		{
			return { index, *container };
		}


		iter_templ(size_t index, container_type& container) : index(index), container(&container) {}
	private:
		size_t index;
		container_type* container;
	};

public:
	using value_type = T;
	using reference = T&;
	using const_reference = const T&;
	using iterator = iter_templ<T>;
	using const_iterator = iter_templ<const T>;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;
	using difference_type = ptrdiff_t;

	constexpr vector_queue() noexcept(noexcept(allocator_type())) : array{}, _size{}, _capacity{}, start{}, alloc{}
	{}
	constexpr explicit vector_queue(const allocator_type& alloc) noexcept : array{}, _size{}, _capacity{}, start{}, alloc{alloc}
	{}
	vector_queue(std::initializer_list<T> values, const allocator_type& alloc = allocator_type()) : _size{}, _capacity(values.size()), start{}, alloc(alloc)
	{
		array = this->alloc.allocate(values.size());
		for(auto& val: values)
		{
			std::construct_at(&array[_size++].value, val);
		}
	}

	vector_queue(vector_queue<T>&& other) noexcept : array(other.array), _size(other._size), _capacity(other._capacity), start(other.start), alloc(std::move(other.alloc))
	{
		other._size = 0;
		other.array = nullptr;
		other._capacity = 0;
	}

	vector_queue(const vector_queue<T>& other) : _capacity(other._size), _size{}, start{}, alloc(other.alloc)
	{
		array = alloc.allocate(capacity());
		for (auto& val : other)
		{
			std::construct_at(&array[_size++].value, val);
		}
	}

	vector_queue<T, Alloc>& operator=(vector_queue<T, Alloc>&& other) noexcept
	{
		if (this == &other)
			return *this;

		array = other.array;
		_size = other.size();
		_capacity = other.capacity();
		start = other.start;
		alloc = std::move(other.alloc);
		other._size = 0;
		other.array = nullptr;
		other._capacity = 0;
		return *this;
	}

	vector_queue<T, Alloc>& operator=(const vector_queue<T, Alloc>& other)
	{
		if (this == &other)
			return *this;

		clear();
		if (capacity() < other.size())
			realloc(other.size());
		for (auto& val : other)
		{
			std::construct_at(&array[_size++].value, val);
		}
		return *this;
	}

	iterator begin() { return { 0, *this }; }
	iterator end() { return { _size, *this }; }
	const_iterator begin() const { return { 0, *this }; }
	const_iterator end() const { return { _size, *this }; }
	const_iterator cbegin() const { return { 0, *this }; }
	const_iterator cend() const { return { _size, *this }; }
	reverse_iterator rbegin() { return reverse_iterator{ iterator{ _size, *this} }; }
	reverse_iterator rend() { return reverse_iterator{ iterator{ 0, *this} }; }
	const_reverse_iterator crbegin() const { return const_reverse_iterator{ const_iterator{ _size, *this} }; }
	const_reverse_iterator crend() const { return const_reverse_iterator{ const_iterator{ 0, *this} }; }

	constexpr bool empty() const
	{
		return size() == 0;
	}

	T& front()
	{
		return (*this)[0];
	}

	const T& front() const
	{
		return (*this)[0];
	}

	T& back()
	{
		return (*this)[size() - 1];
	}

	const T& back() const
	{
		return (*this)[size() - 1];
	}

	T& operator[](size_t index)
	{
		if (start + index < capacity())
			return array[start + index].value;
		return array[start + index - capacity()].value;
	}

	const T& operator[](size_t index) const
	{
		if (start + index < capacity())
			return array[start + index].value;
		return array[start + index - capacity()].value;
	}


	template <class... Args, class = std::enable_if_t<std::is_constructible_v<T, Args...>>>
	void emplace_back(Args&&... args)
	{
		if (size() == capacity()) {
			grow();
			std::construct_at(&array[_size].value, std::forward<Args>(args)...);
			++_size;
		}
		else
		{
			emplace_back_no_grow(std::forward<Args>(args)...);
		}
	}

	void push_back(const T& value)
	{
		emplace_back(value);
	}

	void push_back(T&& value)
	{
		emplace_back(std::move(value));
	}

	template <class... Args, class = std::enable_if_t<std::is_constructible_v<T, Args...>>>
	void emplace_front(Args&&... args)
	{
		if (size() == capacity()) {
			grow();
			std::construct_at(&array[capacity() - 1].value, std::forward<Args>(args)...); // wrap around
			start = capacity() - 1;
			++_size;
		}
		else
		{
			emplace_front_no_grow(std::forward<Args>(args)...);
		}
	}

	void push_front(const T& value)
	{
		emplace_front(value);
	}

	void push_front(T&& value)
	{
		emplace_front(std::move(value));
	}

	void clear()
	{
		for_each_index([this](size_t ix)
			{
				std::destroy_at(&array[ix].value);
			});
		start = 0;
		_size = 0;
	}

	void reserve(size_t new_capacity)
	{
		if (new_capacity > capacity())
			realloc(new_capacity);
	}

	void pop_front()
	{
		std::destroy_at(&(*this)[0]);
		start++;
		if (start == capacity())
			start = 0;
		--_size;
	}

	void pop_back()
	{
		std::destroy_at(&(*this)[_size-1]);
		--_size;		
	}

	void shrink_to_fit()
	{
		realloc(_size);
	}

	constexpr size_t size() const
	{
		return _size;
	}

	constexpr size_t capacity() const
	{
		return _capacity;
	}

	template <class V>
	void erase(iter_templ<V> first, iter_templ<V> last)
	{
		auto n = last - first;
		if (n == size()) {
			clear();
			return;
		}
		if(end() - last <= ptrdiff_t(size() / 2))
		{
			while (first + n < end())
			{
				*first = std::move(*(first + n));
				++first;
			} 
			while(n > 0)
			{
				pop_back();
				--n;
			}
		}
		else
		{
			if(first != begin())
			{
				do
				{
					--first;
					*(first + n) = std::move(*first);
				} while (first != begin());
			}
			while (n > 0)
			{
				pop_front();
				--n;
			}
		}
	}

	template <class V>
	void erase(iter_templ<V> pos)
	{
		erase(pos, pos + 1);
	}


	template <class V, class Iter>
	iterator insert(iter_templ<V> where, Iter first, Iter last)
	{
		if (first == last)
			return where;
		size_t n = std::distance(first, last);
		if (n + size() > capacity())
		{
			vector_queue<T> tmp;
			tmp.reserve(std::max(n + size(), next_capacity()));
			tmp.start = where - begin();
			auto first_inserted = tmp.start;

			// insert the new range first in case of an exception
			for (auto it = first; it != last; ++it)
			{
				std::construct_at(&tmp.array[tmp.start + tmp._size].value, *it);
				++tmp._size;
			}

			// move the last part from the current vector_queue
			for (auto it = where; it != end(); ++it)
			{
				std::construct_at(&tmp.array[tmp.start + tmp._size].value, std::move(*it));
				++tmp._size;
			}

			for (auto it = std::reverse_iterator<iterator>(where); it != rend(); ++it)
			{
				std::construct_at(&tmp.array[tmp.start - 1].value, std::move(*it));
				--tmp.start;
				++tmp._size;
			}

			swap(tmp);
			return { first_inserted, *this };
		}
		if(size_t(where - begin()) < size() / 2)
		{
			if(where == begin())
			{
				insert_n_front(n, first);
				return begin();
			}

			// insert n empty values
			for(size_t i = 0; i < n; ++i)
			{
				emplace_front_no_grow();
			}
			for(auto it = begin()+n; it != where + n;++it)
			{
				*(it - n) = std::move(*it);
			}

			iterator insert_place = { size_t(where - begin()), *this };

			for (size_t i = 0; i < n; ++i)
			{
				*(insert_place+i) = *first;
				++first;
			}
			return insert_place;
		}
		else
		{
			if (where == end())
			{
				std::for_each(first, last, [this](auto& value)
					{
						emplace_back_no_grow(value);
					});
				return end() - n;
			}

			// insert n empty values
			for (size_t i = 0; i < n; ++i)
			{
				emplace_back_no_grow();
			}
			for (auto it = where; it != end()-n; ++it)
			{
				*(it + n) = std::move(*it);
			}

			iterator insert_place = { size_t(where - begin()), *this };			

			for (size_t i = 0; i < n; ++i)
			{
				*(insert_place+i) = *first;
				++first;
			}
			return insert_place;
		}
	}

	template <class V, class... Args, class = std::enable_if_t<std::is_constructible_v<T, Args...>>>
	iterator emplace(iter_templ<V> where, Args&&... args)
	{
		if (size() == capacity())
		{
			vector_queue<T> tmp;
			tmp.reserve(std::max(initial_capacity, next_capacity()));
			tmp.start = where - begin();
			auto first_inserted = tmp.start;

			// construct the value first in case of an exception
			std::construct_at(&tmp.array[tmp.start + tmp._size].value, std::forward<Args>(args)...);
			++tmp._size;

			// move the last part from the current vector_queue
			for (auto it = where; it != end(); ++it)
			{
				std::construct_at(&tmp.array[tmp.start + tmp._size].value, std::move(*it));
				++tmp._size;
			}

			for (auto it = std::reverse_iterator<iterator>(where); it != rend(); ++it)
			{
				std::construct_at(&tmp.array[tmp.start - 1].value, std::move(*it));
				--tmp.start;
				++tmp._size;
			}

			swap(tmp);
			return { first_inserted, *this };
		}
		if (where == begin())
		{
			emplace_front_no_grow(std::forward<Args>(args)...);
			return begin();
		}
		if(where == end())
		{
			emplace_back_no_grow(std::forward<Args>(args)...);
			return --end();
		}
		if (size_t(where - begin()) < size() / 2)
		{
			emplace_front_no_grow(std::move(front()));
			// the where iterator now points to the place we want to put the new value
			for(auto it = ++begin(); true;)
			{
				*it = std::move(*(it + 1));
				++it;
				if(it == where)
				{
					*where = T{ std::forward<Args>(args)... };
					return it;
				}

			}
		}
		else
		{
			emplace_back_no_grow(std::move(back()));
			// the where iterator is unchanged
			for(auto it = end()-2; true; --it)
			{
				*(it + 1) = std::move(*it);
				if (it == where)
				{
					*where = T{ std::forward<Args>(args)... };
					return it;
				}
			}
		}
	}

	template <class V>
	iterator insert(iter_templ<V> where, const T& value)
	{
		return emplace(where, value);
	}

	template <class V>
	iterator insert(iter_templ<V> where, T&& value)
	{
		return emplace(where, std::move(value));
	}

	void swap(vector_queue<T,Alloc>& other) noexcept(std::allocator_traits<Alloc>::propagate_on_container_swap::value
		|| std::allocator_traits<Alloc>::is_always_equal::value)
	{
		std::swap(array, other.array);
		std::swap(start, other.start);
		std::swap(_capacity, other._capacity);
		std::swap(_size, other._size);
		std::swap(alloc, other.alloc);
	}
private:

	void realloc(size_t new_capacity)
	{
		auto tmp = vector_queue<T, Alloc>{};
		tmp.array = this->alloc.allocate(new_capacity);
		tmp._capacity = new_capacity;
		for_each_index([this, &tmp](size_t ix)
			{
				std::construct_at(&tmp.array[tmp._size++].value, std::move(array[ix].value));
				std::destroy_at(&array[ix].value);
			});
		alloc.deallocate(array, capacity());
		swap(tmp);
	}

	template <class Func>
	void for_each_index(Func&& f)
	{
		if (start + size() > capacity()) {// wrapping
			for (size_t i = start; i < capacity(); ++i)
				f(i);
			auto end = start + size() - capacity();
			for (size_t i = 0; i < end; ++i)
				f(i);
		}
		else
		{
			auto end = start + size();
			for (size_t i = start; i < end; ++i)
				f(i);
		}
	}

	size_t next_capacity() const
	{
		if (_capacity == 0)
			return initial_capacity;
		auto new_capacity = _capacity + (_capacity >> 1);
		// round up to nearest smallest_alloc bytes
		if constexpr (smallest_alloc / sizeof(T) > 1) {
			new_capacity = new_capacity + (smallest_alloc-1) / sizeof(T);
			new_capacity = new_capacity & ~(smallest_alloc / sizeof(T) - 1);
		}
		return new_capacity;
	}

	void grow()
	{
		if(capacity() == 0)
		{
			array = alloc.allocate(initial_capacity);
			_capacity = initial_capacity;
		}
		else
		{
			realloc(next_capacity());
		}
	}
};

