CC = gcc
TARGET = app
# 改成新文件名
SRCS = producer_consumer.c

# 重点：新增链接库参数 -lpthread
LIBS = -lpthread

$(TARGET): $(SRCS)
	$(CC) $(SRCS) -o $(TARGET) $(LIBS)

clean:
	rm -f $(TARGET)