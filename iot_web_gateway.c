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
#include <pthread.h>

// --- 配置区域 ---
#define SERIAL_PORT "/dev/ttyUSB0"
#define BAUD_RATE B115200
#define WEB_PORT 8080

// --- 全局共享数据 ---
int global_temp = 0;
int global_humi = 0;
int global_light = 0;
int serial_fd = -1; // 🔥 关键：全局串口句柄

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

// --- Web 服务器线程 (包含反向控制) ---
void *web_server_thread(void *arg) {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // 1. 创建 Socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Web Socket failed");
        exit(EXIT_FAILURE);
    }
    // 端口复用
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(WEB_PORT);

    // 2. 绑定 & 监听
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Web Bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
        perror("Web Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("🌐 [Web] 服务已启动: Port %d (支持反向控制)\n", WEB_PORT);

    // 3. 循环等待连接
    while(1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            continue;
        }

        char buffer[1024];
        int read_len = read(new_socket, buffer, 1024);
        if (read_len > 0) buffer[read_len] = '\0';

        char http_response[4096];

        // 🔥 判断 1: 控制指令 (POST /toggle)
        if (strstr(buffer, "POST /toggle ") != NULL) {
            printf("🕹️ [Web] 收到开关灯指令 -> 发送给 STM32\n");
            
            // 往串口写指令：$CMD,LED#
            if (serial_fd != -1) {
                write(serial_fd, "$CMD,LED#", 9); 
            } else {
                printf("⚠️ 串口未连接，指令发送失败\n");
            }

            sprintf(http_response, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nOK");
        } 
        // 判断 2: 获取数据接口
        else if (strstr(buffer, "GET /data ") != NULL) {
            sprintf(http_response, 
                "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n%d,%d,%d", 
                global_temp, global_humi, global_light);
        } 
        // 判断 3: 主页 (HTML)
        else {
            sprintf(http_response, 
                "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\n\r\n"
                "<!DOCTYPE html><html><head><title>IoT Monitor</title>"
                "<style>"
                "body { background: #1a1a1a; color: white; font-family: sans-serif; text-align: center; margin-top: 50px; }"
                ".card { background: #333; display: inline-block; width: 200px; padding: 20px; margin: 10px; border-radius: 15px; box-shadow: 0 4px 15px rgba(0,0,0,0.5); }"
                ".value { font-size: 50px; font-weight: bold; }"
                ".unit { font-size: 20px; color: #aaa; }"
                ".label { font-size: 18px; color: #bbb; margin-bottom: 10px; display: block; }"
                "#temp { color: #ff6b6b; } #humi { color: #4ecdc4; } #light { color: #ffe66d; }"
                ".btn { padding: 15px 30px; font-size: 20px; border-radius: 50px; border: none; background: linear-gradient(45deg, #ff6b6b, #ff8e53); color: white; cursor: pointer; margin-top: 30px; box-shadow: 0 4px 15px rgba(255, 107, 107, 0.4); transition: transform 0.1s; }"
                ".btn:active { transform: scale(0.95); }"
                "</style></head>"
                "<body>"
                "<h1>🚀 STM32 IoT 实时监控大屏</h1>"
                "<div class='card'><span class='label'>温度 (Temp)</span><div id='temp' class='value'>--</div><span class='unit'>℃</span></div>"
                "<div class='card'><span class='label'>湿度 (Humi)</span><div id='humi' class='value'>--</div><span class='unit'>%%</span></div>"
                "<div class='card'><span class='label'>光照 (Light)</span><div id='light' class='value'>--</div><span class='unit'>Lx</span></div>"
                "<br>"
                "<button class='btn' onclick='toggleLed()'>💡 远程开关灯</button>"
                "<script>"
                "function updateData() {"
                "  fetch('/data').then(res => res.text()).then(txt => {"
                "    let parts = txt.split(',');"
                "    document.getElementById('temp').innerText = parts[0];"
                "    document.getElementById('humi').innerText = parts[1];"
                "    document.getElementById('light').innerText = parts[2];"
                "  }).catch(e=>{});"
                "}"
                "function toggleLed() {"
                "  fetch('/toggle', {method: 'POST'}).then(console.log('Command Sent'));"
                "}"
                "setInterval(updateData, 1000);"
                "</script>"
                "</body></html>"
            );
        }

        write(new_socket, http_response, strlen(http_response));
        close(new_socket);
    }
}

// --- 主程序 ---
int main() {
    // 1. 打开串口 (赋值给全局变量)
    serial_fd = open_serial(SERIAL_PORT);
    if (serial_fd < 0) {
        printf("⚠️ 警告: 串口打开失败，将使用模拟模式\n");
    } else {
        printf("✅ [Serial] 串口连接成功: %s\n", SERIAL_PORT);
    }

    // 2. 启动 Web 线程
    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, web_server_thread, NULL) != 0) {
        perror("无法创建 Web 线程");
        return 1;
    }

    // 3. 主循环：处理串口接收
    char buf[256];
    int len, i;
    int parser_state = 0; // 0:找$ 1:找#
    char data_buf[64];
    int data_idx = 0;

    printf("🚀 系统全速运行中 (Week 5 Final Version)...\n");

    while(1) {
        if (serial_fd < 0) { sleep(1); continue; } 

        len = read(serial_fd, buf, sizeof(buf));
        if (len > 0) {
            for (i = 0; i < len; i++) {
                char c = buf[i];
                if (parser_state == 0) {
                    if (c == '$') { parser_state = 1; data_idx = 0; }
                } else if (parser_state == 1) {
                    if (c == '#') {
                        parser_state = 0;
                        data_buf[data_idx] = '\0';
                        // 解析: ENV,temp,humi,light
                        if (strncmp(data_buf, "ENV,", 4) == 0) {
                            int t, h, l;
                            sscanf(data_buf + 4, "%d,%d,%d", &t, &h, &l);
                            global_temp = t;
                            global_humi = h;
                            global_light = l;
                            printf("📥 [更新] T:%d H:%d L:%d\n", t, h, l);
                        }
                    } else {
                        if (data_idx < 63) data_buf[data_idx++] = c;
                    }
                }
            }
        }
        usleep(10000); // 10ms 
    }
    
    close(serial_fd);
    return 0;
}
