#include<iostream>
#include<string.h>
#include<string>
#include<unistd.h>
#include "ThreadPool.h"
using namespace std;
template<typename T>
ThreadPool<T>::ThreadPool(int min, int max)
{
    do 
    {
    // 任务队列
        this->taskQ=new TaskQueue<T>;
        if(this->taskQ==nullptr){
            cout<<"new taskQs fail..."<<endl;
            break;
        }
        this->workThreadIDs=new pthread_t[max];
        if(this->workThreadIDs==nullptr){
            cout<<"new threadIDs fail..."<<endl;
            break;
        }
        memset(this->workThreadIDs, 0, sizeof(pthread_t) * max);
        this->min=min;
        this->max=max;
        this->busyNum=0;
        this->aliveNum=min;
        this->exitNum=0;


        if (pthread_mutex_init(&this->mutexpool, NULL) != 0 ||
            pthread_cond_init(&this->Empty, NULL) != 0)
        {
            cout<<"mutex or condition init fail..."<<endl;
            break;
        }
        this->shutdown=false;




        // 创建线程
        pthread_create(&managerID, NULL, manager, this);
        for (int i = 0; i < min; ++i)
        {
            pthread_create(&workThreadIDs[i], NULL, worker,this);
        }
    } while (0);
}

template<typename T>
ThreadPool<T>::~ThreadPool()
{
    // 关闭线程池
    this->shutdown = true;
    // 阻塞回收管理者线程
    pthread_join(this->managerID, NULL);
    // 唤醒阻塞的消费者线程
    for (int i = 0; i < this->aliveNum; ++i)
    {
        pthread_cond_signal(&this->Empty);
    }
    // 释放堆内存
    if (this->taskQ->getQueueSize())
    {
        delete(this->taskQ);
    }
    if (this->workThreadIDs)
    {
        delete[](this->workThreadIDs);
    }
    pthread_mutex_destroy(&this->mutexpool);
    pthread_cond_destroy(&this->Empty);

}

template<typename T>
void ThreadPool<T>::addTask(Task<T> task)
{
    if (this->shutdown)
    {
        return;
    }
    // 添加任务
    this->taskQ->addTask(task);

    pthread_cond_signal(&this->Empty);

}

template<typename T>
int ThreadPool<T>::getBustNum()
{
    pthread_mutex_lock(&this->mutexpool);
    int busyNum = this->busyNum;
    pthread_mutex_unlock(&this->mutexpool);
    return busyNum;
}

template<typename T>
int ThreadPool<T>::getAliveNum()
{
    pthread_mutex_lock(&this->mutexpool);
    int aliveNum =this->aliveNum;
    pthread_mutex_unlock(&this->mutexpool);
    return aliveNum;
}

template<typename T>
void* ThreadPool<T>::worker(void* arg)
{
    ThreadPool* pool = static_cast<ThreadPool*>(arg);
    while (1)
    {
        pthread_mutex_lock(&pool->mutexpool);
        // 当前任务队列是否为空
        while (pool->taskQ->getQueueSize() == 0 && !pool->shutdown)
        {
            // 阻塞工作线程
            pthread_cond_wait(&pool->Empty, &pool->mutexpool);

            // 判断是不是要销毁线程
            if (pool->exitNum > 0)
            {
                pool->exitNum--;
                if (pool->aliveNum > pool->min)
                {
                    pool->aliveNum--;
                    pthread_mutex_unlock(&pool->mutexpool);
                    pool->threadExit();
                }
            }
        }

        // 判断线程池是否被关闭了
        if (pool->shutdown)
        {
            pthread_mutex_unlock(&pool->mutexpool);
            pool->threadExit();
        }

        // 从任务队列中取出一个任务
        Task<T> task=pool->taskQ->getTask();

        // 解锁
        pool->busyNum++;
        pthread_mutex_unlock(&pool->mutexpool);

        cout<<"thread"<<to_string(pthread_self())<<"%ld start working...\n";

        task.func(task.arg);
        delete(task.arg);
        task.arg = nullptr;

        cout<<"thread"<<to_string(pthread_self())<<"%ld end working...\n";
        pthread_mutex_lock(&pool->mutexpool);
        pool->busyNum--;
        pthread_mutex_unlock(&pool->mutexpool);
    }
    return NULL;
}

template<typename T>
void* ThreadPool<T>::manager(void* arg)
{
    ThreadPool* pool = static_cast<ThreadPool*>(arg);
    while (!pool->shutdown)
    {
        // 每隔3s检测一次
        sleep(3);
        // 取出线程池中任务的数量和当前线程的数量
        pthread_mutex_lock(&pool->mutexpool);
        int queueSize = pool->taskQ->getQueueSize();
        int aliveNum = pool->aliveNum;
        // 取出忙的线程的数量
        int busyNum = pool->busyNum;
        pthread_mutex_unlock(&pool->mutexpool);

        // 添加线程
        // 任务的个数>存活的线程个数 && 存活的线程数<最大线程数
        if (queueSize > aliveNum && aliveNum < pool->max)
        {
            pthread_mutex_lock(&pool->mutexpool);
            int counter = 0;
            for (int i = 0; i < pool->max && counter < pool->NUMBER
                && pool->aliveNum < pool->max; ++i)
            {
                if (pool->workThreadIDs[i] == 0)
                {
                    pthread_create(&pool->workThreadIDs[i], NULL, worker, pool);
                    counter++;
                    pool->aliveNum++;
                }
            }
            pthread_mutex_unlock(&pool->mutexpool);
        }
        // 销毁线程
        // 忙的线程*2 < 存活的线程数 && 存活的线程>最小线程数
        if (busyNum * 2 < aliveNum && aliveNum > pool->min)
        {
            pthread_mutex_lock(&pool->mutexpool);
            pool->exitNum = pool->NUMBER;
            pthread_mutex_unlock(&pool->mutexpool);
            // 让工作的线程自杀
            for (int i = 0; i < pool->NUMBER; ++i)
            {
                pthread_cond_signal(&pool->Empty);
            }
        }
    }
    return NULL;
}

template<typename T>
void ThreadPool<T>::threadExit()
{
    pthread_t tid = pthread_self();
    for (int i = 0; i < this->max; ++i)
    {
        if (this->workThreadIDs[i] == tid)
        {
            this->workThreadIDs[i] = 0;
            cout<<"threadExit()"<<to_string(tid)<<"called, %ld exiting...\n";
            break;
        }
    }
    pthread_exit(NULL);
}

