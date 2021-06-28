#pragma once
#include <queue>
#include <pthread.h>
#include "TaskQueue.h"
#include "TaskQueue.cpp"
using namespace std;
template<typename T>
class ThreadPool
{
private:
    pthread_t managerID;//管理者线程ID
    pthread_t* workThreadIDs;//消费者线程的IDs
    TaskQueue<T>* taskQ;//工作队列
    int min;//最小线程数
    int max;//最大线程数
    int busyNum;//忙线程数量
    int aliveNum;//活着的线程数量
    int exitNum;//退出的线程数
    static const int NUMBER=2;//一次退出数量

    pthread_mutex_t mutexpool;//线程池锁
    pthread_cond_t Empty;//空条件变量
   // pthread_cond_t Full;//满条件变量

    bool shutdown; //销毁线程池变量,1为销毁，0为不销毁

    static void* manager(void*);
    static void* worker(void*);
    void threadExit();
    
public:
    //创建线程池并初始化
    ThreadPool(int min,int max);
    //销毁线程池
    ~ThreadPool();
    //向线程池中添加任务
    void addTask(Task<T> task);
    //获得忙线程个数
    int getBustNum();
    //获得活着线程的个数
    int getAliveNum();
};