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
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) exit(EXIT_FAILURE);
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(WEB_PORT);
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) exit(EXIT_FAILURE);
    if (listen(server_fd, 3) < 0) exit(EXIT_FAILURE);

    printf("🌐 [Web] 服务已启动: Port %d\n", WEB_PORT);

    while(1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) continue;
        //如果是 break，循环就结束了，服务器关门下班。
        //continue 意思是：“刚才那通电话没接好，算了，挂掉，直接跳回循环开头，准备接下一通。”
        //价值：保证服务器的健壮性。不能因为一个连接出错了，整个程序就崩溃退出，它要忽略错误，继续服务其他人。

        char buffer[1024] = {0};
        //存浏览器发过来的请求头
        read(new_socket, buffer, 1024);
        char http_response[8192]; // 装整个网页, 加大缓冲区以容纳新 HTML

        // 🔥 1. 手动开关灯 -> 强制切换为手动模式
        if (strstr(buffer, "POST /toggle ") != NULL) {
            //函数名：String String (在字符串里找字符串)。
            //在 buffer（浏览器发来的那一大坨字）里，寻找 "POST /toggle " 这个暗号。
                //找到了：返回由暗号开始的位置指针（非 NULL）。
                //没找到：返回 NULL。
            //这就是 Web 路由 (Routing) 的原理！服务器通过判断 URL 里的关键词，来决定执行哪段代码。

            printf("🕹️ [User] 用户手动操作 -> 🚫 AI 已暂停\n");
            enable_auto_mode = 0; // 关掉自动模式
            if (serial_fd != -1) write(serial_fd, "$CMD,LED#", 9);
            //serial_fd != -1：这是防御性编程。防止串口没打开（值为 -1）的时候你去写数据，会导致程序报错。
            //write：这是 Linux 系统调用。把字符通过 USB 线发出去。
            //"$CMD,LED#"：这是我们之前在 STM32 里约定的协议。
            //9：这个字符串刚好 9 个字节（包括标点符号）。

            is_night_mode = !is_night_mode;// 🔥🔥🔥 新增这一行：同步状态！🔥🔥🔥
            sprintf(http_response, "HTTP/1.1 200 OK\r\n\r\nOK");
            //sprintf：String Print Format。它不打印到屏幕，而是把字打印到数组（内存）里。
            //HTTP 协议格式：
                //HTTP/1.1 200 OK：状态行（告诉浏览器：成功了）。
                //\r\n\r\n：非常关键！ 连续两个回车换行。这是 HTTP 协议规定的“头”和“身体”的分界线。
                //OK：这是正文。对于 AJAX 请求，浏览器只要收到个 "OK" 就知道操作成功了，不需要返回整个网页。
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
                //全世界所有的浏览器和所有的服务器（Nginx, Apache, 你的代码）都必须严格遵守这个格式:
                //状态行 : HTTP/1.1 200 OK
                    //告诉浏览器：“我是 HTTP 1.1 版本，处理结果是 200 (成功)”。
                //响应头 : Content-Type: text/plain
                    //告诉浏览器：“我回复的数据类型是纯文本（text/plain）”，而不是 HTML、JSON 之类的。
                    //每行必须以 \r\n (回车换行) 结束。
                //空行 : \r\n
                    //这是最关键的！ 必须有一个单独的空行。浏览器读到这个空行，就知道：“哦，头信息结束了，下面是正文了”。
                //响应体 : %d,%d,%d,%d
                    //这是我们真正要回复给浏览器的数据。格式是：温度,湿度,光照,模式状态
                    //模式状态：1=自动托管中, 0=手动控制中
                    //浏览器拿到这个字符串后，可以用 JavaScript 的 split(',') 方法把它切成四块，分别显示在网页上。
                   

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
    char buf[256]; //从硬件读上来的原始数据
    int len, i; //计数员
    int parser_state = 0; // 0:找$ 1:找#
    char data_buf[64]; //存清洗干净、掐头去尾的有效数据的。
    int data_idx = 0; //记录 目前装了几个字符了。防止装满了溢出。

    printf("🚀 系统全速运行中 (Week 5 Final Version)...\n");

    while(1) {
        if (serial_fd < 0) { sleep(1); continue; } 
        //不加 sleep， 循环会以每秒几百万次的速度疯狂空转（死循环）。while(1)
        //加上 sleep(1)，循环就变成了每秒钟执行几次，既能及时响应串口数据，又不会占用过多 CPU 资源。

        len = read(serial_fd, buf, sizeof(buf));
        if (len > 0) {
            for (i = 0; i < len; i++) {
                char c = buf[i];
                if (parser_state == 0) {
                    if (c == '$') { parser_state = 1; data_idx = 0; }
                    //帧头
                    //0： “我在找头”。我现在两眼一抹黑，正在垃圾堆里找 $ 符号。
                } else if (parser_state == 1) {
                    if (c == '#') {
                        //帧尾
                        //1： “我在找尾”。我已经找到 $ 了，现在正在把有效数据一个个捡到盘子里，直到看到 # 为止。
                        parser_state = 0;
                        data_buf[data_idx] = '\0';
                        // \0 是一个特殊的字符，ASCII 码是 0。在 C 语言里，它叫 Null Terminator （空终止符）


                        // 解析: ENV,temp,humi,light
                        if (strncmp(data_buf, "ENV,", 4) == 0) {
                            //全称：String N Compare （字符串前 N 位比较）。
                            //作用：它比较 data_buf 的前 4 个字符是不是 "ENV,"。
                            //返回值：如果相等，返回 0；如果不相等，返回非 0。
                            //为什么要用它？
                                //因为你的数据包可能是 ，也可能是 。"ENV,25,60...""$CMD,OK#"
                                //你需要先判断这一包数据是环境数据 （ENV） 还是 命令回执 （CMD），才能决定后面怎么处理。
                            //告诉后面处理数据的函数，这里就是结尾

                            int t, h, l;
                            sscanf(data_buf + 4, "%d,%d,%d", &t, &h, &l);
                            //全称：String Scan Formatted （从字符串格式化扫描）。
                            //作用：它是 printf 的逆过程。
                                //printf是把数字变成字符串打印出来。
                                //sscanf是把字符串拆解回数字。
                            //它从 data_buf + 4（也就是跳过 "ENV," 这四个字符）开始，按照 "%d,%d,%d" 的格式，依次把数字读到 t、h、l 这三个变量里。
                                //data_buf + 4：这是一个指针偏移技巧。
                                //“%d，%d，%d”：这是模板。告诉函数：“你要找 3 个整数，中间用逗号隔开”。
                                //&t， &h， &l：把读到的 3 个数字，分别存进 （温度）， （湿度）， （光照） 这三个变量的内存地址里。thl

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
                        //如果当前读到的字符 既不是帧头 ，也不是帧尾，那它肯定是有效数据，需要把它存起来。
                        //“只有当盘子里装的东西少于 63 个时，我才继续装。如果已经装了 63 个了，后面的我就直接丢弃，不存了，保命要紧！”
                            //为什么是 63 而不是 64？因为 C 语言字符串最后通常需要留一个位置给 \0 结束符，所以要预留一点空间。
                        //data_buf[data_idx] = c;  // 动作1：把字符 c 放到当前的格子里
                        //data_idx++; // 动作2：格子编号加 1，准备装下一个字符
                    }
                }
            }
        }
        usleep(10000); // 10ms 
    }
    
    close(serial_fd);
    return 0;
}
