// =============================================================================
// ThreadPool — 三类线程协作（与 main.cpp 配合）
//
//   【主线程】  创建 ThreadPool、Wait()、Save
//   【入队线程】 main 里 std::thread 跑 ParallerlFor，只 AddTask
//   【Worker】  本文件 WorkerThread，取队列任务并 run()
// =============================================================================

#include "thread_pool.h"

#include <atomic>

// =============================================================================
// ParallelForTask — ParallelFor 的内部任务（仅本文件使用）
// 保存坐标 (x, y) 和共享回调，run() 时在【Worker 线程】调用 lambda(x, y)
// =============================================================================
class ParallelForTask : public Task
{
public:
	ParallelForTask(size_t x, size_t y, const std::function<void(size_t, size_t)>& lambda)
		: x(x), y(y), lambda(lambda)
	{
	}

	void run() override
	{
		lambda(x, y);
	}

private:
	size_t x, y;
	std::function<void(size_t, size_t)> lambda;
};

// =============================================================================
// 生命周期 — 【主线程】构造时启动 Worker×N
// =============================================================================

ThreadPool::ThreadPool(size_t thread_count)
{
	alive = 1;

	if (thread_count == 0)
		thread_count = std::thread::hardware_concurrency();

	for (size_t i = 0; i < thread_count; i++)
		threads.emplace_back(ThreadPool::WorkerThread, this);
}

ThreadPool::~ThreadPool()
{
	Wait();

	alive = 0;

	for (auto& thread : threads)
		thread.join();

	threads.clear();
}

// =============================================================================
// 同步 — 【主线程】Wait()：等 pending_task_count 归零
// =============================================================================
void ThreadPool::Wait() const
{
	while (pending_task_count.load(std::memory_order_acquire) > 0)
		std::this_thread::yield();
}

// =============================================================================
// Worker 线程 — 循环取任务、执行 run()、pending_task_count--
// =============================================================================

void ThreadPool::WorkerThread(ThreadPool* master)
{
	while (master->alive == 1)
	{
		Task* task = master->GetTask();
		if (task != nullptr)
		{
			task->run();
			delete task;
			master->pending_task_count.fetch_sub(1, std::memory_order_release);
		}
		else
			std::this_thread::yield();
	}
}

// =============================================================================
// 并行工具 — 【入队线程】调用：只创建 Task 并 AddTask，不跑 lambda
// =============================================================================

void ThreadPool::ParallerlFor(size_t width, size_t height, const std::function<void(size_t, size_t)>& lambda)
{
	for (size_t y = 0; y < height; y++)
	{
		for (size_t x = 0; x < width; x++)
			AddTask(new ParallelForTask(x, y, lambda));
	}
}

// =============================================================================
// 任务队列 — 【入队线程】AddTask / 【Worker】GetTask
// =============================================================================

void ThreadPool::AddTask(Task* task)
{
	Guard guard{ spin_lock };
	tasks.push(task);  // 入队尾
	pending_task_count.fetch_add(1, std::memory_order_release);
}

Task* ThreadPool::GetTask()
{
	Guard guard{ spin_lock };

	if (tasks.empty())
		return nullptr;

	Task* task = tasks.front();
	tasks.pop();  // 出队头（先进先出）
	return task;
}
