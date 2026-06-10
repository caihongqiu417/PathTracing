#pragma once

#include <atomic>
#include <thread>

// 自旋锁
class SpinLock
{
public:
	// 加锁
	void Acquire()
	{
		while (flag.test_and_set(std::memory_order_acquire))
			std::this_thread::yield();
	}

	// 解锁
	void Release()
	{
		flag.clear(std::memory_order_release);
	}

private:
	std::atomic_flag flag{};  // false = 空闲，true = 已锁定
};

// 锁守卫
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
