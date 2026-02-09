#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h> // 引入线程库

// --- 配置区域 ---
#define SERIAL_PORT "/dev/ttyUSB0"
#define BAUD_RATE B115200
#define WEB_PORT 8080
#define RB_SIZE 1024 // 环形缓冲区大小 (2的幂次方)

// --- 环形缓冲区结构体 (V2.0 核心) ---
typedef struct {
    char buffer[RB_SIZE];  // 真正的水池
    int head;              // 写指针 (生产者往这里写)
    int tail;              // 读指针 (消费者从这里读)
    pthread_mutex_t lock;  // 互斥锁 (防止两个人同时抢一个格子)
    pthread_cond_t not_empty; // 信号灯 (池子空了就睡，有水了就叫醒)
} RingBuffer;

// --- 全局共享数据 ---
RingBuffer rb; // 实例化全局缓冲区
int global_temp = 0;
int global_humi = 0;
int global_light = 0;
int serial_fd = -1; // 全局串口句柄
int enable_auto_mode = 1; // 1=自动托管, 0=手动接管
int is_night_mode = 0; // 0:白天模式(灯关), 1:夜间模式(灯开)

// --- 环形缓冲区操作函数 ---

// 初始化缓冲区
void rb_init(RingBuffer *rb) {
    rb->head = 0;
    rb->tail = 0;
    pthread_mutex_init(&rb->lock, NULL);
    pthread_cond_init(&rb->not_empty, NULL);
}

// 【生产者调用】往里写一个字节
void rb_put(RingBuffer *rb, char c) {
    pthread_mutex_lock(&rb->lock); // 上锁，独占池子

    // 计算下一个写入位置 (如果是 1023，下一个就是 0，实现“环形”)
    int next = (rb->head + 1) % RB_SIZE;

    if (next != rb->tail) { // 只要没满，就写入
        rb->buffer[rb->head] = c;
        rb->head = next;
        // 唤醒消费者：喂！醒醒！有数据了！
        pthread_cond_signal(&rb->not_empty); 
    } else {
        // 缓冲区满了！这时候只能丢弃新数据 (这种情况极少发生，除非消费者死机了)
        // printf("⚠️ Buffer Full! Dropping data.\n"); 
    }

    pthread_mutex_unlock(&rb->lock); // 解锁
}

// 【消费者调用】取出一个字节
char rb_get(RingBuffer *rb) {
    pthread_mutex_lock(&rb->lock); // 上锁

    // 如果池子是空的 (head == tail)，我就睡觉等待
    while (rb->head == rb->tail) {
        pthread_cond_wait(&rb->not_empty, &rb->lock); 
    }

    // 被叫醒了，开始取数据
    char c = rb->buffer[rb->tail];
    rb->tail = (rb->tail + 1) % RB_SIZE; // 移动读指针

    pthread_mutex_unlock(&rb->lock); // 解锁
    return c;
}

// --- 串口初始化 ---
int open_serial(const char *device) {
    int fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd == -1) {
        perror("无法打开串口");
        return -1;
    }
    struct termios options;
    tcgetattr(fd, &options);
    cfsetispeed(&options, BAUD_RATE);
    cfsetospeed(&options, BAUD_RATE);
    options.c_cflag |= (CLOCAL | CREAD);
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tcsetattr(fd, TCSANOW, &options);
    return fd;
}

// --- 线程1：Web 服务器 (UI交互) ---
void *thread_web_server(void *arg) {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Socket 创建常规流程...
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) pthread_exit(NULL);
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(WEB_PORT);
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) pthread_exit(NULL);
    if (listen(server_fd, 3) < 0) pthread_exit(NULL);

    printf("🌐 [Web] 服务已启动: Port %d\n", WEB_PORT);

    while(1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) continue;

        char buffer[1024] = {0};
        read(new_socket, buffer, 1024);
        char http_response[8192]; 

        // 🔥 1. 手动开关灯
        if (strstr(buffer, "POST /toggle ") != NULL) {
            printf("🕹️ [User] 用户手动操作 -> 🚫 AI 已暂停\n");
            enable_auto_mode = 0; 
            if (serial_fd != -1) write(serial_fd, "$CMD,LED#", 9);
            is_night_mode = !is_night_mode;
            sprintf(http_response, "HTTP/1.1 200 OK\r\n\r\nOK");
        } 
        // 🔥 2. 恢复自动模式
        else if (strstr(buffer, "POST /auto_on ") != NULL) {
            printf("🤖 [User] 用户激活托管 -> ✅ AI 已运行\n");
            enable_auto_mode = 1; 
            sprintf(http_response, "HTTP/1.1 200 OK\r\n\r\nOK");
        }
        // 3. 数据接口
        else if (strstr(buffer, "GET /data ") != NULL) {
            sprintf(http_response, 
                "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n%d,%d,%d,%d", 
                global_temp, global_humi, global_light, enable_auto_mode);
        } 
        // 4. 网页主页
        else {
            sprintf(http_response, 
                "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\n\r\n"
                "<!DOCTYPE html><html><head><title>IoT AI Gateway</title>"
                "<style>"
                "body { background: #1a1a1a; color: white; font-family: sans-serif; text-align: center; margin-top: 50px; }"
                ".card { background: #333; display: inline-block; width: 180px; padding: 20px; margin: 10px; border-radius: 15px; }"
                ".value { font-size: 40px; font-weight: bold; margin: 10px 0; }"
                ".label { color: #aaa; }"
                ".btn { padding: 15px 30px; font-size: 18px; border-radius: 50px; border: none; cursor: pointer; margin: 20px; color: white; transition: 0.2s; }"
                "#btn-manual { background: linear-gradient(45deg, #ff6b6b, #ff8e53); }"
                "#btn-auto { background: linear-gradient(45deg, #4ecdc4, #556270); opacity: 0.5; }"
                ".status-badge { display: inline-block; padding: 5px 15px; border-radius: 20px; background: #555; margin-bottom: 20px; }"
                "</style></head>"
                "<body>"
                "<h1>🚀 STM32 智能边缘网关 V2.0</h1>"
                "<div class='status-badge'>当前模式: <span id='mode-text'>--</span></div><br>"
                "<div class='card'><div class='label'>TEMP</div><div id='t' class='value'>--</div>℃</div>"
                "<div class='card'><div class='label'>HUMI</div><div id='h' class='value'>--</div>%%</div>"
                "<div class='card'><div class='label'>LIGHT</div><div id='l' class='value'>--</div>Lx</div>"
                "<br>"
                "<button id='btn-manual' class='btn' onclick='manualToggle()'>🕹️ 手动开关 (Manual)</button>"
                "<button id='btn-auto' class='btn' onclick='enableAuto()'>🤖 恢复托管 (Auto)</button>"
                "<script>"
                "function manualToggle() { fetch('/toggle', {method:'POST'}); updateUI(0); }"
                "function enableAuto() { fetch('/auto_on', {method:'POST'}); updateUI(1); }"
                "function updateUI(mode) {"
                "  document.getElementById('mode-text').innerText = mode ? '🤖 AI 托管中' : '🕹️ 手动控制中';"
                "  document.getElementById('mode-text').style.color = mode ? '#4ecdc4' : '#ff6b6b';"
                "  document.getElementById('btn-auto').style.opacity = mode ? '1' : '0.5';"
                "}"
                "setInterval(() => {"
                "  fetch('/data').then(r=>r.text()).then(t => {"
                "    let d = t.split(',');"
                "    document.getElementById('t').innerText=d[0];"
                "    document.getElementById('h').innerText=d[1];"
                "    document.getElementById('l').innerText=d[2];"
                "    updateUI(parseInt(d[3]));"
                "  });"
                "}, 1000);"
                "</script></body></html>"
            );
        }
        write(new_socket, http_response, strlen(http_response));
        close(new_socket);
    }
    return NULL;
}

// --- 线程2：生产者 (只负责极速接收串口数据) ---
void *thread_uart_reader(void *arg) {
    char temp_buf[256]; 
    printf("🧵 [Producer] 生产者线程启动: 全力接收串口数据...\n");

    while (1) {
        if (serial_fd < 0) {
            sleep(1); 
            continue; 
        }

        // 这一步极快，读到底层缓冲区的原始数据
        int len = read(serial_fd, temp_buf, sizeof(temp_buf));
        if (len > 0) {
            // 马上把数据塞进 Ring Buffer，绝不耽误时间去解析
            for (int i = 0; i < len; i++) {
                rb_put(&rb, temp_buf[i]);
            }
        } else {
            usleep(1000); // 没数据稍微歇一下 (1ms)，防止 CPU 空转
        }
    }
    return NULL;
}

// --- 线程3：消费者 (只负责从容解析数据) ---
void *thread_parser(void *arg) {
    char data_buf[64];
    int data_idx = 0;
    int parser_state = 0; // 0:找$ 1:找#

    printf("🧵 [Consumer] 消费者线程启动: 等待处理数据...\n");

    while (1) {
        // 这一步会阻塞：如果 Ring Buffer 空了，线程会自动睡觉，不占 CPU
        char c = rb_get(&rb); 

        // --- 解析逻辑 ---
        if (parser_state == 0) {
            if (c == '$') { parser_state = 1; data_idx = 0; }
        } else if (parser_state == 1) {
            if (c == '#') {
                parser_state = 0;
                data_buf[data_idx] = '\0'; // 封口

                // 解析: ENV,temp,humi,light
                if (strncmp(data_buf, "ENV,", 4) == 0) {
                    int t, h, l;
                    sscanf(data_buf + 4, "%d,%d,%d", &t, &h, &l);
                    
                    // 更新全局变量
                    global_temp = t;
                    global_humi = h;
                    global_light = l;
                    printf("📥 [更新] T:%d H:%d L:%d\n", t, h, l);

                    // --- 🤖 边缘计算逻辑 ---
                    if (enable_auto_mode == 1) { 
                        if (l < 20 && is_night_mode == 0) {
                            printf("🌙 [AI] 天黑 -> 自动开灯\n");
                            if (serial_fd != -1) write(serial_fd, "$CMD,LED#", 9);
                            is_night_mode = 1;
                        } 
                        else if (l > 35 && is_night_mode == 1) {
                            printf("☀️ [AI] 天亮 -> 自动关灯\n");
                            if (serial_fd != -1) write(serial_fd, "$CMD,LED#", 9);
                            is_night_mode = 0;
                        }
                    }
                }
            } else {
                if (data_idx < 63) data_buf[data_idx++] = c;
            }
        }
    }
    return NULL;
}

// --- 主程序 ---
int main() {
    // 1. 初始化环形缓冲区
    rb_init(&rb);

    // 2. 打开串口
    serial_fd = open_serial(SERIAL_PORT);
    if (serial_fd < 0) {
        printf("⚠️ 警告: 串口打开失败\n");
    } else {
        printf("✅ [Serial] 串口连接成功: %s\n", SERIAL_PORT);
    }

    // 3. 创建并启动三个线程
    pthread_t t_web, t_reader, t_parser;

    // 启动 Web 线程
    if (pthread_create(&t_web, NULL, thread_web_server, NULL) != 0) {
        perror("Web线程创建失败");
    }

    // 启动生产者线程
    if (pthread_create(&t_reader, NULL, thread_uart_reader, NULL) != 0) {
        perror("生产者线程创建失败");
    }

    // 启动消费者线程
    if (pthread_create(&t_parser, NULL, thread_parser, NULL) != 0) {
        perror("消费者线程创建失败");
    }

    // 4. 主线程等待 (实际上永远不会结束)
    pthread_join(t_web, NULL);
    pthread_join(t_reader, NULL);
    pthread_join(t_parser, NULL);

    close(serial_fd);
    return 0;
}