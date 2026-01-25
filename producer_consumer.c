#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

// 共享资源
int g_data = 0;             // 0 表示没数据，>0 表示有数据
pthread_mutex_t lock;       // 锁：保护 g_data
pthread_cond_t  cond_var;   // 条件变量：用于通知/等待

// --- 消费者线程 (存盘) ---
void *consumer(void *arg) {
    while (1) {
        pthread_mutex_lock(&lock); // 1. 加锁

        // 2. 检查有没有数据
        // 如果没数据(g_data==0)，就进入等待模式
        // 注意：pthread_cond_wait 会自动释放锁，并让线程睡过去
        while (g_data == 0) {
            printf("    [消费者] 没数据，睡觉等待...\n");
            pthread_cond_wait(&cond_var, &lock);
        }

        // 3. 醒来后(也就是收到了信号)，处理数据
        printf("    [消费者] 收到通知！取出数据: %d\n", g_data);
        g_data = 0; // 清空数据，表示处理完了

        pthread_mutex_unlock(&lock); // 4. 解锁
    }
    return NULL;
}

// --- 生产者线程 (采集) ---
void *producer(void *arg) {
    for (int i = 1; i <= 5; i++) {
        sleep(2); // 模拟采集耗时
        
        pthread_mutex_lock(&lock); // 1. 加锁
        
        g_data = i; // 生产数据
        printf("[生产者] 采集到数据: %d，发送通知！\n", g_data);
        
        // 2. 敲锣打鼓，唤醒消费者(把线程的状态从 “阻塞 (Blocked)” 改为 “就绪 (Ready)”。)
        // pthread_cond_signal 负责把等待在 cond_var 上的线程叫醒
        pthread_cond_signal(&cond_var);//此时锁还在生产者手里!!!
        //注意：如果你的系统里有好几个条件变量（比如 cond_temp 温度报警, cond_motion 运动报警），你传哪个地址，就只叫醒等那个条件的人。
        pthread_mutex_unlock(&lock); // 3. 解锁,生产者彻底放手
    }
    return NULL;
}

int main() {
    pthread_t t1, t2;//定义了两个用来装线程 ID 的变量

    // 初始化锁和条件变量
    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&cond_var, NULL);

    pthread_create(&t1, NULL, consumer, NULL); // 先启动消费者，让他去等着
    pthread_create(&t2, NULL, producer, NULL);

    pthread_join(t2, NULL); // 等待生产者结束，主程序参与join
    //第二个传参：void **__thread_return (作业本地址)【双重指针】
    //它的作用是：让子线程在结束时（通过 return 或 pthread_exit），能把一个结果传回来给主线程。如果不需要结果，这里直接传 NULL。 
    
    // 这里的逻辑稍微简化了，实际项目中需要让消费者也优雅退出
    // 为了演示简单，我们就不等待消费者死循环了
    
    //

    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&cond_var);



    return 0;
}