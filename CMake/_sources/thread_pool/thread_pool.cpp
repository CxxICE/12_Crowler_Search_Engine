#include "thread_pool.h"

ThreadPool::ThreadPool()
{
	std::stop_token stop = _stoper.get_token();
	for (int i = 0; i < _cores; ++i)
	{
		_pool.push_back(std::jthread(&ThreadPool::work, this, stop));
	}	
}

void ThreadPool::work(std::stop_token stop)
{
	_waitCores.arrive_and_wait();//ожидание создания требуемого кол-ва потоков
	while (true)
	{
		auto tmp = _tasks.pop();
		if (tmp)
		{
			//std::cout << std::this_thread::get_id() << std::endl;
			tmp();
		}
		std::this_thread::yield();
		if (stop.stop_requested() && _tasks.empty())
		{
			break;
		}
	};
}

void ThreadPool::submit(std::function<void()> task)
{
	_tasks.push(std::move(task));
}

ThreadPool::~ThreadPool()
{
	if (_stoper.stop_possible())
	{
		_stoper.request_stop();
	}
	for (auto &thread : _pool)
	{
		if (thread.joinable())
		{
			thread.join();
		}
	}	
}


