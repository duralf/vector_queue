#define CATCH_CONFIG_MAIN
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