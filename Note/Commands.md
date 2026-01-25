## 1. producer_consumer.c 线程函数
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


## 2. File_io.c
* int fd; 
    * 核心概念：文件描述符 (File Descriptor)
    * 通俗理解：这就是我们之前说的“号码牌”。
        * 在 Linux 内核里，它不会记录“sensor_data.txt”这个文件名，它只认 fd 这个数字（比如 3）。
        * 以后所有的操作（写、读、关），你都只需要向系统出示这个数字。

        