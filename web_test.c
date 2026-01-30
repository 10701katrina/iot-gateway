#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080  // 我们将监听 8080 端口

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int opt = 1;

    // 1. 创建 Socket (还是那套标准动作)
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // 2. 设置端口复用 (防止重启时报 Address already in use)
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // 3. 绑定端口
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // 监听所有网卡 IP
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // 4. 开始监听
    if (listen(server_fd, 3) < 0) {
        perror("Listen");
        exit(EXIT_FAILURE);
    }

    printf("🌐 Web Server started on Port %d\n", PORT);
    printf("👉 Open Browser and visit: http://<Your-Linux-IP>:8080\n");

    // 5. 循环等待连接
    while(1) {
        printf("\nWaiting for connection...\n");
        // 阻塞在这里，直到有浏览器连上来
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Accept");
            continue;
        }

        // --- 握手成功！浏览器连上来了 ---
        
        // 这里可以读取浏览器发了什么，但我们暂时忽略，直接回复它
        
        // 构造 HTTP 响应报文
        // 格式：头信息 + 空行 + HTML内容
        char *http_response = 
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html; charset=utf-8\r\n"
            "\r\n" // 空行极其重要！表示头结束
            "<html>"
            "<body style='text-align:center; padding-top:50px;'>"
            "<h1 style='color:blue; font-size:40px;'>🚀 Hello Week 5!</h1>"
            "<p>This is my first C Web Server.</p>"
            "<p>Current Temperature: <b>26.0 C</b> (Mock Data)</p>"
            "</body>"
            "</html>";

        // 发送给浏览器
        write(new_socket, http_response, strlen(http_response));
        printf("✅ Response sent to browser!\n");

        // 必须关闭连接，因为 HTTP 1.0 是短连接
        close(new_socket);
    }
    
    return 0;
}
