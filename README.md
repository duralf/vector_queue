# vector_queue
vector_queue is a double ended queue implemented as a circular growable vector. It's intended to be a drop-in replacement for std::vector but with three additional methods: pop_front, emplace_front, and push_front. The focus is to have a simple and bugfree implementation rather than focusing too much on performance. Still, as a C++ developer speed is of course also important. 

vector_queue is currently missing some less and some more important features: It's currently not allocator aware, it's missing some methods, it's probably not following the exception guarantees that std::vector has. I will probably never implement reverse_iterators because I can't be bothered. vector_queue will definitely never have data() since the data is not contiguous.

# License
vector_queue is licensed under the MIT license.
