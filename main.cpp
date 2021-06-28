#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "ThreadPool.h"
#include "ThreadPool.cpp"
void taskFunc(void *arg)
{
    int num=*(int *)arg;
    printf("tid is %ld,num is %d\n",pthread_self(),num);
    sleep(1);
}
int main(){
    ThreadPool<int>* pool=new ThreadPool<int>(3,10);
    for(int i=0;i<100;i++)
    {
        int* num= new int(i+100);
        pool->addTask(Task<int>(taskFunc,num));
    }
    sleep(30);
    return 0;
}