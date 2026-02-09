
# 🚀 基于 Linux 与 RTOS 的多线程智能边缘网关
**Smart Edge Gateway V2.0**
### 系统架构图

```mermaid
graph TD
    %% 这里粘贴我刚才给你的【视图一】的所有代码
    classDef linux fill:#e1f5fe,stroke:#0288d1,stroke-width:2px;
    classDef stm32 fill:#fff3e0,stroke:#f57c00,stroke-width:2px;
    %% ... (省略中间代码) ...
    class FreeRTOS,HAL stm32;
```

> 一个集成了 **STM32 数据采集**、**FreeRTOS 实时调度** 与 **Linux 多线程边缘计算** 的完整 AIoT 解决方案。

---

## 📖 项目简介 (Introduction)

本项目旨在解决物联网场景下高频传感器数据的采集、传输、解析与丢包问题。

系统采用经典的**生产者-消费者模型**，通过手写的**线程安全环形缓冲区 (Ring Buffer)** 实现了串口数据接收与业务逻辑处理的异步解耦。STM32 端运行 FreeRTOS 进行多任务调度，Linux 端作为边缘网关，提供 HTTP Web 服务实现远程实时监控与反向控制。

## ✨ 核心功能

* **多线程并发架构**：Linux 端采用 `Pthreads` 实现采集、解析、Web 服务三线程并行，CPU 利用率高效。
* **零丢包传输**：通过自研环形缓冲区与状态机（FSM）协议解析，解决串口通信中的粘包与数据丢失问题。
* **边缘计算策略**：网关本地集成自动化控制逻辑（如：光照过低自动开灯），降低云端依赖。
* **Web 可视化监控**：内嵌轻量级 HTTP 服务器，支持浏览器实时查看环境数据（温湿度、光照）及远程控制。


### Web 可视化监控
内嵌轻量级 HTTP 服务器，支持浏览器实时查看环境数据及远程控制。

<div align="center">
  <img src="https://github.com/user-attachments/assets/2d2c8ad2-f58e-4042-b38c-552c9c6cb327" width="800" alt="Web Dashboard" style="border-radius: 10px; box-shadow: 0 4px 8px rgba(0,0,0,0.2);">
  <br>
  <em>STM32 智能边缘网关 - 实时监控面板</em>
</div>

---

## 🛠️ 技术栈 (Tech Stack)

### 💻 Linux 网关端 (Software)
* **开发语言**：C 语言 (C99 Standard)
* **系统编程**：Multi-threading (pthread), Mutex/Cond (同步互斥), File I/O
* **网络编程**：TCP/IP Socket, HTTP 1.1 协议构建
* **数据结构**：Ring Buffer (环形缓冲区), Linked List
* **构建工具**：GCC, Makefile

### 🔌 STM32 硬件端 (Hardware & Firmware)
* **MCU**：STM32F103C8T6
* **操作系统**：FreeRTOS (任务调度/消息队列)
* **传感器**：DHT11 (温湿度), LDR (光敏电阻)
* **通信接口**：UART (串口 DMA/中断)
* **开发环境**：Keil MDK 5, STM32CubeMX

---

## 📡 通信协议 (Communication Protocol)

系统采用自定义的 ASCII 文本帧协议，具备头部引导与尾部校验，有效防止粘包。

### 1. 上行数据 (STM32 -> Linux)
* **格式**：`$ENV,温度,湿度,光照#`
* **示例**：`$ENV,25,60,15#`

| 字段 | 说明 | 示例 |
| :--- | :--- | :--- |
| `$` | 帧头 | - |
| `ENV` | 数据类型标识 | - |
| `temp` | 温度值 (int) | `25` |
| `humi` | 湿度值 (int) | `60` |
| `light` | 光照强度 (int) | `15` |
| `#` | 帧尾 | - |

### 2. 下行控制 (Linux -> STM32)
* **格式**：`$CMD,指令#`
* **示例**：`$CMD,LED_ON#`

| 指令内容 | 功能描述 |
| :--- | :--- |
| `$CMD,LED_ON#` | 强制开启设备 LED |
| `$CMD,LED_OFF#` | 强制关闭设备 LED |

---

## 🚀 快速开始 (Quick Start)

### 1. 硬件连接
* 将 STM32 的串口 (TX/RX) 通过 USB-TTL 模块连接至 Linux 主机 (或虚拟机)。
* 确认设备节点（如 `/dev/ttyUSB0`）。

### 2. 编译 Linux 网关

```bash
cd Linux_Gateway
make clean
make
```
### 3. 运行网关程序

```bash
# 需要 sudo 权限以访问串口设备
sudo ./iot_gateway
```
### 4. 访问 Web 监控
打开浏览器，访问 Linux 主机的 IP 地址：
* `http://localhost:8080` (如果是本地运行)
* `http://<你的Linux_IP>:8080`

---

## 💡 技术难点与解决方案 (Highlights)

* **难点 1：高频数据下的串口丢包**
    * **解决**：放弃传统的单线程阻塞读写，设计了独立**生产者线程**，只负责将串口原始数据极速搬运至内存。

* **难点 2：多线程数据竞争**
    * **解决**：设计 Ring Buffer 作为共享资源池，使用互斥锁 (`pthread_mutex`) 保护临界区，配合条件变量 (`pthread_cond`) 实现“无数据休眠，有数据唤醒”的高效同步机制，避免 CPU 空转。

* **难点 3：TCP/串口粘包**
    * **解决**：在消费者线程中实现了一个**有限状态机 (FSM)**，逐字节解析数据流，精准识别 `$` 和 `#` 边界，确保应用层获取的数据帧绝对完整。

---

## 📧 联系作者

* **作者**：Katrina
* **电子邮件**：1635712009@qq.com
* **GitHub**：[github.com/10701katrina](https://github.com/10701katrina)

本项目为嵌入式 Linux 学习与实践项目，欢迎 Issue 和 Star！ ⭐
