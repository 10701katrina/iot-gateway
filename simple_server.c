#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h> //(Internet Operations) 提供了用于处理网络地址转换和操作的函数。
//计算机内部处理数字很快，但人类习惯看字符串形式的 IP 地址。该库就在两者之间架起桥梁。
#include <sys/socket.h> //(System Socket)定义了创建、连接和监听网络连接所需的函数和数据结构。

#define PORT 8888
//非特权端口：在 Linux 中，0~1023 号端口（如 80 HTTP, 22 SSH）是保留给系统管理员（root）用的。如果你不是 root 权限运行程序，绑定这些端口会报错“Permission denied”。
//测试常用：1024~65535 是普通用户可以用的端口。程序员习惯用 8080, 8888, 3000 这些好记的数字来做开发测试。
//它就是我们要监听的“分机号”。
#define BUFFER_SIZE 1024

int main() {
    int server_fd, client_fd;
    //server_fd (监听 socket) = 站在酒店门口的迎宾员。【建立连接】
    //client_fd (已连接 socket) = 专门服务某个房间的服务员。【传输数据】
    struct sockaddr_in address;
    //sockaddr = Socket Address (套接字地址)
    //_in = Internet (互联网/IPv4)
    //这是 Linux 网络编程中用来专门存放 IPv4 地址信息的结构体。
    int addr_len = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};

    // 1. 创建 Socket (买手机)
    // AF_INET: IPv4协议, SOCK_STREAM: TCP协议
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        //函数定义：int socket(int domain, int type, int protocol);
        //socket() 系统调用用于在内核空间创建一个通信端点，并返回一个文件描述符供用户态程序操作。【资源分配与端点创建】
        //AF_INET (Address Family - IPv4)：我们要用 IPv4 协议（如果是 IPv6 就用 AF_INET6）。
        //SOCK_STREAM (Socket Type - TCP)：我们要用 TCP 协议（稳定、有序、面向连接），对比：如果是 UDP 就用 SOCK_DGRAM(数据报)。
        //0 (Protocol)：自动选择默认协议。对于 SOCK_STREAM，默认就是 TCP。
        //返回值：成功时返回一个非负整数，称为“文件描述符”，比如 3。这就是你买到的“手机”。它是我们后续操作这个 Socket 的唯一标识符。
        //对于 socket() 函数，返回 -1 代表失败。所以一定要用 < 0 来捕捉错误。

        perror("Socket failed");
        exit(EXIT_FAILURE);
        //exit(0) 或者 exit(EXIT_SUCCESS) 表示程序成功结束。
        //exit(1) 或者 exit(EXIT_FAILURE) 表示程序异常终止。
    }

    // 配置地址结构体 (准备电话卡)
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // 允许任何 IP 连接我
    //INADDR_ANY 是一个宏，本质上是数字 0（也就是 IP 地址 0.0.0.0）。
    //告诉内核：“只要是发给这台电脑的包，无论从哪个网卡进来的，我都收。”
    address.sin_port = htons(PORT);       // 端口号转换成网络字节序(大端)
    //htons = Host to Network Short (主机字节序 转 网络字节序，短整型)。
    //把人类和电脑习惯的数字格式（小端序），转换成网络标准格式（大端序）。


    // 2. 绑定端口 (插卡)
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    //bind() 系统调用将一个具体的网络地址（IP 地址 + 端口号）与之前创建的 socket 文件描述符进行强制关联。【地址注册与命名】
    //(struct sockaddr *)&address:强制类型转换
    //把 IPv4 专用名片 (sockaddr_in)，强行伪装成 通用名片 (sockaddr)，才能递给 bind 函数。
    //为了通用性，设计者只接受一种通用的结构体，叫 struct sockaddr。不管是 IPv4 还是 IPv6，传进来的时候都得把名字改成这个通用名字。

    //bind、listen、connect、write 这种“执行动作”的函数，都有一个通用的返回值标准：
    //返回值为 0 表示成功，返回 -1 表示失败。

        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // 3. 开始监听 (等电话)
    if (listen(server_fd, 3) < 0) {
        //listen() 系统调用将 socket 从 主动状态（Active） 转换为 被动监听状态（Passive），并指示内核为该 socket 分配并维护两个连接队列：半连接队列、全连接队列。【状态切换与队列初始化】
        //backlog 参数主要限制全连接队列的长度（在某些旧内核或配置下可能限制两队列之和）。如果队列已满，新的连接请求（SYN）可能会被内核直接丢弃或忽略。
        //“内核大哥，我要开始接客了。但我只有一张嘴（单线程），忙不过来的时候，麻烦你在门口帮我先留住 3 个人，多了的你就帮我赶走吧。”
        
        //三次握手发生
        
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }
    
    printf("TCP Server 正在监听端口 %d ...\n", PORT);

    // 4. 阻塞等待连接 (电话响了，接起来)
    // 注意：accept 返回的 client_fd 是专门用来跟这个客户聊天的
    if ((client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addr_len)) < 0) {
        //accept 的作用是从全连接队列里取出一个已经完成三次握手的连接，并为这个连接创建一个新的文件描述符（client_fd）。
        perror("Accept failed");
        exit(EXIT_FAILURE);
    }
    
    printf(">>> 有客户端连接上来了！\n");

    // 5. 循环收发数据
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        //memset = Memory Set。用来把一块内存填满同一个值。
        //这里的作用是把 buffer 这个数组的前 BUFFER_SIZE 个字节全部填成 0。
        //0 在 ASCII 码表里是 NUL 字符，表示字符串的结束。所以这一步相当于“清空”缓冲区，防止上次的数据残留影响这次读取。
        
        // 读取数据 (recv 也是阻塞的)
        int valread = read(client_fd, buffer, BUFFER_SIZE); 
        //read 函数从 client_fd（电话线）中读取数据，存到 buffer 里，最多读 BUFFER_SIZE 个字节。
        //返回值 valread 是实际读取到的字节数。如果返回值是 0，表示对方关闭了连接；如果是 -1，表示读取出错。如果大于 0，表示成功读取到数据。
        if (valread < 0) { 
            printf("Read error");
            break;// 读取出错，跳出循环
        }else if (valread == 0) {
            //当客户端调用 close() 关闭连接时，服务器端的 read 不会阻塞，而是立即返回 0（代表 End of File / EOF）。
            //如果不处理 == 0，你的 while(1) 就会变成死循环，疯狂打印空消息，CPU 瞬间飙升到 100%。
            printf("客户端断开了连接。\n");
            break; // 对方挂了，我也退出
        } else {
            // 正常处理数据
            printf("收到: %s\n", buffer);
        }
        

        // 回发数据 (Echo)
        char *msg = "我收到了！\n";
        send(client_fd, msg, strlen(msg), 0);
    }

    // 6. 挂电话
    close(client_fd);
    close(server_fd);
    return 0;
}