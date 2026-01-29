# 1. producer_consumer.c 线程函数
* 你只需要记住以下 4 组 核心函数。这几组函数构成了 Linux 多线程编程的 90% 场景，也是面试中手写代码（Whiteboard Coding）最常考的内容：

    * 基础管理：

        pthread_create：创建分身（派任务）。

        pthread_join：等待收尸（收作业）。

    * 互斥锁 (Mutex)：

        pthread_mutex_init / destroy：初始化与销毁。

        pthread_mutex_lock / unlock：加锁与解锁（抢钥匙与还钥匙）。

    * 条件变量 (Cond)：

        pthread_cond_init / destroy：闹钟的准备与拆除。

        pthread_cond_wait / signal：睡觉与唤醒。

    * 退出机制：

        pthread_exit：线程自己主动提前退休。

    学习标准：不需要背出每个参数的顺序，但要能写出伪代码。比如：pthread_create(&id, NULL, func, &arg); 这种程度即可。


* 为了让你在面试中展现出“工业级”视野，你需要了解以下几个高阶概念（知道它们的存在即可，不需要背函数）：

    A. 线程分离 (Detached Thread)
        函数：pthread_detach

        场景：如果你不想让主线程在那死等（join），可以设置分离。线程干完活自动消失，系统自动回收资源。

        联系：这适合那些“只管跑，不用回传结果”的背景任务。

    B. 属性设置 (Attributes)
        函数：pthread_attr_init

        场景：调整线程的“性格”。比如修改线程的栈大小（Stack Size）。

        联系：在你提到的项目中，有“任务堆栈检测机制”。在嵌入式 Linux 中，如果内存紧张，你就需要通过属性函数调小线程的默认栈空间。

    C. 读写锁 (Read-Write Lock)
        函数：pthread_rwlock_t

        场景：如果有 100 个线程在读数据，只有 1 个线程在写。

        逻辑：普通锁太浪费了（读的人也要排队）。读写锁允许多个人同时读，但只要有人想写，就必须独占。


# 📉 bind() 和 listen() 失败原因大揭秘

在网络编程中，我们必须通过 `if (func(...) < 0)` 来捕获错误。当这两个函数返回 `-1` 时，通常是因为触发了 Linux 内核的某种“保护机制”或“逻辑校验”。

可以使用 `perror("函数名")` 打印具体报错信息（Error Message）。

---

## 1. bind() 失败的原因
**作用**：将 socket（手机）绑定到具体的 IP 和端口（电话号码）上。

| 错误代码 (errno) | 错误信息 (perror) | **核心原因 (大白话)** | **解决方案** |
| :--- | :--- | :--- | :--- |
| **EADDRINUSE** | `Address already in use` | **端口已被占用（最常见！）**<br>1. 你的服务器程序已经在运行了，你又启动了一个。<br>2. 另一个程序占用了这个端口。<br>3. 程序刚关，但 TCP 处于 `TIME_WAIT` 状态（端口没释放干净）。 | 1. 关掉正在跑的进程 (`kill`)。<br>2. 换个端口号 (如 8889)。<br>3. **高级招数**：使用 `setsockopt` 设置 `SO_REUSEADDR` 允许地址重用。 |
| **EACCES** | `Permission denied` | **权限不足**<br>你想绑定 **0~1023** 号端口（特权端口），但你不是 root 用户。 | 1. 改用 **1024~65535** 的端口。<br>2. 使用 `sudo` 运行程序（不推荐）。 |
| **EADDRNOTAVAIL** | `Cannot assign requested address` | **IP 地址无效**<br>你试图绑定一个**不属于这台电脑**的 IP 地址。<br>例如：你的本机 IP 是 `192.168.1.5`，你非要绑定 `192.168.1.6`。 | 1. 检查 IP 是否写错。<br>2. 推荐使用 `INADDR_ANY` (自动适配本机所有 IP)。 |
| **EBADF** | `Bad file descriptor` | **fd 无效**<br>传进去的 `server_fd` 是个坏的（比如 socket 创建失败了你没处理，或者已经被 `close` 了）。 | 检查前面的 `socket()` 函数返回值是否 `< 0`。 |
| **EINVAL** | `Invalid argument` | **重复绑定**<br>这个 socket 已经被绑定过一次了，你又绑了一次。 | 检查代码逻辑，不要重复调用 `bind`。 |



---

## 2. listen() 失败的原因
**作用**：将 socket 设置为被动监听模式，准备接受连接。

| 错误代码 (errno) | 错误信息 (perror) | **核心原因 (大白话)** | **解决方案** |
| :--- | :--- | :--- | :--- |
| **EOPNOTSUPP** | `Operation not supported` | **协议不支持（最常见！）**<br>你试图监听一个 **UDP** socket。<br>UDP 是无连接的，不需要监听，只能直接发数据。 | 1. 确认 `socket()` 时用的是 `SOCK_STREAM` (TCP)。<br>2. 如果是 UDP，删掉 `listen` 和 `accept` 代码。 |
| **EBADF** | `Bad file descriptor` | **fd 无效**<br>同上，socket 句柄本身就是坏的。 | 检查 socket 创建逻辑。 |
| **EDESTADDRREQ** | `Destination address required` | **未绑定地址**<br>有些系统要求 `listen` 前必须成功 `bind` (虽然大部分现代 Linux 会自动分配，但为了稳定建议先 bind)。 | 确保在 `listen` 之前调用了 `bind`。 |

---

## 💡 调试小贴士：如何看到上面的错误？

在代码中，永远不要只写 `exit(1)`，一定要把错误打印出来！

**推荐写法：**
```c
if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    // perror 会自动把 errno 翻译成人类能看懂的英语句子
    // 输出示例：Bind failed: Address already in use
    perror("Bind failed"); 
    exit(EXIT_FAILURE);
}