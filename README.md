# vector_queue
vector_queue is a double ended queue implemented as a circular growable vector in C++20. It's intended to be a drop-in replacement for std::vector but with three additional methods: pop_front, emplace_front, and push_front. The focus is to have a simple and bugfree implementation rather than focusing too much on performance. Still, as a C++ developer speed is of course also important. 

vector_queue is currently missing some methods, it's probably not following the exception guarantees that std::vector has (though I've tried to implement it). vector_queue will definitely never have data() since the data is not contiguous. There are some tests but they are not comprehensive! Buyer beware!

# License
vector_queue is licensed under the MIT license.
