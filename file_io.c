#include <stdio.h>
#include <string.h>
#include <unistd.h>  // 包含 write, close
#include <fcntl.h>   // 包含 open 和 O_* 宏定义

int main() {
    int fd;
    char *filename = "sensor_data.txt";
    
    // 模拟从传感器获取的数据
    char *data = "[2026-01-22 18:30] Temp: 28.5C, Humidity: 40%\n";

    // 1. 打开文件 (这是最关键的一步)
    // O_WRONLY: 只写模式
    // O_CREAT:  如果文件不存在，就创建它
    // O_APPEND: 追加模式（每次写都在文件末尾，不覆盖旧数据）
    // 0644:     文件权限（所有者读写，其他人只读）
    fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644);

    if (fd == -1) {
        perror("打开文件失败"); // 自动打印错误原因（如权限不足）
        return 1;
    }

    // 2. 写入数据
    // write 返回值是实际写入的字节数
    int len = write(fd, data, strlen(data));
    
    if (len > 0) {
        printf("成功写入日志: %s", data);
    } else {
        printf("写入失败！\n");
    }

    // 3. 关闭文件 (不做这一步会导致内存泄漏)
    close(fd);

    return 0;
}