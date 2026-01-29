#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8888 //什么端口？
#define BUFFER_SIZE 1024

int main(){
    int server_fd, client_fd;
    struct sockaddr_in address;//结构体是啥
    int addr_len = sizeof(address);
    char buffer[BUFFER_SIZE]= {0};

    if((server_fd = socket(AF_INET,SOCK_STREAM,0)) == 0){ //socket函数作用、定义；fd==0？
        perror("Socket failed");
        exit(EXIT_FAILURE); //exit函数作用、定义
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if(bind(server_fd,(struct sockaddr *)&address,sizeof(address))<0){
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if(listen(server_fd,3)<0){
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("TCP Server 正在监听端口 %d ...\n", PORT);

    if((client_fd = accept(server_fd,(struct sockaddr *)&address,(socklen_t*)&addr_len))<0){ //accept函数作用、定义，为什么要另一个fd？
        perror("Accept failed");
        exit(EXIT_FAILURE);
    }
    printf(">>> 有客户端连接上来了！\n");

    while(1){
        memset(buffer,0,BUFFER_SIZE); //这个函数是什么意思？0是什么？

        int valread = read(client_fd,buffer,BUFFER_SIZE);//read函数是什么？
        if(valread<0){ //<0是出错吗？
            perror("客户端断开连接。\n");
            break;
        }

        printf("收到客户端消息: %s\n",buffer);

        char *msg = "我收到了！\n"; //为什么用指针？是因为只存字符串第一个字地址吗
        send(client_fd,msg,strlen(msg),0);  //0是什么意思？
    }

    close(client_fd);
    close(server_fd);
    return 0;
}