# 定义变量：编译器用 gcc
CC = gcc

# 定义目标文件：我们要生成的程序叫 app
TARGET = app

# 编译规则：app 依赖于 hello.c
$(TARGET): hello.c
	$(CC) hello.c -o $(TARGET)

# 清理规则：输入 make clean 就会删掉生成的程序
clean:
	rm -f $(TARGET)