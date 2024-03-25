#include "safe_queue.h"

template <typename T>
SafeQueue<T>::SafeQueue()
{
	_empty.test_and_set();
}

template <typename T>
void SafeQueue<T>::push(T obj)
{
	{
		std::scoped_lock sl(_m);
		_queue.push(std::move(obj));
		_empty.clear();
	}
	_done.notify_all();
}

template <typename T>
T SafeQueue<T>::pop()
{
	T tmp;
	std::unique_lock ul(_m);
	if (_done.wait_for(ul, 1s, [&]() {return !_empty.test(); }))
	{
		tmp = std::move(_queue.front());
		_queue.pop();
		if (_queue.empty())
		{
			_empty.test_and_set();
		}		
	}
	return tmp;
}

template <typename T>
bool SafeQueue<T>::empty()
{
	return _empty.test();
}
