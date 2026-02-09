#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>     //// 包含 read, write, close, sleep 等系统调用
#include <fcntl.h>      // 文件控制定义
#include <termios.h>    // POSIX 终端控制定义 (关键!)
#include <errno.h>

// 你的设备路径 (刚才 ls 出来的那个)
#define SERIAL_PORT "/dev/ttyUSB0" 

int main() {
    int serial_fd;
    struct termios tty; 
    //termios: Terminal I/O Structure  终端输入输出结构体

    // 1. 打开串口文件
    // O_RDWR:(Read/Write) 读写模式->需要读传感器数据，同时也可能需要发控制指令
    // O_NOCTTY: (No Controlling TTY)不把这个串口作为控制终端 (防止 Ctrl+C 杀掉程序).标志就是告诉 Linux：“这只是个数据通道，不要把它当成控制台。”
    // O_NDELAY: (Non-Delay / Non-Blocking)非阻塞模式 (暂时不想卡死在这里).告诉 read() 函数，如果串口里没有数据，不要死等（卡死程序），而是立刻返回 0 或 -1，让你能继续干别的事。
    serial_fd = open(SERIAL_PORT, O_RDWR | O_NOCTTY);
    if (serial_fd < 0) {
        perror("Error opening serial port");
        return 1;
    }
    printf("✅ 成功打开串口: %s (FD: %d)\n", SERIAL_PORT, serial_fd);

    // 2. 配置串口 (Termios 魔法)
    // 先获取当前配置
    if (tcgetattr(serial_fd, &tty) != 0) {
        //Terminal Control Get Attributes
        //作用：去系统里把当前的串口配置读出来，存到你的 tty 变量里。
        perror("Error from tcgetattr");
        return 1;
    }

    // 设置波特率 (这里设为 115200，正点原子例程通常默认是这个)
    cfsetospeed(&tty, B115200);
    cfsetispeed(&tty, B115200);
    //不同的 CPU（比如 x86、ARM）或者不同的串口控制器，对于“115200 波特率”，底层寄存器里需要填写的二进制代码是完全不一样的。

    //位操作技巧：&= ~XXXX 意思是“清除这一位”；|= XXXX 意思是“开启这一位”。

    //❎清除某一位：&= ~ (按位与 + 取反) 
    //  1010 1110 (当前状态，第2位是1)
    //& 1111 1101 (我的模具，只有第2位是0) ~ 作用于 0000 0010,结果 = 1111 1101 👈 这就是我们要的模具！
    //----------
    //  1010 1100 (结果：其他位保持原样，唯独第2位被强制按成了0)

    //✅开启某一位：|= (按位或)
    //  1010 1100 (原始状态)
    //| 0000 0010 (我要开启的那一位，即 CREAD)
    //-----------
    //  1010 1110 (结果：其他位没动，第2位变成了1)

    // 设置数据位 = 8位
    tty.c_cflag &= ~CSIZE; 
    tty.c_cflag |= CS8;
    
    // 禁用奇偶校验 (No Parity)
    tty.c_cflag &= ~PARENB;
    
    // 停止位 = 1位 (One Stop Bit)
    tty.c_cflag &= ~CSTOPB;

    // 禁用硬件流控 (无 RTS/CTS)
    tty.c_cflag &= ~CRTSCTS;
    // 你的 USB 转串口线通常只有 TX, RX, GND 三根线，没有流控脚。
    // 如果不禁用流控，Linux 会一直等待对方的 "Ready" 信号，导致发不出数据。


    // 启用接收器，并忽略调制解调器控制线
    tty.c_cflag |= (CLOCAL | CREAD);
    // CLOCAL: 忽略调制解调器控制线 (我们是直连，不用拨号)
    // CREAD:  🔥 关键！开启接收功能。如果不写这一句，你永远读不到数据。

    // 3. 应用配置
    // TCSANOW: 立即生效
    if (tcsetattr(serial_fd, TCSANOW, &tty) != 0) {
        //Terminal Control Set Attributes
        //作用：把你配置好的 tty 结构体写回系统，让新的设置生效。
        //参数2的 TCSANOW 表示“立即生效”，还有 TCSADDR 和 TCSAFLUSH 等选项，分别表示“等所有数据发送完再生效”和“先清空输入输出缓冲区再生效”。
        // TCSANOW: Make changes now without waiting (立即生效)
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
        // 每次清空缓冲区，防止打印上次的残留数据
        // 参数1：要擦哪块内存 (数组名)
        // 参数2：擦成什么值 (0 表示清空)
        // 参数3：擦多大 (整个数组的大小)
        
        // read 会在这里尝试读取
        // 因为前面设了 O_NDELAY，如果没有数据它会返回 0 或 -1
        // 为了演示方便，我们这里还是用 read，但在实际项目中通常配合 Epoll
        int num_bytes = read(serial_fd, read_buf, sizeof(read_buf));

        if (num_bytes > 0) {
            // 读到了数据！
            printf("收到 [%d 字节]: %s\n", num_bytes, read_buf);
        }
        
        // 稍微睡一下，防止 CPU 100% 空转
        usleep(10000); // 10ms
        //Microsecond Sleep (微秒睡眠)
        //相当于告诉操作系统：“我干完活了，先休息一下，把 CPU 让给别人用。”
        //如果里面没有等待，CPU 会一秒钟空转几亿次，占用率直接飙到 100%
    }

    close(serial_fd);
    return 0;
}