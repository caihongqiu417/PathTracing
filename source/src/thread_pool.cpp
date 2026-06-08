#include "thread_pool.h"

ThreadPool::ThreadPool(size_t thread_count)
{
	alive = true;
	// 未指定线程数（参数为0）→ 自动获取CPU核心数
    if (thread_count == 0)
        thread_count = std::thread::hardware_concurrency();

    // 创建指定个数线程，这些线程会立即执行工作函数
    for (size_t i = 0; i < thread_count; i++)
        threads.push_back(std::thread(ThreadPool::WorkerThread, this));
}

ThreadPool::~ThreadPool()
{
	//循环检查任务队列是否为空
	while (!tasks.empty())
		std::this_thread::yield();	 // 让出CPU，等任务被取走

	alive = false;
	// 等待所有线程执行完毕
    for (auto& thread : threads)
        thread.join();

    // 清空线程
    threads.clear();
}

// 每个线程的工作函数：
//	从任务队列取出任务并执行(如果任务为空则让出)
void ThreadPool::WorkerThread(ThreadPool* master)
{
	while (master->alive)
	{
		Task* task = master->GetTask();
		if (task != nullptr)
		{
			task->run();
		}
		else
		{
			std::this_thread::yield();
		}
	}
}

// 添加任务到任务队列
void ThreadPool::AddTask(Task* task)
{
	std::lock_guard<std::mutex> guard(lock);  // 自动加锁，离开作用域自动解锁
	
	tasks.push_back(task);
}

// 从队列取出一个任务（由工作线程调用）
Task* ThreadPool::GetTask()
{
	std::lock_guard<std::mutex> guard(lock);

	if (tasks.empty())
		return nullptr;

	Task* task = tasks.front();
	tasks.pop_front();
	return task;
}