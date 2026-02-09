/**
 ****************************************************************************************************
 * @file        gtim.h
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2021-10-15
 * @brief       通用定时器 驱动代码
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:正点原子 STM32F407开发板
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 * 公司网址:www.alientek.com
 * 购买地址:openedv.taobao.com
 *
 * 修改说明
 * V1.0 20211015
 * 第一次发布
 * V1.1 20211015
 * 新增gtim_timx_pwm_chy_init函数
 * V1.2 20211015
 * 1,新增gtim_timx_cap_chy_init函数
 * V1.3 20211015
 * 1,支持外部脉冲计数功能
 * 2,新增gtim_timx_cnt_chy_init,gtim_timx_cnt_chy_get_count和gtim_timx_cnt_chy_restart三个函数  
 * 
 ****************************************************************************************************
 */

#ifndef __GTIM_H
#define __GTIM_H

#include "./SYSTEM/sys/sys.h"

/******************************************************************************************/
/* 通用定时器 定义 */

/* TIMX 中断定义 
 * 默认是针对TIM2~TIM5
 * 注意: 通过修改这4个宏定义,可以支持TIM1~TIM8任意一个定时器.
 */
 
#define GTIM_TIMX_INT                       TIM3
#define GTIM_TIMX_INT_IRQn                  TIM3_IRQn
#define GTIM_TIMX_INT_IRQHandler            TIM3_IRQHandler
#define GTIM_TIMX_INT_CLK_ENABLE()          do{ __HAL_RCC_TIM3_CLK_ENABLE(); }while(0)   /* TIM3 时钟使能 */

/* TIMX PWM输出定义 
 * 这里输出的PWM控制LED0(RED)的亮度
 * 默认是针对TIM2~TIM5
 * 注意: 通过修改这几个宏定义,可以支持TIM1~TIM8任意一个定时器,任意一个IO口输出PWM
 */
#define GTIM_TIMX_PWM_CHY_GPIO_PORT         GPIOF
#define GTIM_TIMX_PWM_CHY_GPIO_PIN          GPIO_PIN_9
#define GTIM_TIMX_PWM_CHY_GPIO_CLK_ENABLE() do{ __HAL_RCC_GPIOF_CLK_ENABLE(); }while(0)  /* PF口时钟使能 */
#define GTIM_TIMX_PWM_CHY_GPIO_AF           GPIO_AF9_TIM14                               /* 端口复用到TIM14 */

/* TIMX REMAP设置
 * 因为我们LED0接在PB5上, 必须通过开启TIM3的部分重映射功能, 才能将TIM3_CH2输出到PB5上
 * 因此, 必须实现GTIM_TIMX_PWM_CHY_GPIO_REMAP
 * 对那些使用默认设置的定时器PWM输出脚, 不用设置重映射, 是不需要该函数的!
 */

#define GTIM_TIMX_PWM                       TIM14                                        /* TIM14 */
#define GTIM_TIMX_PWM_CHY                   TIM_CHANNEL_1                                /* 通道Y,  1<= Y <=4 */
#define GTIM_TIMX_PWM_CHY_CCRX              TIM14->CCR1                                  /* 通道Y的输出比较寄存器 */
#define GTIM_TIMX_PWM_CHY_CLK_ENABLE()      do{ __HAL_RCC_TIM14_CLK_ENABLE(); }while(0)  /* TIM3 时钟使能 */

 /* TIMX 输入捕获定义 
 * 这里的输入捕获使用定时器TIM5_CH1,捕获WK_UP按键的输入
 * 默认是针对TIM2~TIM5. 
 * 注意: 通过修改这几个宏定义,可以支持TIM1~TIM8任意一个定时器,任意一个IO口做输入捕获
 *       特别要注意:默认用的PA0,设置的是下拉输入!如果改其他IO,对应的上下拉方式也得改!
 */
#define GTIM_TIMX_CAP_CHY_GPIO_PORT         GPIOA
#define GTIM_TIMX_CAP_CHY_GPIO_PIN          GPIO_PIN_0
#define GTIM_TIMX_CAP_CHY_GPIO_CLK_ENABLE() do{ __HAL_RCC_GPIOA_CLK_ENABLE(); }while(0)   /* PA口时钟使能 */
#define GTIM_TIMX_CAP_CHY_GPIO_AF           GPIO_AF2_TIM5                                 /* AF功能选择 */

#define GTIM_TIMX_CAP                       TIM5
#define GTIM_TIMX_CAP_IRQn                  TIM5_IRQn
#define GTIM_TIMX_CAP_IRQHandler            TIM5_IRQHandler
#define GTIM_TIMX_CAP_CHY                   TIM_CHANNEL_1                                 /* 通道Y,  1<= Y <=4 */
#define GTIM_TIMX_CAP_CHY_CCRX              TIM5->CCR1                                    /* 通道Y的输出比较寄存器 */
#define GTIM_TIMX_CAP_CHY_CLK_ENABLE()      do{ __HAL_RCC_TIM5_CLK_ENABLE(); }while(0)    /* TIM5 时钟使能 */


/* TIMX 输入计数定义
* 这里的输入计数使用定时器TIM2_CH1,捕获WK_UP按键的输入
* 默认是针对TIM2~TIM5, 只有CH1和CH2通道可以用做输入计数, CH3/CH4不支持!
* 注意: 通过修改这几个宏定义,可以支持TIM1~TIM8任意一个定时器,CH1/CH2对应IO口做输入计数
*       特别要注意:默认用的PA0,设置的是下拉输入!如果改其他IO,对应的上下拉方式也得改!
*/
#define GTIM_TIMX_CNT_CHY_GPIO_PORT            GPIOA
#define GTIM_TIMX_CNT_CHY_GPIO_PIN             GPIO_PIN_0
#define GTIM_TIMX_CNT_CHY_GPIO_CLK_ENABLE()    do{ __HAL_RCC_GPIOA_CLK_ENABLE(); }while(0)   /* PA口时钟使能 */

#define GTIM_TIMX_CNT                          TIM2
#define GTIM_TIMX_CNT_IRQn                     TIM2_IRQn
#define GTIM_TIMX_CNT_IRQHandler               TIM2_IRQHandler
#define GTIM_TIMX_CNT_CHY                      TIM_CHANNEL_1                            /* 通道Y,  1<= Y <=2 */
#define GTIM_TIMX_CNT_CHY_CLK_ENABLE()         do{ __HAL_RCC_TIM2_CLK_ENABLE(); }while(0)    /* TIM2 时钟使能 */

/******************************************************************************************/

void gtim_timx_int_init(uint16_t arr, uint16_t psc);        /* 通用定时器 定时中断初始化函数 */
void gtim_timx_pwm_chy_init(uint16_t arr, uint16_t psc);    /* 通用定时器 PWM初始化函数 */
void gtim_timx_cap_chy_init(uint32_t arr, uint16_t psc);    /* 通用定时器 输入捕获初始化函数 */
void gtim_timx_cnt_chy_init(uint16_t psc);                  /* 通用定时器 脉冲计数初始化函数 */
uint32_t gtim_timx_cnt_chy_get_count(void);                 /* 通用定时器 获取脉冲计数 */
void gtim_timx_cnt_chy_restart(void);                       /* 通用定时器 重启计数器 */

#endif

















