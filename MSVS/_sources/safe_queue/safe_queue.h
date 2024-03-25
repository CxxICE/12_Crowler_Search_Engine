#pragma once

#include <iostream>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <chrono>

using namespace std::chrono_literals;

template <typename T>
class SafeQueue
{
public:
	SafeQueue();
	SafeQueue(const SafeQueue &other) = delete;
	SafeQueue(SafeQueue &&other) = delete;
	SafeQueue &operator=(const SafeQueue &other) = delete;
	SafeQueue &operator=(SafeQueue &&other) = delete;

	void push(T obj);
	T pop();
	bool empty();

private:
	std::mutex _m;
	std::condition_variable _done;
	std::queue<T> _queue;
	std::atomic_flag _empty;
};

