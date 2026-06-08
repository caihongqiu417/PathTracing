#include "thread_pool.h"

ThreadPool::ThreadPool(size_t thread_count)
{
	alive = 1;

	if (thread_count == 0)
		thread_count = std::thread::hardware_concurrency();

	for (size_t i = 0; i < thread_count; i++)
		threads.emplace_back(ThreadPool::WorkerThread, this);
}

void ThreadPool::Wait() const
{
	while (!tasks.empty())
		std::this_thread::yield();
}

ThreadPool::~ThreadPool()
{
	Wait();

	alive = 0;

	for (auto& thread : threads)
		thread.join();

	threads.clear();
}

void ThreadPool::WorkerThread(ThreadPool* master)
{
	while (master->alive == 1)
	{
		Task* task = master->GetTask();
		if (task != nullptr)
			task->run();
		else
			std::this_thread::yield();
	}
}

/// ParallelFor 的内部任务：对单个 (x, y) 调用回调
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

void ThreadPool::ParallerlFor(size_t width, size_t height, const std::function<void(size_t, size_t)>& lambda)
{
	for (size_t y = 0; y < height; y++)
	{
		for (size_t x = 0; x < width; x++)
		{
			//创建任务对象
			AddTask(new ParallelForTask(x, y, lambda));
		}
	}
}

void ThreadPool::AddTask(Task* task)
{
	Guard guard{ spin_lock };
	tasks.push_back(task);
}

Task* ThreadPool::GetTask()
{
	Guard guard{ spin_lock };

	if (tasks.empty())
		return nullptr;

	Task* task = tasks.front();
	tasks.pop_front();
	return task;
}
