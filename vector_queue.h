#pragma once
#include <type_traits>
#include <memory>
#include <iterator>

template <class T>
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
	array_type* array;
	size_t _size;
	size_t _capacity;
	size_t start;


	template <class V>
	struct iter_templ
	{
		typedef ptrdiff_t difference_type;
		typedef T value_type;
		typedef V* pointer;
		typedef V& reference;
		typedef std::random_access_iterator_tag iterator_category;

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

		ptrdiff_t operator-(const iter_templ<V>& other)
		{
			return ptrdiff_t(index - other.index);
		}

		iter_templ(size_t index, vector_queue<T>& container) : index(index), container(&container) {}
	private:
		size_t index;
		vector_queue<T>* container;
	};

public:
	using iterator = iter_templ<T>;
	using const_iterator = iter_templ<const T>;

	vector_queue() : array{}, _size{}, _capacity{}, start{}
	{}

	vector_queue(std::initializer_list<T> values) : _size{}, _capacity(values.size()), start{}
	{
		array = new array_type[values.size()];
		for(auto& val: values)
		{
			std::construct_at(&array[_size++].value, val);
		}
	}

	vector_queue(vector_queue<T>&& other) noexcept : array(other.array), _size(other._size), _capacity(other._capacity), start(other.start)
	{
		other._size = 0;
		other.array = nullptr;
		other._capacity = 0;
	}

	vector_queue(const vector_queue<T>& other) : _capacity(other._capacity), _size{}, start{}
	{
		array = new array_type[_capacity];
		for (auto& val : other)
		{
			std::construct_at(&array[_size++].value, val);
		}
	}

	iterator begin() { return { 0, *this }; }
	iterator end() { return { _size, *this }; }
	const_iterator begin() const { return { 0, *this }; }
	const_iterator end() const { return { _size, *this }; }

	T& operator[](size_t index)
	{
		if (start + index < _capacity)
			return array[start + index].value;
		return array[start + index - _capacity].value;
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
		for_each_index(begin(), end(), [this](size_t ix)
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
		if (start == _capacity)
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

	template <class V>
	void erase(iter_templ<V> pos)
	{
		if(pos - begin() < ptrdiff_t(_size / 2))
		{
			std::destroy_at(&*pos);
			while (pos != begin())
			{
				auto prev = --iter_templ<V>(pos);
				*pos = std::move(*prev);
				pos = prev;
			} 
			if (++start == capacity())
				start = 0;
		}
		else
		{
			std::destroy_at(&*pos);
			auto last = --end();
			while (pos != last)
			{
				auto next = ++iter_templ<V>(pos);
				*pos = std::move(*next);
				pos = next;
			}
		}
		--_size;
	}

private:

	T* alloc_back()
	{
		if (_size == _capacity) {
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

		if (_size == _capacity) {
			grow();
			start = _capacity - 1; // wrap around
			++_size;
			return &array[start].value;
		}
		else
		{
			if (start == 0)
				start = _capacity;
			++_size;
			return &array[--start].value;
		}
	}

	void realloc(size_t new_capacity)
	{
		auto tmp = std::make_unique<array_type[]>(new_capacity);
		size_t new_index = 0;;
		for_each_index([this, &tmp, &new_index](size_t ix)
			{
				std::construct_at(&tmp.get()[new_index++].value, std::move(array[ix].value));
				std::destroy_at(&array[ix].value);
			});
		delete[] array;
		array = tmp.release();
		start = 0;
		_capacity = new_capacity;
	}

	template <class Func>
	void for_each_index(Func&& f)
	{
		if (start + _size > _capacity) {// wrapping
			for (size_t i = start; i < _capacity; ++i)
				f(i);
			auto end = start + _size - _capacity;
			for (size_t i = 0; i < end; ++i)
				f(i);
		}
		else
		{
			auto end = start + _size;
			for (size_t i = start; i < end; ++i)
				f(i);
		}
	}

	void grow()
	{
		if(_capacity == 0)
		{
			array = new array_type[inital_capacity];
			_capacity = inital_capacity;
		}
		else
		{
			realloc(_capacity + (_capacity >> 1));
		}
	}


};