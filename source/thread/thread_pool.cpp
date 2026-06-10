#include "thread_pool.h"

#include <atomic>

ThreadPool thread_pool{};

// 并行循环里的单个任务
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

// 构造 / 析构
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

// 等任务跑完
void ThreadPool::Wait() const
{
	while (pending_task_count.load(std::memory_order_acquire) > 0)
		std::this_thread::yield();
}

// 工作线程
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

// 提交二维网格任务
void ThreadPool::ParallerlFor(size_t width, size_t height, const std::function<void(size_t, size_t)>& lambda)
{
	for (size_t y = 0; y < height; y++)
	{
		for (size_t x = 0; x < width; x++)
			AddTask(new ParallelForTask(x, y, lambda));
	}
}

// 任务队列
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
