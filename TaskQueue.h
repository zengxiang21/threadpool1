#pragma once
#include <queue>
#include <pthread.h>
using namespace std;
using callback=void (*)(void*);
template<typename T>
struct Task{
    callback func;
    T* arg;
    Task(){
        func=nullptr;
        arg=nullptr;
    };
    Task(callback func,void* arg){
        this->func=func;
        this->arg=(T*)arg;
    };
};

template<typename T>
class TaskQueue{
public:
    TaskQueue();
    ~TaskQueue();
    //添加一个任务
    void addTask(Task<T> task);
    void addTask(callback func,void* arg);
    //取出一个任务
    Task<T> getTask();
    //队列个数
    inline int getQueueSize(){
        return m_taskQ.size();
    }
private:
    pthread_mutex_t m_mutex;
    queue<Task<T>> m_taskQ;

};