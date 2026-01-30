#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>

#define PORT 8888
#define MAX_EVENTS 100
#define SERIAL_PORT "/dev/ttyUSB0"  // 你的板子路径
#define RB_SIZE 1024                // 缓冲区大小

// ==========================================
// 第一部分：Ring Buffer (环形缓冲区) 实现
// ==========================================

// 定义缓冲区结构体
typedef struct {
    char *buffer;    // 真正的内存空间
    int size;        // 总容量
    int head;        // 写入位置 (头)
    int tail;        // 读取位置 (尾)
    int is_full;     // 满了没？
} ring_buffer_t;

// 1. 初始化
void ring_buffer_init(ring_buffer_t *rb, char *mem, int size) {
    rb->buffer = mem;
    rb->size = size;
    rb->head = 0;
    rb->tail = 0;
    rb->is_full = 0;
}

// 2. 写入数据 (Epoll 线程调用这个)
void ring_buffer_write(ring_buffer_t *rb, char *data, int len) {
    for (int i = 0; i < len; i++) {
        rb->buffer[rb->head] = data[i];
        
        // 移动头指针
        rb->head = (rb->head + 1) % rb->size;

        // 如果满了，尾巴也要被迫向前移（覆盖旧数据）
        if (rb->is_full) {
            rb->tail = (rb->tail + 1) % rb->size;
        }

        // 判断是不是追上尾巴了
        if (rb->head == rb->tail) {
            rb->is_full = 1;
        }
    }
}

// 3. 简单打印当前缓冲区状态 (调试用)
void debug_print_rb(ring_buffer_t *rb) {
    printf("📊 [RingBuffer] Head:%d Tail:%d Full:%d\n", rb->head, rb->tail, rb->is_full);
}

// ==========================================
// 第二部分：串口与 Epoll 逻辑
// ==========================================

// 全局大水桶
char my_memory[RB_SIZE];
ring_buffer_t my_rb;

// 打开串口的函数
int open_serial(const char *device) {
    int fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd == -1) {
        perror("❌ 无法打开串口 (请检查USB线或权限)");
        return -1;
    }
    
    struct termios options;
    tcgetattr(fd, &options);
    
    // 配置波特率 115200
    cfsetispeed(&options, B115200);
    cfsetospeed(&options, B115200);
    
    // 配置 8N1 (8数据位, 无校验, 1停止位)
    options.c_cflag |= (CLOCAL | CREAD);
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;
    
    // 禁用流控 (防止无法发送)
    options.c_cflag &= ~CRTSCTS; 

    // 启用原始模式 (Raw Mode) - 这一点很重要，防止特殊字符被系统吃掉
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    options.c_oflag &= ~OPOST;

    if (tcsetattr(fd, TCSANOW, &options) != 0) {
        perror("配置串口失败");
        return -1;
    }
    
    return fd;
}

// 简单的日志写入函数
void write_log(char *msg) {
    // O_APPEND: 追加模式 (不覆盖旧内容)
    // O_CREAT: 如果文件不存在就创建
    // 0644: 文件权限 (rw-r--r--)
    int fd = open("alarm_log.txt", O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (fd > 0) {
        // 获取当前时间（可选，为了简单这里先不加）
        char log_buf[256];
        sprintf(log_buf, "⚠️ [ALARM] %s", msg); 
        write(fd, log_buf, strlen(log_buf));
        close(fd); // 写完立刻关闭，保存数据
    }
}

int main() {
    int server_fd, client_fd, epoll_fd, serial_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    struct epoll_event ev, events[MAX_EVENTS];

    printf("🚀 正在启动 IoT 边缘网关...\n");

    // 1. 初始化 RingBuffer
    ring_buffer_init(&my_rb, my_memory, RB_SIZE);

    // 2. 启动网络监听 (Server)
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }
    // 端口复用 (防止重启报错)
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }
    printf("✅ 网络服务已启动: Port %d\n", PORT);

    // 3. 打开串口 (Sensor)
    serial_fd = open_serial(SERIAL_PORT);
    if (serial_fd > 0) {
        printf("✅ 串口连接成功: %s\n", SERIAL_PORT);
    } else {
        printf("⚠️ 警告: 串口打开失败，程序将只运行网络功能。\n");
    }

    // 4. 创建 Epoll
    epoll_fd = epoll_create(1);

    // 添加 Server FD
    ev.events = EPOLLIN;
    ev.data.fd = server_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev);

    // 添加 Serial FD (如果打开成功的话)
    if (serial_fd > 0) {
        ev.events = EPOLLIN;
        ev.data.fd = serial_fd;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, serial_fd, &ev);
    }

    printf("✨ 系统就绪，等待数据...\n\n");

    // 5. 主循环
    while (1) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);

        for (int i = 0; i < nfds; i++) {
            int current_fd = events[i].data.fd;

            // --- A. 网络有新连接 ---
            if (current_fd == server_fd) {
                client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
                printf("👋 新客户端上线: FD %d\n", client_fd);
                
                // 把新客户端也加入 Epoll
                ev.events = EPOLLIN;
                ev.data.fd = client_fd;
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev);
            }
            // --- B. 串口有数据 (STM32 发来的) ---
            else if (current_fd == serial_fd) {
                char buf[256];
                int n = read(serial_fd, buf, sizeof(buf) - 1);
                if (n > 0) {
                    buf[n] = '\0';
                    
                    // 1. 存入 RingBuffer (现有逻辑)
                    ring_buffer_write(&my_rb, buf, n);
                    printf("📥 [串口] 收到: %s", buf);

                    // 2. 🔥 新增：简单的阈值判断与日志记录
                    // 假设数据格式里包含 "ex:59.3" (外部温度)
                    char *ptr = strstr(buf, "ex:"); 
                    if (ptr != NULL) {
                        float temp = atof(ptr + 3); // 跳过 "ex:" 这3个字符，解析后面的数字
                        
                        // 设定阈值，比如 30度
                        if (temp > 30.0) {
                            printf("🔥 温度过高 (%.1f)！正在写入日志...\n", temp);
                            write_log(buf); // 把这一整行原始数据写进文件
                        }
                    }
                }
            }
            
            // --- C. 客户端有数据 (网络发来的) ---
            else {
                char buf[1024];
                int n = read(current_fd, buf, sizeof(buf));
                if (n <= 0) {
                    // 客户端断开
                    close(current_fd);
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, current_fd, NULL);
                    printf("👋 客户端下线: FD %d\n", current_fd);
                } else {
                    buf[n] = '\0';
                    printf("🌐 [网络] 收到: %s\n", buf);
                    // 这里你也可以解析指令，比如 "OPEN_LED" 然后 write(serial_fd, ...)
                }
            }
        }
    }
    return 0;
}