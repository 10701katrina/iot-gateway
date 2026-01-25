#include <stdio.h>   // (Standard Input Output)。为了用 printf, perror
#include <string.h>  // (String Handling)。为了用 strlen (算数据长度)
#include <unistd.h>  // （Unix Standard）。包含 write, close
#include <fcntl.h>   // （File Control）。包含 open 和 O_* 宏定义

int main() {
    int fd; // 文件描述符 (File Descriptor)
    char *filename = "sensor_data.txt";
    // filename 这个变量，存的只是第一个字母 's' 的内存地址。
    //当把第一个字母的地址交出，顺着这个地址往后摸，就能把这一整串字符都读出来，直到摸到结束标记 \0 为止。
    
    // 模拟从传感器获取的数据
    char *data = "[2026-01-22 18:30] Temp: 28.5C, Humidity: 40%\n"; 
    //  \n 很重要，保证日志是按行存储的，方便以后查阅。

    // 1. 打开文件 (这是最关键的一步)
    // O_WRONLY: 只写模式
    // O_CREAT:  如果文件不存在，就创建它
    // O_APPEND: 追加模式（fd指针自动跳到文件末尾,每次写都在文件末尾，不覆盖旧数据）
    // 0644:     文件权限（所有者读写，其他人只读）
    fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
    // | (按位或)：把多个选项拼在一起: 既要只写，又要自动创建，还要追加。

    if (fd == -1) {
        perror("打开文件失败"); // 自动打印错误原因（如权限不足）
        //perror：它比 printf 高级。它会自动去查系统的“错误字典”（errno），并把错误原因打印出来。
        return 1;
    }

    // 2. 写入数据
    // write 返回值是实际写入的字节数
    int len = write(fd, data, strlen(data));
    //通常 len 应该等于 strlen(data)
    
    if (len > 0) {
        printf("成功写入日志: %s", data);
    } else {
        printf("写入失败！\n");
    }

    // 3. 关闭文件 (不做这一步会导致内存泄漏)
    // 在之前的问答中反复强调，如果不关，在长时间运行的网关程序中，会导致文件描述符耗尽（Too many open files），程序崩溃。
    close(fd);

    return 0;
}