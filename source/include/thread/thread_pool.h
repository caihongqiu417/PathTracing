#pragma once

#include <functional>
#include <queue>
#include <thread>
#include <vector>

#include "spin_lock.h"

// 一个任务
class Task
{
public:
	virtual void run() = 0;
	virtual ~Task() = default;
};

// 线程池
class ThreadPool
{
public:
	// thread_count 为 0 时，自动取 CPU 核心数
	ThreadPool(size_t thread_count = 0);
	~ThreadPool();

	// 按块切图后提交任务，每个任务负责一片像素区域
	// complexity: 任务复杂度 (0.1~1.0 轻任务，1.0~10.0 正常任务，>10.0 重任务)
	void ParallerlFor(size_t width, size_t height, const std::function<void(size_t, size_t)>& lambda, float complexity = 1.0f);

	// 入队。task 交给线程池，跑完后由 Worker delete
	void AddTask(Task* task);
	Task* GetTask();

	// 等所有任务跑完
	void Wait() const;

	// 工作线程入口
	static void WorkerThread(ThreadPool* master);

private:
	std::atomic<int> alive;                  // 1 表示继续运行，0 表示准备退出
	std::atomic<size_t> pending_task_count{ 0 };  // 还没做完的任务数
	std::vector<std::thread> threads;       // 工作线程
	std::queue<Task*> tasks;                // 等待执行的任务
	SpinLock spin_lock;                     // 保护任务队列
};

extern ThreadPool thread_pool;