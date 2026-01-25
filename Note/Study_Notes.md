# 🐧 Linux 嵌入式开发学习笔记

## 📅 Week 1: 环境搭建与文件 I/O

### 1. 核心命令
* `ls -l`: 查看文件详情（权限、大小）。
* `rm -f app`: 强制删除文件。
* `make`: 根据 Makefile 自动编译。
* `make clean`: 清理生成的垃圾文件。

### 2. 文件 I/O (系统调用)
Linux 下“一切皆文件”。操作硬件（如 GPIO、串口）本质上也是在读写文件。

**核心代码模板：**
```c
int fd = open("log.txt", O_WRONLY | O_CREAT | O_APPEND, 0644);
if (fd < 0) {
    perror("打开失败"); // 必写！打印错误原因
}
write(fd, data, len);
close(fd); // 必做！防止内存泄漏
```

### 3. Makefile 通用模板 (必备！)

这是一个可以复用的 Makefile 模板。以后新建项目，只需要修改 `SRCS` (源文件名) 和 `LIBS` (库) 即可。

**⚠️以此为准，复制到你的 Makefile 中：**

```makefile
# 1. 编译器设置
#(CC: 这是 C Compiler (C 语言编译器) 的缩写。gcc: 这是真正干活的程序（GNU C Compiler）)
CC = gcc

# 2. 目标文件名称 
#(TARGET: 意思是我们最终要生成的那个可执行文件（绿色的那个文件）叫什么名字。app: 这是随手起的名字。)
TARGET = app

# 3. 源文件列表
# (要做哪个实验，就改这里的文件名,SRCS: 是 Sources (源文件) 的缩写。含义: 告诉 Make 工具，原料是哪个 .c 文件。)
# 例如：hello.c, file_io.c, pthread_test.c
SRCS = producer_consumer.c
#如果有多个文件怎么办？你可以写成：SRCS = main.c sensor.c wifi.c (用空格隔开)。

# 4. 链接库 (多线程必须加 -lpthread)
#LIBS: 是 Libraries (库) 的缩写。
LIBS = -lpthread
#-lpthread: 这是一个 GCC 的参数。-l: 代表 link (链接)。pthread: 代表 POSIX thread 库。
#为什么需要它？printf 这种函数是 C 语言自带的（标准库），GCC 默认认识。pthread_create 这种函数属于“扩展包”，GCC 默认不带，你必须显式告诉它：“嘿，编译的时候把线程库也带上！”

# 5. 编译规则 (也就是 make 时执行的命令)
# $@ 代表目标(app), $^ 代表依赖(xxx.c)
$(TARGET): $(SRCS)#app: producer_consumer.c 
#含义：我要生成 app 这个文件，但我依赖 producer_consumer.c
#逻辑: Make 会检查，如果 producer_consumer.c 变动了（比如你刚修改保存了），它就会执行下面的命令重新编译。如果不依赖任何变动，它就不编译（省时间）。
	$(CC) $(SRCS) -o $(TARGET) $(LIBS)
#终端实际执行：gcc producer_consumer.c -o app -lpthread
#⚠️ 极其重要: 这行必须以 Tab 键开头（键盘 Q 左边那个），不能用空格！这是 Makefile 最坑爹的语法规定。


# 6. 清理规则 (make clean)
clean:
#输入 make clean 时，它就会执行下面的命令。
	rm -f $(TARGET) sensor_data.txt
#rm -f $(TARGET)----->>> rm -f app
#含义: 强制删除 (-f force) 生成的 app 文件。这样下次编译就是从零开始，保证干净。
```


    ⭐ 核心考点笔记：

Tab 键陷阱：Makefile 中命令（如 $(CC) 和 rm）的前面必须是 Tab 键，绝对不能用空格！

-lpthread：Linux 默认不链接线程库，编译多线程代码必须显式加上这个参数，否则报错 undefined reference

