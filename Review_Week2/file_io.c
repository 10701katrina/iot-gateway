#include <stdio.h>   // (Standard Input Output)。为了用 printf, perror
#include <string.h>  // (String Handling)。为了用 strlen (算数据长度)
#include <unistd.h>  // （Unix Standard）。包含 write, close
#include <fcntl.h>

int main(){

    int fd;
    char *filename = "sensor_data.txt";
    char *data = "[2026-01-22 18:30] Temp: 28.5C, Humidity: 40%\n";

    fd = open(filename,O_WRONLY | O_CREAT | O_APPEND, 0644);    
    if(fd == -1){
        perror("打开文件失败");
        return 1;
    }   

    int len = write(fd,data,strlen(data));

    if(len > 0){
        printf("成功写入日志: %s", data);
    } else {
        printf("写入失败！\n");
    }
     
    close(fd);

    return 0;   

}