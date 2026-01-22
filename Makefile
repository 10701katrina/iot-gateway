CC = gcc
TARGET = app
# 这里的 *.c 代表当前目录下所有的 .c 文件，以后你加文件它自动识别
SRCS = file_io.c 

$(TARGET): $(SRCS)
	$(CC) $(SRCS) -o $(TARGET)

clean:
	rm -f $(TARGET) sensor_data.txt