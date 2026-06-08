#pragma once

#include <functional>
#include <list>
#include <thread>
#include <vector>

#include "spin_lock.h"

// =============================================================================
// Task — 任务基类
// 继承并实现 run() 以定义具体任务逻辑
// =============================================================================
class Task
{
public:
	virtual void run() = 0;
	virtual ~Task() = default;
};

// =============================================================================
// ThreadPool — 线程池
// 固定数量工作线程 + 任务队列，支持自定义 Task 与 ParallelFor
// =============================================================================
class ThreadPool
{
public:
	// --- 生命周期 ---

	/// @param thread_count 工作线程数；为 0 时自动使用 CPU 核心数
	ThreadPool(size_t thread_count = 0);
	~ThreadPool();

	// --- 并行工具 ---

	/// 对 [0, width) × [0, height) 的每个坐标提交一个并行任务
	void ParallerlFor(size_t width, size_t height, const std::function<void(size_t, size_t)>& lambda);

	// --- 任务队列 ---

	void AddTask(Task* task);
	Task* GetTask();

	/// 阻塞直到任务队列为空（不保证任务已全部执行完毕）
	void Wait() const;

	// --- 工作线程 ---

	static void WorkerThread(ThreadPool* master);

private:
	// --- 成员变量 ---

	std::atomic<int> alive;           // 1 = 运行中，0 = 请求退出
	std::vector<std::thread> threads; // 工作线程
	std::list<Task*> tasks;           // 待执行的任务队列
	SpinLock spin_lock;               // 保护 tasks 的并发访问
};
