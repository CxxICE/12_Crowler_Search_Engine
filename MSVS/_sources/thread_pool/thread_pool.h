#pragma once

#include <iostream>
#include <thread>
#include <latch>
#include <functional>

#include <safe_queue/safe_queue.h>
#include <safe_queue/safe_queue.cpp>

class ThreadPool
{
public:
	ThreadPool();
	ThreadPool(const ThreadPool & other) = delete;
	ThreadPool(ThreadPool && other) = delete;	
	ThreadPool &operator=(const ThreadPool &other) = delete;
	ThreadPool &operator=(ThreadPool &&other) = delete;
		
	void submit(std::function<void()> task);	

	~ThreadPool();

private:
	const int _cores = std::thread::hardware_concurrency();
	std::latch _waitCores{ _cores };
	std::vector<std::jthread> _pool;
	SafeQueue<std::function<void()>> _tasks;
	std::stop_source _stoper;	

	void work(std::stop_token stop);
};

