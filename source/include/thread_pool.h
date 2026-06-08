#pragma once 

#include<vector>   
#include<thread>    
#include<list>     
#include<mutex>    

/*
* 线程池：预先创建一批线程，用于并行执行任务
*/

/** 任务基类（抽象类）*/
class Task{
public:
    virtual void run() = 0;  // 纯虚函数，具体任务需要继承并实现这个方法
};

class ThreadPool
{
public:
    /// @brief 创建线程池
    /// @param thread_count 指定线程数量（0表示自动检测CPU核心数）
    ThreadPool(size_t thread_count = 0);

    ~ThreadPool();  // 析构函数：等待线程结束并清理资源

    // 每个线程执行的工作函数（静态，作为线程入口）
    static void WorkerThread(ThreadPool* master);

    // 添加任务到队列
    void AddTask(Task* task);
    
    // 从队列取出一个任务（没有则返回nullptr）
    Task* GetTask();
    
private:
    bool alive;
    std::vector<std::thread> threads;  // 存储所有工作线程
    std::list<Task*> tasks;            // 任务队列（链表）
    std::mutex lock;                   // 互斥锁（保护tasks的线程安全）
};