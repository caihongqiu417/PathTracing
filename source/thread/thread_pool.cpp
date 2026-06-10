#include "thread_pool.h"

#include <algorithm>
#include <atomic>
#include <cmath>

ThreadPool thread_pool{};

// 一个块任务：负责 [begin_x, end_x) x [begin_y, end_y) 这片区域
class ParallelForTask : public Task
{
public:
	ParallelForTask(
		size_t begin_x,
		size_t begin_y,
		size_t end_x,
		size_t end_y,
		const std::function<void(size_t, size_t)>& lambda
	)
		: begin_x(begin_x), begin_y(begin_y), end_x(end_x), end_y(end_y), lambda(lambda)
	{
	}

	void run() override
	{
		for (size_t y = begin_y; y < end_y; ++y)
		{
			for (size_t x = begin_x; x < end_x; ++x)
				lambda(x, y);
		}
	}

private:
	size_t begin_x, begin_y;
	size_t end_x, end_y;
	std::function<void(size_t, size_t)> lambda;
};

// 清掉队列里还没执行的任务
static void DeleteQueuedTasks(std::queue<Task*>& tasks)
{
	while (!tasks.empty())
	{
		delete tasks.front();
		tasks.pop();
	}
}

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

	// 正常情况队列已经空了，这里再清一次，防止漏删。
	{
		Guard guard{ spin_lock };
		DeleteQueuedTasks(tasks);
	}

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
			// 任务跑完就释放，所有权在 Worker 这里结束。
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
	// 按块切图，一个任务管一片区域，别碎到“一像素一任务”。
	// 线程越多，块越小，任务越多，更容易把活分匀。
	const float thread_scale = std::sqrt(static_cast<float>(threads.size()));
	float chunk_width_float = static_cast<float>(width) / std::sqrt(16.f) / thread_scale;
	float chunk_height_float = static_cast<float>(height) / std::sqrt(16.f) / thread_scale;
	size_t chunk_width = static_cast<size_t>(std::ceil(chunk_width_float));
	size_t chunk_height = static_cast<size_t>(std::ceil(chunk_height_float));

	// 块至少 1 像素，避免除出来是 0。
	chunk_width = (std::max)(chunk_width, size_t{ 1 });
	chunk_height = (std::max)(chunk_height, size_t{ 1 });

	for (size_t y = 0; y < height; y += chunk_height)
	{
		for (size_t x = 0; x < width; x += chunk_width)
		{
			const size_t end_x = (std::min)(x + chunk_width, width);
			const size_t end_y = (std::min)(y + chunk_height, height);

			// new 出来的任务交给线程池，最后由 Worker 线程 delete。
			AddTask(new ParallelForTask(x, y, end_x, end_y, lambda));
		}
	}
}

// 任务队列
void ThreadPool::AddTask(Task* task)
{
	Guard guard{ spin_lock };
	// 入队后由线程池接管，别在外面 delete。
	tasks.push(task);
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
