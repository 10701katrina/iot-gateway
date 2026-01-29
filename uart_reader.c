#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>      // 文件控制定义
#include <termios.h>    // POSIX 终端控制定义 (关键!)
#include <errno.h>

// 你的设备路径 (刚才 ls 出来的那个)
#define SERIAL_PORT "/dev/ttyUSB0" 

int main() {
    int serial_fd;
    struct termios tty;

    // 1. 打开串口文件
    // O_RDWR: 读写模式
    // O_NOCTTY: 不把这个串口作为控制终端 (防止 Ctrl+C 杀掉程序)
    // O_NDELAY: 非阻塞模式 (暂时不想卡死在这里)
    serial_fd = open(SERIAL_PORT, O_RDWR | O_NOCTTY);
    if (serial_fd < 0) {
        perror("Error opening serial port");
        return 1;
    }
    printf("✅ 成功打开串口: %s (FD: %d)\n", SERIAL_PORT, serial_fd);

    // 2. 配置串口 (Termios 魔法)
    // 先获取当前配置
    if (tcgetattr(serial_fd, &tty) != 0) {
        perror("Error from tcgetattr");
        return 1;
    }

    // 设置波特率 (这里设为 115200，正点原子例程通常默认是这个)
    cfsetospeed(&tty, B115200);
    cfsetispeed(&tty, B115200);

    // 设置数据位 = 8位
    tty.c_cflag &= ~CSIZE; 
    tty.c_cflag |= CS8;
    
    // 禁用奇偶校验 (No Parity)
    tty.c_cflag &= ~PARENB;
    
    // 停止位 = 1位 (One Stop Bit)
    tty.c_cflag &= ~CSTOPB;

    // 禁用硬件流控 (无 RTS/CTS)
    tty.c_cflag &= ~CRTSCTS;

    // 启用接收器，并忽略调制解调器控制线
    tty.c_cflag |= (CLOCAL | CREAD);

    // 3. 应用配置
    // TCSANOW: 立即生效
    if (tcsetattr(serial_fd, TCSANOW, &tty) != 0) {
        perror("Error from tcsetattr");
        return 1;
    }
    printf("⚙️  波特率配置为 115200, 8N1\n");

    // 4. 读取循环
    char read_buf[256];
    printf("📡 等待接收数据... (按 Ctrl+C 退出)\n");

    while (1) {
        //以此为基础，读取数据
        memset(read_buf, 0, sizeof(read_buf));
        
        // read 会在这里尝试读取
        // 因为前面设了 O_NDELAY，如果没有数据它会返回 0 或 -1
        // 为了演示方便，我们这里还是用 read，但在实际项目中通常配合 Epoll
        int num_bytes = read(serial_fd, read_buf, sizeof(read_buf));

        if (num_bytes > 0) {
            printf("收到 [%d 字节]: %s\n", num_bytes, read_buf);
        }
        
        // 稍微睡一下，防止 CPU 100% 空转
        usleep(10000); // 10ms
    }

    close(serial_fd);
    return 0;
}