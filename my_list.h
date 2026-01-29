#ifndef _MY_LIST_H
#define _MY_LIST_H

//你需要达到什么水平？
    //背下来：struct list_head { prev, next } 的结构。
    //画出来：能画出节点插入时，4 根指针是如何断开重连的。
    //说清楚：container_of 宏是如何通过“偏移量”从成员指针反推结构体首地址的（这是降维打击面试官的神技）。
//对于链表：
    //花 20 分钟把我在上面发给你的 my_list.h 代码看懂。
    //特别是 list_add 和 container_of 这两个宏。

#include <stddef.h> // for offsetof

// 1. 定义链表节点 (只有前后指针，没有数据！)
// 这就像一个通用的“挂钩”
struct list_head {
    struct list_head *next, *prev;
};

// 2. 初始化链表头
// 让头节点的前后指针都指向自己
static inline void INIT_LIST_HEAD(struct list_head *list) {
    list->next = list;
    list->prev = list;
}

// 3. 插入节点 (核心通用逻辑)
// new: 新节点, prev: 前一个, next: 后一个
static inline void __list_add(struct list_head *new,
                              struct list_head *prev,
                              struct list_head *next) {
    next->prev = new;
    new->next = next;
    new->prev = prev;
    prev->next = new;
}

// 头插法
static inline void list_add(struct list_head *new, struct list_head *head) {
    __list_add(new, head, head->next);
}

// 尾插法
static inline void list_add_tail(struct list_head *new, struct list_head *head) {
    __list_add(new, head->prev, head);
}

// 4. 删除节点
static inline void __list_del(struct list_head *prev, struct list_head *next) {
    next->prev = prev;
    prev->next = next;
}

static inline void list_del(struct list_head *entry) {
    __list_del(entry->prev, entry->next);
    entry->next = NULL;
    entry->prev = NULL;
}

// 5. 判空
static inline int list_empty(const struct list_head *head) {
    return head->next == head;
}

// ==========================================================
// 🔥 魔法宏：container_of (面试必问)
// 作用：已知“挂钩(member)”的地址，反推“大结构体(type)”的起始地址
// 原理：结构体地址 = 成员地址 - 成员在结构体内的偏移量
// ==========================================================
#define container_of(ptr, type, member) ({          \
    const typeof( ((type *)0)->member ) *__mptr = (ptr); \
    (type *)( (char *)__mptr - offsetof(type,member) );})

// 6. 遍历链表的宏
// pos: 循环游标(大结构体指针), head: 链表头, member: 大结构体里list_head的名字
#define list_for_each_entry(pos, head, member)              \
    for (pos = container_of((head)->next, typeof(*pos), member); \
         &pos->member != (head);    \
         pos = container_of(pos->member.next, typeof(*pos), member))

#endif