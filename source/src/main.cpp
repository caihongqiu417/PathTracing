#include <iostream>
#include <glm/glm.hpp>
#include "thread_pool.h"
#include "film.h"

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
	//创建胶片
	Film film{ 1920,1080 };
	for (int i = 0;i < 1920;i++) {
		for (int j = 0;j < 1080;j++)
		{
			film.SetPixel(i, j,{0.5,0.7,0.2});
		}
	}
	//保存到文件
	film.Save("Test1.ppm");


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