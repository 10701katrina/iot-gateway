/**
 ****************************************************************************************************
 * @file        atim.h
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.3
 * @date        2021-10-19
 * @brief       高级定时器 驱动代码
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
 * V1.0 20211019
 * 第一次发布
 * V1.1 20211019
 * 1, 新增atim_timx_comp_pwm_init函数, 实现输出比较模式PWM输出功能
 * V1.2 20211019
 * 1, 增加atim_timx_cplm_pwm_init函数
 * 2, 增加atim_timx_cplm_pwm_set函数
 * V1.3 20211019
 * 1, 增加atim_timx_pwmin_chy_init函数
 * 2, 增加atim_timx_pwmin_chy_restart函数
 *
 ****************************************************************************************************
 */

#ifndef __ATIM_H
#define __ATIM_H

#include "./SYSTEM/sys/sys.h"

/******************************************************************************************/
/* 高级定时器 定义 */

 /* TIMX 输出指定个数PWM 定义 
 * 这里输出的PWM通过PC6(TIM8_CH1)输出, 我们用杜邦线连接PC6和PE5, 然后在程序里面将PE5设置成浮空输入
 * 就可以 看到TIM8_CH1控制LED1(GREEN)的亮灭, 亮灭一次表示一个PWM波
 * 默认使用的是TIM8_CH1.
 * 注意: 通过修改这几个宏定义, 可以支持TIM1/TIM8定时器, 任意一个IO口输出指定个数的PWM
 */
#define ATIM_TIMX_NPWM_CHY_GPIO_PORT            GPIOC
#define ATIM_TIMX_NPWM_CHY_GPIO_PIN             GPIO_PIN_6
#define ATIM_TIMX_NPWM_CHY_GPIO_CLK_ENABLE()    do{  __HAL_RCC_GPIOC_CLK_ENABLE(); }while(0)    /* PC口时钟使能 */

#define ATIM_TIMX_NPWM_CHY_GPIO_AF              GPIO_AF3_TIM8

#define ATIM_TIMX_NPWM                          TIM8
#define ATIM_TIMX_NPWM_IRQn                     TIM8_UP_TIM13_IRQn
#define ATIM_TIMX_NPWM_IRQHandler               TIM8_UP_TIM13_IRQHandler
#define ATIM_TIMX_NPWM_CHY                      TIM_CHANNEL_1                               /* 通道Y,  1<= Y <=4 */
#define ATIM_TIMX_NPWM_CHY_CCRX                 TIM8->CCR1                                  /* 通道Y的输出比较寄存器 */
#define ATIM_TIMX_NPWM_CHY_CLK_ENABLE()         do{ __HAL_RCC_TIM8_CLK_ENABLE(); }while(0)  /* TIM8 时钟使能 */


/* TIMX 输出比较模式 定义 
 * 这里通过TIM8的输出比较模式,控制PC6,PC7,PC8,PC9输出4路PWM,占空比50%,并且每一路PWM之间的相位差为25%
 * 修改CCRx可以修改相位.
 * 默认是针对TIM8
 * 注意: 通过修改这些宏定义,可以支持TIM1/TIM8任意一个定时器,任意一个IO口使用输出比较模式,输出PWM
 */
#define ATIM_TIMX_COMP_CH1_GPIO_PORT            GPIOC
#define ATIM_TIMX_COMP_CH1_GPIO_PIN             GPIO_PIN_6
#define ATIM_TIMX_COMP_CH1_GPIO_CLK_ENABLE()    do{ __HAL_RCC_GPIOC_CLK_ENABLE(); }while(0)   /* PC口时钟使能 */

#define ATIM_TIMX_COMP_CH2_GPIO_PORT            GPIOC
#define ATIM_TIMX_COMP_CH2_GPIO_PIN             GPIO_PIN_7
#define ATIM_TIMX_COMP_CH2_GPIO_CLK_ENABLE()    do{ __HAL_RCC_GPIOC_CLK_ENABLE(); }while(0)   /* PC口时钟使能 */

#define ATIM_TIMX_COMP_CH3_GPIO_PORT            GPIOC
#define ATIM_TIMX_COMP_CH3_GPIO_PIN             GPIO_PIN_8
#define ATIM_TIMX_COMP_CH3_GPIO_CLK_ENABLE()    do{ __HAL_RCC_GPIOC_CLK_ENABLE(); }while(0)   /* PC口时钟使能 */

#define ATIM_TIMX_COMP_CH4_GPIO_PORT            GPIOC
#define ATIM_TIMX_COMP_CH4_GPIO_PIN             GPIO_PIN_9
#define ATIM_TIMX_COMP_CH4_GPIO_CLK_ENABLE()    do{ __HAL_RCC_GPIOC_CLK_ENABLE(); }while(0)   /* PC口时钟使能 */

#define ATIM_TIMX_COMP_GPIO_AF                  GPIO_AF3_TIM8

#define ATIM_TIMX_COMP                          TIM8
#define ATIM_TIMX_COMP_CH1_CCRX                 ATIM_TIMX_COMP->CCR1                          /* 通道1的输出比较寄存器 */
#define ATIM_TIMX_COMP_CH2_CCRX                 ATIM_TIMX_COMP->CCR2                          /* 通道2的输出比较寄存器 */
#define ATIM_TIMX_COMP_CH3_CCRX                 ATIM_TIMX_COMP->CCR3                          /* 通道3的输出比较寄存器 */
#define ATIM_TIMX_COMP_CH4_CCRX                 ATIM_TIMX_COMP->CCR4                          /* 通道4的输出比较寄存器 */
#define ATIM_TIMX_COMP_CLK_ENABLE()             do{ __HAL_RCC_TIM8_CLK_ENABLE(); }while(0)    /* TIM8 时钟使能 */
 

/* TIMX 互补输出模式 定义 
 * 这里设置互补输出相关硬件配置, CHY即正常输出, CHYN即互补输出
 * 修改CCRx可以修改占空比.
 * 默认是针对TIM1
 * 注意: 通过修改这些宏定义,可以支持TIM1/TIM8定时器, 任意一个IO口输出互补PWM(前提是必须有互补输出功能)
 */

/* 输出通道引脚 */
#define ATIM_TIMX_CPLM_CHY_GPIO_PORT            GPIOE
#define ATIM_TIMX_CPLM_CHY_GPIO_PIN             GPIO_PIN_9
#define ATIM_TIMX_CPLM_CHY_GPIO_CLK_ENABLE()    do{ __HAL_RCC_GPIOE_CLK_ENABLE(); }while(0)   /* PE口时钟使能 */

/* 互补输出通道引脚 */
#define ATIM_TIMX_CPLM_CHYN_GPIO_PORT           GPIOE
#define ATIM_TIMX_CPLM_CHYN_GPIO_PIN            GPIO_PIN_8
#define ATIM_TIMX_CPLM_CHYN_GPIO_CLK_ENABLE()   do{ __HAL_RCC_GPIOE_CLK_ENABLE(); }while(0)   /* PE口时钟使能 */

/* 刹车输入引脚 */
#define ATIM_TIMX_CPLM_BKIN_GPIO_PORT           GPIOE
#define ATIM_TIMX_CPLM_BKIN_GPIO_PIN            GPIO_PIN_15
#define ATIM_TIMX_CPLM_BKIN_GPIO_CLK_ENABLE()   do{ __HAL_RCC_GPIOE_CLK_ENABLE(); }while(0)   /* PE口时钟使能 */

/* TIMX REMAP设置
 * 因为PE8/PE9/PE15, 默认并不是TIM1的复用功能脚, 必须开启完全重映射, 才可以将: TIM1_CH1->PE9; TIM1_CH1N->PE8; TIM1_BKIN->PE15;
 * 这样, PE8/PE9/PE15, 才能用作TIM1的CH1N/CH1/BKIN功能.
 * 因此, 必须实现ATIM_TIMX_CPLM_CHYN_GPIO_REMAP, 通过sys_gpio_remap_set函数设置重映射
 * 如果我们使用默认的复用功能输出, 则不用设置重映射, 是可以不需要该函数的! 根据具体需要来实现.
 */
#define ATIM_TIMX_CPLM_CHY_GPIO_AF             GPIO_AF1_TIM1

/* 互补输出使用的定时器 */
#define ATIM_TIMX_CPLM                          TIM1
#define ATIM_TIMX_CPLM_CHY                      TIM_CHANNEL_1
#define ATIM_TIMX_CPLM_CHY_CCRY                 ATIM_TIMX_CPLM->CCR1
#define ATIM_TIMX_CPLM_CLK_ENABLE()             do{ __HAL_RCC_TIM1_CLK_ENABLE(); }while(0)    /* TIM1 时钟使能 */

/* TIMX PWM输入模式 定义 
 * 这里的输入捕获使用定时器TIM1_CH1,捕获WK_UP按键的输入
 * 默认是针对TIM1/TIM8等高级定时器 
 * 注意: 通过修改这几个宏定义,可以支持TIM1~TIM8任意一个定时器的通道1/通道2
 */
#define ATIM_TIMX_PWMIN_CHY_GPIO_PORT           GPIOC
#define ATIM_TIMX_PWMIN_CHY_GPIO_PIN            GPIO_PIN_6
#define ATIM_TIMX_PWMIN_CHY_GPIO_AF             GPIO_AF3_TIM8
#define ATIM_TIMX_PWMIN_CHY_GPIO_CLK_ENABLE()   do{ __HAL_RCC_GPIOC_CLK_ENABLE(); }while(0)   /* PC口时钟使能 */

#define ATIM_TIMX_PWMIN                         TIM8
#define ATIM_TIMX_PWMIN_IRQn                    TIM8_UP_TIM13_IRQn
#define ATIM_TIMX_PWMIN_IRQHandler              TIM8_UP_TIM13_IRQHandler
#define ATIM_TIMX_PWMIN_CHY                     TIM_CHANNEL_1                           /* 通道Y,  1<= Y <=2*/ 
#define ATIM_TIMX_PWMIN_CHY_CLK_ENABLE()        do{ __HAL_RCC_TIM8_CLK_ENABLE(); }while(0)  /* TIM8 时钟使能 */

 /* TIM1 / TIM8 有独立的捕获中断服务函数,需要单独定义,对于TIM2~5等,则不需要以下定义 */
#define ATIM_TIMX_PWMIN_CC_IRQn                 TIM8_CC_IRQn
#define ATIM_TIMX_PWMIN_CC_IRQHandler           TIM8_CC_IRQHandler

/******************************************************************************************/

void atim_timx_pwm_chy_init(uint16_t arr, uint16_t psc);    /* 高级定时器 PWM初始化函数 */
void atim_timx_npwm_chy_init(uint16_t arr, uint16_t psc);   /* 高级定时器 输出指定个数PWM初始化函数 */
void atim_timx_npwm_chy_set(uint32_t npwm);                 /* 高级定时器 设置输出PWM的个数 */
void atim_timx_comp_pwm_init(uint16_t arr, uint16_t psc);   /* 高级定时器 输出比较模式输出PWM 初始化函数 */
void atim_timx_cplm_pwm_init(uint16_t arr, uint16_t psc);   /* 高级定时器 互补输出 初始化函数 */
void atim_timx_cplm_pwm_set(uint16_t ccr, uint8_t dtg);     /* 高级定时器 互补输出 设置输出比较值 & 死区时间 */
void atim_timx_pwmin_chy_init(void);                        /* 高级定时器 PWM输入模式初始化 */
void atim_timx_pwmin_chy_restart(void);                     /* 高级定时器 重启PWM输入模式检测 */

#endif

















