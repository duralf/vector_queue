#define CATCH_CONFIG_MAIN
#define VECTOR_QUEUE_HAS_SSE
#include <catch.hpp>
#include <vector_queue.h>

template <class T>
bool equals(const vector_queue<T>& q, std::initializer_list<T> l)
{
	return std::equal(q.begin(), q.end(), l.begin(), l.end());
}

TEST_CASE("push_pop")
{
	vector_queue<int> q{1,2,3,4};
	REQUIRE(equals(q, { 1,2,3,4 }));
	//REQUIRE(q == {1, 2, 3, 4});
	q.emplace_back(5);
	q.pop_front();

	q.emplace_back(5);
	q.pop_front();

	q.emplace_back(5);
	q.pop_front();
	REQUIRE(equals(q, { 4, 5, 5, 5 }));
	q.erase(std::find(q.begin(), q.end(), 4));
	REQUIRE(equals(q, { 5, 5, 5 }));
	q.emplace_front(4);
	q.emplace_back(6);
	REQUIRE(equals(q, { 4, 5, 5, 5, 6}));
	q.erase(std::find(q.begin(), q.end(), 6));
	REQUIRE(equals(q, { 4, 5, 5, 5 }));
	q.pop_back();
	REQUIRE(equals(q, { 4, 5, 5 }));

}


TEST_CASE("push_pop_complex")
{
	using namespace  std::string_literals;
	vector_queue<std::string> q;
	q.emplace_back("test");
	q.emplace_back("hello world!");
	q.pop_front();
	q.emplace_front("test\n");
	q.emplace_front("test2\n");
	q.emplace_front("at capacity!\n");
	REQUIRE(equals(q, { "at capacity!\n"s , "test2\n"s, "test\n"s,  "hello world!"s }));
	q.emplace_front("realloc!\n");
	REQUIRE(equals(q, { "realloc!\n"s, "at capacity!\n"s , "test2\n"s, "test\n"s,  "hello world!"s }));
	q.erase(std::find(q.begin(), q.end(), "hello world!"));
	REQUIRE(equals(q, { "realloc!\n"s, "at capacity!\n"s , "test2\n"s, "test\n"s}));
}

TEST_CASE("erase test")
{
	auto init = { 1,2,3,4 };
	vector_queue<int> q(init);
	q.erase(q.begin(), q.end());
	REQUIRE(equals(q, {}));
	q = init;
	REQUIRE(equals(q, init));
	q.erase(++q.begin(), --q.end());
	REQUIRE(equals(q, { 1,4 }));
	q.erase(q.begin());
	REQUIRE(equals(q, { 4 }));
	
}


TEST_CASE("insert test")
{
	auto init = { 1,2,3,4 };
	vector_queue<int> q(init);
	q.insert(q.begin() + 1, init.begin(), init.end());
	REQUIRE(equals(q, { 1,1,2,3,4,2,3,4 }));
	q = init;
	q.insert(q.end(), init.begin(), init.end());
	REQUIRE(equals(q, { 1,2,3,4,1,2,3,4 }));
	q = init;
	q.insert(q.begin(), init.begin(), init.end());
	REQUIRE(equals(q, { 1,2,3,4,1,2,3,4 }));

	vector_queue<int>::iterator it = q.begin();
	vector_queue<int>::const_iterator it2 = it;
	*it = 5;
	q.clear();

	q.insert(q.end(), init.begin(), init.end());
	REQUIRE(equals(q, init));
	q.clear();

	q.insert(q.begin(), init.begin(), init.end());
	REQUIRE(equals(q, init));
	REQUIRE(q.capacity() >= 8);
	q.insert(q.begin(), init.begin(), init.end());
	REQUIRE(equals(q, { 1,2,3,4,1,2,3,4 }));
	q.clear();
	q.insert(q.begin(), init.begin(), init.end());
	REQUIRE(equals(q, init));
	q.insert(q.begin() + 1, init.begin(), init.end());
	REQUIRE(equals(q, { 1,1,2,3,4,2,3,4 }));
	q.erase(q.begin() + 1, q.begin() + 5);
	q.insert(q.end(), init.begin(), init.end());
	REQUIRE(equals(q, { 1,2,3,4,1,2,3,4 }));

	q.erase(q.begin(), q.begin() + 4);
	q.insert(q.end()-1, init.begin(), init.end());
	REQUIRE(equals(q, { 1,2,3,1,2,3,4,4 }));


}

TEST_CASE("emplace test")
{
	auto init = { 1,2,3,4 };
	vector_queue<int> q(init);
	q.erase(q.end() - 2);
	REQUIRE(equals(q, { 1,2,4 }));

	q.emplace(q.end()-1, 3);
	REQUIRE(equals(q, { 1,2,3,4 }));
	q.erase(q.begin()+1);
	q.emplace(q.begin() + 1, 2);
	REQUIRE(equals(q, { 1,2,3,4 }));
	q.emplace(q.end() - 1, 3);
	REQUIRE(equals(q, { 1,2,3,3,4 }));
	q = {};
	q.emplace(q.end(), 1);
	REQUIRE(equals(q, { 1 }));
}

TEST_CASE("rounding")
{
	vector_queue<uint64_t> q;
	q.reserve(2);
	REQUIRE(q.capacity() == 2);
	q.reserve(3);
	REQUIRE(q.capacity() == 4);
	q.reserve(8);
	REQUIRE(q.capacity() == 8);
	q.reserve(9);
	REQUIRE(q.capacity() == 16);

}

TEST_CASE("find small queue")
{
	auto init = std::initializer_list<uint8_t>{ 1,2,3,4,1,2,3,4,1,2,3,4,1,2,3,4 };
	vector_queue<uint8_t> q{init};
	q.pop_front();
	q.emplace_back(5);
	REQUIRE(q.capacity() == q.size());
	auto it = q.find(5);
	REQUIRE(it != q.end());
	REQUIRE(*it == 5);
	q.erase(q.begin(), q.begin() + 8);
	it = q.find(5);
	REQUIRE(it != q.end());
	REQUIRE(*it == 5);
	
}

TEST_CASE("find big queue")
{
	auto init = std::initializer_list<uint8_t>{ 1,2,3,4,1,2,3,4,1,2,3,4,1,2,3,4,1,2,3,4,1,2,3,4,1,2,3,4,1,2,3,4 };
	vector_queue<uint8_t> q{ init };
	q.pop_front();
	q.emplace_back(5);
	REQUIRE(q.capacity() == q.size());
	auto it = q.find(5);
	REQUIRE(it != q.end());
	REQUIRE(*it == 5);
	q[18] = 8;
	it = q.find(8);
	REQUIRE(it != q.end());
	REQUIRE(*it == 8);
	REQUIRE(it - q.begin() == 18);
	*it = 0;
	q.erase(q.begin(), q.begin() + 8);
	it = q.find(5);
	REQUIRE(it != q.end());
	REQUIRE(*it == 5);
	q[18] = 8;
	it = q.find(8);
	REQUIRE(it != q.end());
	REQUIRE(*it == 8);
	REQUIRE(it - q.begin() == 18);
}


TEST_CASE("find big element SSE")
{
	auto init = std::initializer_list<uint16_t>{ 1,2,3,4,1,2,3,4,1,2,3,4,1,2,3,4 };
	vector_queue<uint16_t> q{ init };
	q.pop_front();
	q.emplace_back(5);
	REQUIRE(q.capacity() == q.size());
	auto it = q.find(5);
	REQUIRE(it != q.end());
	REQUIRE(*it == 5);
	it = q.find(5);
	REQUIRE(it != q.end());
	REQUIRE(*it == 5);
	q[10] = 8;
	it = q.find(8);
	REQUIRE(it != q.end());
	REQUIRE(*it == 8);
	REQUIRE(it - q.begin() == 10);
}

TEST_CASE("find bigger element SSE")
{
	auto init = std::initializer_list<uint32_t>{ 1,2,3,4,1,2,3,4,1,2,3,4,1,2,3,4 };
	vector_queue<uint32_t> q{ init };
	q.pop_front();
	q.emplace_back(5);
	REQUIRE(q.capacity() == q.size());
	auto it = q.find(5);
	REQUIRE(it != q.end());
	REQUIRE(*it == 5);
	it = q.find(5);
	REQUIRE(it != q.end());
	REQUIRE(*it == 5);
	q[10] = 8;
	it = q.find(8);
	REQUIRE(it != q.end());
	REQUIRE(*it == 8);
	REQUIRE(it - q.begin() == 10);
}