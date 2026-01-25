#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

int g_data = 0; // 共享数据
pthread_mutex_t lock; // 互斥锁  定义联合体或结构体变量，内部有成员
pthread_cond_t cond_var; // 条件变量

void *consumer(void *arg){

    while (1)
    {
        pthread_mutex_lock(&lock);
        while (g_data==0){
            printf("[消费者]没数据，我先睡了...\n");
            pthread_cond_wait(&cond_var,&lock);
        }
        printf("[消费者]收到通知，取出数据:%d\n",g_data);
        g_data=0;
        pthread_mutex_unlock(&lock);
    }
    return NULL;
}

void *producer(void *arg){
    for (int i=1;i<=5;i++){
        sleep(2);

        pthread_mutex_lock(&lock);

        g_data=i;
        printf("[生产者]采集到数据:%d，发送通知！\n",g_data);

        pthread_cond_signal(&cond_var);
        pthread_mutex_unlock(&lock);
    }
    return NULL;   
}

int main(){

    pthread_t t1,t2;
    pthread_mutex_init(&lock,NULL);
    pthread_cond_init(&cond_var,NULL);

    pthread_create(&t1,NULL,consumer,NULL);
    pthread_create(&t2,NULL,producer,NULL);

    pthread_join(t2,NULL);

    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&cond_var);    




}