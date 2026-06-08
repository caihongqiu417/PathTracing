#include <iostream>
#include <glm/glm.hpp>
#include "thread_pool.h"

class SimpleTask : public Task
{
public:
	void run() override
	{
		std::cout << "Hello World" << std::endl;
	}
};
int main()
{
	//创建线程池
	ThreadPool thread_pool{};

	//加入一个任务
	thread_pool.AddTask(new SimpleTask());
	thread_pool.AddTask(new SimpleTask());
	thread_pool.AddTask(new SimpleTask());
	thread_pool.AddTask(new SimpleTask());
	thread_pool.AddTask(new SimpleTask());

	return 0;
}