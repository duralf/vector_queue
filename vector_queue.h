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

template <class T, class Alloc = std::allocator<T>>
class vector_queue
{
	static constexpr size_t inital_capacity = 4;
	struct dummy_value {};
	union array_type
	{
		T value;
		dummy_value dummy;
		array_type() : dummy{} {}
		~array_type() {}
	};
	using allocator_traits = std::allocator_traits<Alloc>;
	using allocator = typename allocator_traits::template rebind_alloc<array_type>;
	array_type* array;
	size_t _size;
	size_t _capacity;
	size_t start;
	[[no_unique_address]] allocator alloc;



	template <class V, class Deque>
	struct iter_templ
	{
		typedef ptrdiff_t difference_type;
		typedef T value_type;
		typedef V* pointer;
		typedef V& reference;
		typedef std::random_access_iterator_tag iterator_category;

		V& operator*() { return (*container)[index]; }
		V* operator->() { return &(*container)[index]; }

		iter_templ<V, Deque>& operator++()
		{
			++index;
			return *this;
		}

		iter_templ<V, Deque> operator++(int)
		{
			return {index++, *container};
		}

		iter_templ<V, Deque>& operator--()
		{
			--index;
			return *this;
		}

		iter_templ<V, Deque> operator--(int)
		{
			return { index--, *container };
		}

		bool operator!=(const iter_templ<V, Deque>& other) const
		{
			return index != other.index;
		}

		bool operator==(const iter_templ<V, Deque>& other) const = default;

		ptrdiff_t operator-(const iter_templ<V, Deque>& other) const
		{
			return ptrdiff_t(index - other.index);
		}

		iter_templ<V, Deque>& operator+=(ptrdiff_t diff)
		{
			index += diff;
			return *this;
		}

		iter_templ<V, Deque>& operator-=(ptrdiff_t diff)
		{
			index -= diff;
			return *this;
		}

		iter_templ<V, Deque> operator+(ptrdiff_t diff) const
		{
			auto tmp = *this;
			return tmp += diff;
		}

		std::strong_ordering operator<=>(const iter_templ& other) const
		{
			return index <=> other.index;
		}

		iter_templ(size_t index, Deque& container) : index(index), container(&container) {}
	private:
		size_t index;
		Deque* container;
	};

public:
	using iterator = iter_templ<T, vector_queue<T>>;
	using const_iterator = iter_templ<const T, const vector_queue<T>>;
	constexpr vector_queue() noexcept(noexcept(allocator())) : array{}, _size{}, _capacity{}, start{}, alloc{}
	{}
	constexpr explicit vector_queue(const allocator& alloc) noexcept : array{}, _size{}, _capacity{}, start{}, alloc{alloc}
	{}
	vector_queue(std::initializer_list<T> values, const allocator& alloc = allocator()) : _size{}, _capacity(values.size()), start{}, alloc(alloc)
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
		std::construct_at(alloc_back(), std::forward<Args>(args)...);
	}

	void push_back(const T& value)
	{
		std::construct_at(alloc_back(), value);
	}

	template <class... Args, class = std::enable_if_t<std::is_constructible_v<T, Args...>>>
	void emplace_front(Args&&... args)
	{
		std::construct_at(alloc_front(), std::forward<Args>(args)...);
	}


	void push_front(const T& value)
	{
		std::construct_at(alloc_front(), value);
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

	size_t size() const
	{
		return _size;
	}

	size_t capacity() const
	{
		return _capacity;
	}

	template <class V, class Deque>
	void erase(iter_templ<V, Deque> first, iter_templ<V, Deque> last)
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


	template <class V, class Deque>
	void erase(iter_templ<V, Deque> pos)
	{
		erase(pos, pos + 1);
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

	T* alloc_back()
	{
		if (size() == capacity()) {
			grow();
			return &array[_size++].value;
		}
		else
		{
			return &(*this)[_size++];
		}
	}

	T* alloc_front()
	{

		if (size() == capacity()) {
			grow();
			start = capacity() - 1; // wrap around
			++_size;
			return &array[start].value;
		}
		else
		{
			if (start == 0)
				start = capacity();
			++_size;
			return &array[--start].value;
		}
	}

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

	void grow()
	{
		if(capacity() == 0)
		{
			array = alloc.allocate(inital_capacity);
			_capacity = inital_capacity;
		}
		else
		{
			realloc(capacity() + (capacity() >> 1));
		}
	}
};
