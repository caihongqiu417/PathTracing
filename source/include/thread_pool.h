#pragma once

#include <functional>
#include <queue>
#include <thread>
#include <vector>

#include "spin_lock.h"

// 一个任务，线程池会调用 run()
class Task
{
public:
	virtual void run() = 0;
	virtual ~Task() = default;
};

// 线程池：维护一组工作线程和一个任务队列
class ThreadPool
{
public:
	// thread_count 为 0 时，自动用 CPU 核心数
	ThreadPool(size_t thread_count = 0);
	~ThreadPool();

	// 对 [0, width) x [0, height) 的每个坐标提交一个任务
	void ParallerlFor(size_t width, size_t height, const std::function<void(size_t, size_t)>& lambda);

	void AddTask(Task* task);
	Task* GetTask();

	// 等到队列里的任务全部做完
	// ParallelFor 只是提交任务，不会在这里等它们真的跑完
	void Wait() const;

	static void WorkerThread(ThreadPool* master);

private:
	std::atomic<int> alive;                  // 1 表示继续运行，0 表示准备退出
	std::atomic<size_t> pending_task_count{ 0 };  // 还没做完的任务数
	std::vector<std::thread> threads;       // 工作线程
	std::queue<Task*> tasks;                // 等待执行的任务
	SpinLock spin_lock;                     // 保护任务队列
};
