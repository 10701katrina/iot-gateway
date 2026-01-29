#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define BUFFER_SIZE 5  // 环形缓冲区大小

int buffer[BUFFER_SIZE];
int in = 0; // 下一个写入位置
int out = 0; // 下一个读取位置
int count = 0; //快速判断“满”和“空”

pthread_mutex_t lock;
pthread_cond_t cond_not_full;
pthread_cond_t cond_not_empty;

void *producer(void *arg){
    int i;
    for (i = 1; i <=20 ; i++){ //今天只生产 20 个包子，卖完就收摊
        //在网关项目里，生产者（采集传感器）和消费者（写 Flash）通常都是死循环 while(1)
        //因为设备要 7x24 小时一直跑，永远不停。

        pthread_mutex_lock(&lock); 
        //核心原则：锁是用来保护“共享数据”那一瞬间的操作，而不是保护整个线程的。 
        //持有锁的时间越短越好，给别人留出机会。不放在for外面。

        // while 就是红绿灯检查点
        while (count == BUFFER_SIZE){ //// 如果是红灯
            printf("队列满，生产者等待...\n");
            pthread_cond_wait(&cond_not_full, &lock); //// 拉手刹，熄火，就在这死等！哪也不去！ 【释放锁】>>【睡觉】
            // ...直到变绿灯 【消费者干活后】  ，重新点火...  【抢回锁】
            // 醒来后再次抬头看一眼红绿灯，哦，变绿了，跳出循环 【再次检查条件】>>【跳出循环】
        }

        // 跳出循环后，车子才继续往前开
       buffer[in]= i ; //// 通过路口
       printf("+++ 生产：%d (位置：%d)\n",i,in); // 计步器+1
       

       in = (in + 1)% BUFFER_SIZE;
        //in 往前走一步0 -> 1 -> 2 -> 3 -> 4 -> 0 -> 1 ...
       count++;

       pthread_cond_signal(&cond_not_empty);

       pthread_mutex_unlock(&lock);
       usleep(100000); // 模拟生产时间

    }
    return NULL;
}

void *consumer(void *arg){
    int i;
    for (i = 1; i <= 20; i++){
        pthread_mutex_lock(&lock);

        while(count == 0){
            printf("队列空，消费者等待...\n");
            pthread_cond_wait(&cond_not_empty, &lock);
        }

        int data = buffer[out];
        printf("--- 消费：%d (位置：%d)\n",data,out);

        out = (out + 1)% BUFFER_SIZE;
        count--;

        pthread_cond_signal(&cond_not_full);

        pthread_mutex_unlock(&lock);
        usleep(200000); // 模拟消费时间
    }
    return NULL;
}

int main(){
    pthread_t p_tid, c_tid;

    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&cond_not_full, NULL);
    pthread_cond_init(&cond_not_empty, NULL);

    pthread_create(&p_tid, NULL, producer, NULL);
    pthread_create(&c_tid, NULL, consumer, NULL);

    pthread_join(p_tid,NULL);
    pthread_join(c_tid,NULL);

    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&cond_not_full);
    pthread_cond_destroy(&cond_not_empty);

    return 0;
}