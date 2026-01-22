#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

int g_counter = 0; 

// 1. 定义一把“锁” (Mutex)
pthread_mutex_t lock;

void *thread_inc(void *arg) {
    for (int i = 0; i < 1000000; i++) {
        // 2. 进门加锁：如果别人锁了，我就在这死等
        pthread_mutex_lock(&lock);
        
        g_counter++; // 现在的操作是安全的（原子的）
        
        // 3. 出门解锁：让给下一个人
        pthread_mutex_unlock(&lock);
    }
    return NULL;
}

void *thread_dec(void *arg) {
    for (int i = 0; i < 1000000; i++) {
        // 同样要加锁
        pthread_mutex_lock(&lock);
        
        g_counter--; 
        
        pthread_mutex_unlock(&lock);
    }
    return NULL;
}

int main() {
    pthread_t t1, t2;

    // A. 初始化锁 (必须做！)
    pthread_mutex_init(&lock, NULL);

    pthread_create(&t1, NULL, thread_inc, NULL);
    pthread_create(&t2, NULL, thread_dec, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    
    // B. 用完销毁锁
    pthread_mutex_destroy(&lock);

    printf("最终结果: %d (加了锁，一定是 0)\n", g_counter);

    return 0;
}