#pragma once

#include <atomic>
#include <thread>

// =============================================================================
// SpinLock — 自旋锁
// 锁被占用时循环等待并 yield，适合临界区极短的场景
// =============================================================================
class SpinLock
{
public:
	// --- 加锁 / 解锁 ---

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

// =============================================================================
// Guard — RAII 锁守卫
// 构造时自动 Acquire，析构时自动 Release
// =============================================================================
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
