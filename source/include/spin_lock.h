#pragma once

#include <atomic>
#include <thread>

/// 自旋锁：锁被占用时循环等待，适合临界区极短的场景
class SpinLock
{
public:
	void Acquire()
	{
		while (flag.test_and_set(std::memory_order_acquire))
			std::this_thread::yield();
	}

	void Release()
	{
		flag.clear(std::memory_order_release);
	}

private:
	std::atomic_flag flag{};  // false = 空闲，true = 已锁定
};

/// RAII 锁守卫：构造时加锁，析构时自动解锁
class Guard
{
public:
	explicit Guard(SpinLock& spin_lock)
		: spin_lock(spin_lock)
	{
		spin_lock.Acquire();
	}

	~Guard()
	{
		spin_lock.Release();
	}

private:
	SpinLock& spin_lock;
};
