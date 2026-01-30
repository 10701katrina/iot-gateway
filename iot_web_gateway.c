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
int enable_auto_mode = 1; // 🔥 新增：1=自动托管, 0=手动接管
int is_night_mode = 0; // 0:白天模式(灯关), 1:夜间模式(灯开)

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

    // Socket 创建常规流程...
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) exit(EXIT_FAILURE);
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(WEB_PORT);
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) exit(EXIT_FAILURE);
    if (listen(server_fd, 3) < 0) exit(EXIT_FAILURE);

    printf("🌐 [Web] 服务已启动: Port %d\n", WEB_PORT);

    while(1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) continue;

        char buffer[1024] = {0};
        read(new_socket, buffer, 1024);
        char http_response[8192]; // 加大缓冲区以容纳新 HTML

        // 🔥 1. 手动开关灯 -> 强制切换为手动模式
        if (strstr(buffer, "POST /toggle ") != NULL) {
            printf("🕹️ [User] 用户手动操作 -> 🚫 AI 已暂停\n");
            enable_auto_mode = 0; // 关掉自动模式
            if (serial_fd != -1) write(serial_fd, "$CMD,LED#", 9);
            is_night_mode = !is_night_mode;// 🔥🔥🔥 新增这一行：同步状态！🔥🔥🔥
            sprintf(http_response, "HTTP/1.1 200 OK\r\n\r\nOK");
        } 
        // 🔥 2. 新增接口：恢复自动模式
        else if (strstr(buffer, "POST /auto_on ") != NULL) {
            printf("🤖 [User] 用户激活托管 -> ✅ AI 已运行\n");
            enable_auto_mode = 1; // 开启自动模式
            sprintf(http_response, "HTTP/1.1 200 OK\r\n\r\nOK");
        }
        // 3. 数据接口 (增加返回当前模式状态)
        else if (strstr(buffer, "GET /data ") != NULL) {
            sprintf(http_response, 
                "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n%d,%d,%d,%d", 
                global_temp, global_humi, global_light, enable_auto_mode);
        } 
        // 4. 网页主页 (更新 HTML 界面)
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
                ".active { opacity: 1 !important; box-shadow: 0 0 15px currentColor; }"
                "</style></head>"
                "<body>"
                "<h1>🚀 STM32 智能边缘网关</h1>"
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

                        
                            // --- 🤖 边缘计算逻辑 ---

                            // 🔥 加上这个 if 判断！只有允许自动模式时，才执行下面的代码
                            if (enable_auto_mode == 1) { 
                                
                                // 场景 A: 天黑 开灯
                                if (l < 20 && is_night_mode == 0) {
                                    printf("🌙 [AI] 天黑 -> 自动开灯\n");
                                    if (serial_fd != -1) write(serial_fd, "$CMD,LED#", 9);
                                    is_night_mode = 1;
                                }
                                
                                // 场景 B: 天亮 关灯
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
        }
        usleep(10000); // 10ms 
    }
    
    close(serial_fd);
    return 0;
}
