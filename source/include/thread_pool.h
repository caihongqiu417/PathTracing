#pragma once

#include <functional>
#include <list>
#include <thread>
#include <vector>

#include "spin_lock.h"

/// 任务基类，子类实现 run() 以定义具体逻辑
class Task
{
public:
	virtual void run() = 0;
	virtual ~Task() = default;
};

/// 固定数量工作线程 + 任务队列，支持自定义 Task 与 ParallelFor
class ThreadPool
{
public:
	/// @param thread_count 工作线程数；为 0 时使用 CPU 核心数
	ThreadPool(size_t thread_count = 0);
	~ThreadPool();

	/// 工作线程入口，从队列取任务并执行
	static void WorkerThread(ThreadPool* master);

	/// 对 [0, width) × [0, height) 每个坐标提交一个并行任务
	void ParallerlFor(size_t width, size_t height, const std::function<void(size_t, size_t)>& lambda);

	void AddTask(Task* task);
	Task* GetTask();

	/// 阻塞直到任务队列被取空（不保证任务已执行完毕）
	void Wait() const;

private:
	std::atomic<int> alive;              // 1 = 运行中，0 = 请求退出
	std::vector<std::thread> threads;
	std::list<Task*> tasks;
	SpinLock spin_lock;
};
