#include <stdio.h>
#include <pthread.h> // 必须包含这个头文件
#include <unistd.h>  // 包含 sleep

// --- 线程 1：模拟传感器采集 (切菜工) ---
// 线程函数的固定格式：返回 void*，参数 void*
void *thread_sensor(void *arg) {
    while (1) {
        printf(">>> [传感器] 正在采集数据... (1秒一次)\n");
        sleep(1); // 模拟采集只需要 1 秒
    }
    return NULL;
}

// --- 线程 2：模拟写硬盘 (厨师) ---
void *thread_storage(void *arg) {
    while (1) {
        printf("<<< [存储] 正在写入磁盘... (2秒一次)\n");
        sleep(2); // 模拟写硬盘很慢，需要 2 秒
    }
    return NULL;
}

int main() {
    pthread_t t1, t2; // 定义两个线程 ID 变量

    // 1. 创建线程 (让它们开始跑)
    // 参数：&线程ID, 属性(NULL), 函数名, 函数参数(NULL)
    if (pthread_create(&t1, NULL, thread_sensor, NULL) != 0) {
        perror("创建传感器线程失败");
    }

    if (pthread_create(&t2, NULL, thread_storage, NULL) != 0) {
        perror("创建存储线程失败");
    }

    // 2. 主线程不能死 (主线程死了，小弟们都会被强制关闭)
    // 这里我们用 pthread_join 等待它们结束 (虽然它们是死循环)
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    return 0;
}