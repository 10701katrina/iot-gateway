/**
 ****************************************************************************************************
 * @file        atim.c
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


#include "./BSP/TIMER/atim.h"
#include "./BSP/LED/led.h"

/***************************************************************************************************/
/* 定时器配置句柄 定义 */

/* 高级定时器PWM */
TIM_HandleTypeDef g_atimx_pwm_chy_handle;     /* 定时器x句柄 */
TIM_OC_InitTypeDef g_atimx_oc_pwm_chy_handle; /* 定时器输出句柄 */

/* g_npwm_remain表示当前还剩下多少个脉冲要发送
 * 每次最多发送256个脉冲
 */
//static uint32_t g_npwm_remain = 0;

/* 高级定时器输出比较模式 */
TIM_HandleTypeDef g_atimx_comp_pwm_handle;                              /* 定时器x句柄 */

/* 互补输出带死区控制 */
TIM_HandleTypeDef g_atimx_cplm_pwm_handle;                              /* 定时器x句柄 */
TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};              /* 死区时间设置 */

/* PWM输入状态(g_timxchy_cap_sta)
 * 0, 没有成功捕获
 * 1, 已经成功捕获了
 */
uint8_t g_atimxchy_pwmin_sta  = 0;   /* PWM输入状态 */
uint16_t g_atimxchy_pwmin_psc  = 0;  /* PWM输入分频系数 */
uint32_t g_atimxchy_pwmin_hval = 0;  /* PWM的高电平脉宽 */
uint32_t g_atimxchy_pwmin_cval = 0;  /* PWM的周期宽度 */
static TIM_HandleTypeDef g_timx_pwmin_chy_handle;                              /* 定时器x句柄 */


/***************************************************************************************************/
/* HAL 通用回调接口函数 */

/**
 * @brief       高级定时器TIMX NPWM中断服务函数
 * @param       无
 * @retval      无
 */
//void ATIM_TIMX_NPWM_IRQHandler(void)
//{
//    HAL_TIM_IRQHandler(&g_atimx_pwm_chy_handle); /* 定时器共用处理函数 */
//}

///**
// * @brief       定时器更新中断回调函数
// * @param       htim:定时器句柄指针
// * @note        此函数会被定时器中断函数共同调用的
// * @retval      无
// */
//void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
//{
//    uint16_t npwm = 0;

//    if (htim == (&g_atimx_pwm_chy_handle))  /* 高级定时器中断实验回调执行的内容 */
//    {
//        if (g_npwm_remain > 256)            /* 还有大于256个脉冲需要发送 */
//        {
//            g_npwm_remain = g_npwm_remain - 256;
//            npwm = 256;
//        }
//        else if (g_npwm_remain % 256)       /* 还有位数（不到256）个脉冲要发送 */
//        {
//            npwm = g_npwm_remain % 256;
//            g_npwm_remain = 0;              /* 没有脉冲了 */
//        }

//        if (npwm)   /* 有脉冲要发送 */
//        {
//            g_atimx_pwm_chy_handle.Instance->RCR = npwm - 1;                        /* 设置重复计数寄存器值为npwm-1, 即npwm个脉冲 */
//            HAL_TIM_GenerateEvent(&g_atimx_pwm_chy_handle, TIM_EVENTSOURCE_UPDATE); /* 产生一次更新事件,在中断里面处理脉冲输出 */
//            __HAL_TIM_ENABLE(&g_atimx_pwm_chy_handle);                              /* 使能定时器TIMX */
//        }
//        else
//        {
//            g_atimx_pwm_chy_handle.Instance->CR1 &= ~(1 << 0); /* 关闭定时器TIMX，使用HAL Disable会清除PWM通道信息，此处不用 */
//        }
//    }
//}

///**
// * @brief       高级定时器TIMX 通道Y 输出指定个数PWM 初始化函数
// * @note
// *              高级定时器的时钟来自APB2, 而PCLK2 = 84Mhz, 我们设置PPRE2不分频, 因此
// *              高级定时器时钟 = 84Mhz
// *              定时器溢出时间计算方法: Tout = ((arr + 1) * (psc + 1)) / Ft us.
// *              Ft=定时器工作频率,单位:Mhz
// *
// * @param       arr: 自动重装值
// * @param       psc: 时钟预分频数
// * @retval      无
// */
//void atim_timx_npwm_chy_init(uint16_t arr, uint16_t psc)
//{
//    ATIM_TIMX_NPWM_CHY_GPIO_CLK_ENABLE();   /* TIMX 通道IO口时钟使能 */
//    ATIM_TIMX_NPWM_CHY_CLK_ENABLE();        /* TIMX 时钟使能 */

//    g_atimx_pwm_chy_handle.Instance = ATIM_TIMX_NPWM;                   /* 定时器x */
//    g_atimx_pwm_chy_handle.Init.Prescaler = psc;                        /* 定时器分频 */
//    g_atimx_pwm_chy_handle.Init.CounterMode = TIM_COUNTERMODE_DOWN;     /* 向下计数模式 */
//    g_atimx_pwm_chy_handle.Init.Period = arr;                           /* 自动重装载值 */
//    g_atimx_pwm_chy_handle.Init.ClockDivision=TIM_CLOCKDIVISION_DIV1;   /* 分频因子 */
//    g_atimx_pwm_chy_handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE; /* 使能TIMx_ARR进行缓冲 */
//    g_atimx_pwm_chy_handle.Init.RepetitionCounter = 0;                  /* 开始时不计数 */
//    HAL_TIM_PWM_Init(&g_atimx_pwm_chy_handle);                          /* 初始化PWM */

//    g_atimx_oc_pwm_chy_handle.OCMode = TIM_OCMODE_PWM1;                 /* 模式选择PWM1 */
//    g_atimx_oc_pwm_chy_handle.Pulse = arr/2;
//    g_atimx_oc_pwm_chy_handle.OCPolarity = TIM_OCPOLARITY_LOW;          /* 输出比较极性为低 */
//    HAL_TIM_PWM_ConfigChannel(&g_atimx_pwm_chy_handle, &g_atimx_oc_pwm_chy_handle, ATIM_TIMX_NPWM_CHY); /* 配置TIMx通道y */
//    HAL_TIM_PWM_Start(&g_atimx_pwm_chy_handle, ATIM_TIMX_NPWM_CHY);     /* 开启对应PWM通道 */
//    __HAL_TIM_ENABLE_IT(&g_atimx_pwm_chy_handle, TIM_IT_UPDATE);        /* 允许更新中断 */

//    HAL_NVIC_SetPriority(ATIM_TIMX_NPWM_IRQn, 1, 3); /* 设置中断优先级，抢占优先级1，子优先级3 */
//    HAL_NVIC_EnableIRQ(ATIM_TIMX_NPWM_IRQn);         /* 开启ITMx中断 */
//}


///**
// * @brief       定时器底层驱动，时钟使能，引脚配置
//                此函数会被HAL_TIM_PWM_Init()调用
// * @param       htim:定时器句柄
// * @retval      无
// */
//void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef *htim)
//{
//    if (htim->Instance == ATIM_TIMX_NPWM)
//    {
//        GPIO_InitTypeDef gpio_init_struct;
//        ATIM_TIMX_NPWM_CHY_GPIO_CLK_ENABLE(); /* 开启通道y的CPIO时钟 */
//        ATIM_TIMX_NPWM_CHY_CLK_ENABLE();

//        gpio_init_struct.Pin = ATIM_TIMX_NPWM_CHY_GPIO_PIN;         /* 通道y的GPIO口 */
//        gpio_init_struct.Mode = GPIO_MODE_AF_PP;                    /* 复用推完输出 */
//        gpio_init_struct.Pull = GPIO_PULLUP;                        /* 上拉 */
//        gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;              /* 高速 */
//        gpio_init_struct.Alternate = ATIM_TIMX_NPWM_CHY_GPIO_AF;    /* 端口复用 */
//        HAL_GPIO_Init(ATIM_TIMX_NPWM_CHY_GPIO_PORT, &gpio_init_struct);
//    }
//}

///**
// * @brief       高级定时器TIMX NPWM设置PWM个数
// * @param       rcr: PWM的个数, 1~2^32次方个
// * @retval      无
// */
//void atim_timx_npwm_chy_set(uint32_t npwm)
//{
//    if (npwm == 0)
//        return;

//    g_npwm_remain = npwm;                                                   /* 保存脉冲个数 */
//    HAL_TIM_GenerateEvent(&g_atimx_pwm_chy_handle, TIM_EVENTSOURCE_UPDATE); /* 产生一次更新事件,在中断里面处理脉冲输出 */
//    __HAL_TIM_ENABLE(&g_atimx_pwm_chy_handle);                              /* 使能定时器TIMX */
//}

/***************************************************************************************************/
/**
 * @brief       高级定时器TIMX 输出比较模式 初始化函数（使用输出比较模式）
 * @note
 *              配置高级定时器TIMX 4路输出比较模式PWM输出,实现50%占空比,不同相位控制
 *              注意,本例程输出比较模式,每2个计数周期才能完成一个PWM输出,因此输出频率减半
 *              另外,我们还可以开启中断在中断里面修改CCRx,从而实现不同频率/不同相位的控制
 *              但是我们不推荐这么使用,因为这可能导致非常频繁的中断,从而占用大量CPU资源
 *
 *              高级定时器的时钟来自APB2, 而PCLK2 = 168Mhz, 我们设置PPRE2不分频, 因此
 *              高级定时器时钟 = 168Mhz
 *              定时器溢出时间计算方法: Tout = ((arr + 1) * (psc + 1)) / Ft us.
 *              Ft=定时器工作频率,单位:Mhz
 *
 * @param       arr: 自动重装值。
 * @param       psc: 时钟预分频数
 * @retval      无
 */
void atim_timx_comp_pwm_init(uint16_t arr, uint16_t psc)
{
    TIM_OC_InitTypeDef sConfigOC = {0};

    g_atimx_comp_pwm_handle.Instance = ATIM_TIMX_COMP;                  /* 定时器8 */
    g_atimx_comp_pwm_handle.Init.Prescaler = psc  ;                     /* 定时器分频 */
    g_atimx_comp_pwm_handle.Init.CounterMode = TIM_COUNTERMODE_UP;      /* 向上计数模式 */
    g_atimx_comp_pwm_handle.Init.Period = arr;                          /* 自动重装载值 */
    g_atimx_comp_pwm_handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;/* 不分频 */
    g_atimx_comp_pwm_handle.Init.RepetitionCounter = 0;                 /* 重复计数器寄存器为0 */
    g_atimx_comp_pwm_handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE; /* 不使能影子寄存器TIMx_ARR */
    HAL_TIM_OC_Init(&g_atimx_comp_pwm_handle);    /* 输出比较模式初始化 */

    sConfigOC.OCMode = TIM_OCMODE_TOGGLE;         /* 比较输出模式 */
    sConfigOC.Pulse = 250;                        /* 设置输出比较寄存器的值 */
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;   /* 输出比较极性为高 */
    HAL_TIM_OC_ConfigChannel(&g_atimx_comp_pwm_handle, &sConfigOC,  TIM_CHANNEL_1); /* 初始化定时器的输出比较通道1 */
    __HAL_TIM_ENABLE_OCxPRELOAD(&g_atimx_comp_pwm_handle, TIM_CHANNEL_1);           /* 通道1 预装载使能 */

    sConfigOC.Pulse = 500;
    HAL_TIM_OC_ConfigChannel(&g_atimx_comp_pwm_handle, &sConfigOC, TIM_CHANNEL_2);  /* 初始化定时器的输出比较通道2 */
    __HAL_TIM_ENABLE_OCxPRELOAD(&g_atimx_comp_pwm_handle, TIM_CHANNEL_2);           /* 通道2 预装载使能 */

    sConfigOC.Pulse = 750;
    HAL_TIM_OC_ConfigChannel(&g_atimx_comp_pwm_handle, &sConfigOC,  TIM_CHANNEL_3); /* 初始化定时器的输出比较通道3 */
    __HAL_TIM_ENABLE_OCxPRELOAD(&g_atimx_comp_pwm_handle, TIM_CHANNEL_3);           /* 通道3 预装载使能 */

    sConfigOC.Pulse = 1000;
    sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
    HAL_TIM_OC_ConfigChannel(&g_atimx_comp_pwm_handle, &sConfigOC,  TIM_CHANNEL_4); /* 初始化定时器的输出比较通道4 */
    __HAL_TIM_ENABLE_OCxPRELOAD(&g_atimx_comp_pwm_handle, TIM_CHANNEL_4);           /* 通道4 预装载使能 */

    HAL_TIM_OC_Start(&g_atimx_comp_pwm_handle,TIM_CHANNEL_1);
    HAL_TIM_OC_Start(&g_atimx_comp_pwm_handle,TIM_CHANNEL_2);
    HAL_TIM_OC_Start(&g_atimx_comp_pwm_handle,TIM_CHANNEL_3);
    HAL_TIM_OC_Start(&g_atimx_comp_pwm_handle,TIM_CHANNEL_4);
}

/**
 * @brief       定时器底层驱动，时钟使能，引脚配置
                此函数会被HAL_TIM_OC_Init()调用
 * @param       htim:定时器句柄
 * @retval      无
 */
void HAL_TIM_OC_MspInit(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == ATIM_TIMX_COMP)
    {
        ATIM_TIMX_COMP_CLK_ENABLE();            /* 使能定时器时钟 */
        GPIO_InitTypeDef gpio_init_struct;
        ATIM_TIMX_COMP_CH1_GPIO_CLK_ENABLE();
        ATIM_TIMX_COMP_CH2_GPIO_CLK_ENABLE();
        ATIM_TIMX_COMP_CH3_GPIO_CLK_ENABLE();
        ATIM_TIMX_COMP_CH4_GPIO_CLK_ENABLE();

        gpio_init_struct.Pin = ATIM_TIMX_COMP_CH1_GPIO_PIN;
        gpio_init_struct.Mode = GPIO_MODE_AF_PP;
        gpio_init_struct.Pull = GPIO_NOPULL;
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
        gpio_init_struct.Alternate = ATIM_TIMX_COMP_GPIO_AF;
        HAL_GPIO_Init(ATIM_TIMX_COMP_CH1_GPIO_PORT, &gpio_init_struct);

        gpio_init_struct.Pin = ATIM_TIMX_COMP_CH2_GPIO_PIN;
        HAL_GPIO_Init(ATIM_TIMX_COMP_CH2_GPIO_PORT, &gpio_init_struct);

        gpio_init_struct.Pin = ATIM_TIMX_COMP_CH3_GPIO_PIN;
        HAL_GPIO_Init(ATIM_TIMX_COMP_CH3_GPIO_PORT, &gpio_init_struct);

        gpio_init_struct.Pin = ATIM_TIMX_COMP_CH4_GPIO_PIN;
        HAL_GPIO_Init(ATIM_TIMX_COMP_CH4_GPIO_PORT, &gpio_init_struct);
    }
}

/*******************************互补输出带死区控制程序**************************************/

/**
 * @brief       高级定时器TIMX 互补输出 初始化函数（使用PWM模式1）
 * @note
 *              配置高级定时器TIMX 互补输出, 一路OCy 一路OCyN, 并且可以设置死区时间
 *
 *              高级定时器的时钟来自APB2, 而PCLK2 = 168Mhz, 我们设置PPRE2不分频, 因此
 *              高级定时器时钟 = 168Mhz
 *              定时器溢出时间计算方法: Tout = ((arr + 1) * (psc + 1)) / Ft us.
 *              Ft=定时器工作频率, 单位 : Mhz
 *
 * @param       arr: 自动重装值。
 * @param       psc: 时钟预分频数
 * @retval      无
 */

void atim_timx_cplm_pwm_init(uint16_t arr, uint16_t psc)
{
    TIM_OC_InitTypeDef sConfigOC = {0};
    
    GPIO_InitTypeDef gpio_init_struct = {0};

    ATIM_TIMX_CPLM_CHY_GPIO_CLK_ENABLE();   /* 通道X对应IO口时钟使能 */
    ATIM_TIMX_CPLM_CHYN_GPIO_CLK_ENABLE();  /* 通道X互补通道对应IO口时钟使能 */
    ATIM_TIMX_CPLM_BKIN_GPIO_CLK_ENABLE();  /* 通道X刹车输入对应IO口时钟使能 */
  
    gpio_init_struct.Pin = ATIM_TIMX_CPLM_BKIN_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_AF_PP;
    gpio_init_struct.Pull = GPIO_PULLUP;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH ;
    gpio_init_struct.Alternate = ATIM_TIMX_CPLM_CHY_GPIO_AF;       /* 端口复用 */
    HAL_GPIO_Init(ATIM_TIMX_CPLM_BKIN_GPIO_PORT, &gpio_init_struct);

    gpio_init_struct.Pin = ATIM_TIMX_CPLM_CHY_GPIO_PIN;
    HAL_GPIO_Init(ATIM_TIMX_CPLM_CHY_GPIO_PORT, &gpio_init_struct);

    gpio_init_struct.Pin = ATIM_TIMX_CPLM_CHYN_GPIO_PIN;
    HAL_GPIO_Init(ATIM_TIMX_CPLM_CHYN_GPIO_PORT, &gpio_init_struct);

    ATIM_TIMX_CPLM_CLK_ENABLE();

    g_atimx_cplm_pwm_handle.Instance = ATIM_TIMX_CPLM;                    /* 定时器x */
    g_atimx_cplm_pwm_handle.Init.Prescaler = psc;                         /* 定时器预分频系数 */
    g_atimx_cplm_pwm_handle.Init.CounterMode = TIM_COUNTERMODE_UP;        /* 向上计数模式 */
    g_atimx_cplm_pwm_handle.Init.Period = arr;                            /* 自动重装载值 */
    g_atimx_cplm_pwm_handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;  /* 时钟分频因子 */
    g_atimx_cplm_pwm_handle.Init.RepetitionCounter = 0;                   /* 重复计数器寄存器为0 */
    g_atimx_cplm_pwm_handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;         /* 使能影子寄存器TIMx_ARR */
    HAL_TIM_PWM_Init(&g_atimx_cplm_pwm_handle) ;

    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_LOW;      /* OCy 低电平有效 */
    sConfigOC.OCNPolarity = TIM_OCNPOLARITY_LOW;    /* OCyN 低电平有效 */
    HAL_TIM_PWM_ConfigChannel(&g_atimx_cplm_pwm_handle, &sConfigOC, ATIM_TIMX_CPLM_CHY);    /* 配置后默认清CCER的互补输出位 */

    /* 设置死区参数，开启死区中断 */
    sBreakDeadTimeConfig.BreakState = TIM_BREAK_ENABLE;                     /* BKE = 1, 使能BKIN检测 */
    sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;                     /* 上电只能写一次，需要更新死区时间时只能用此值 */
    sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_LOW;             /* BKP = 1, BKIN高电平有效 */
    sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_ENABLE;      /* 使能AOE位，允许刹车后自动恢复输出 */
    HAL_TIMEx_ConfigBreakDeadTime(&g_atimx_cplm_pwm_handle, &sBreakDeadTimeConfig);     /* 设置BDTR寄存器 */

    HAL_TIM_PWM_Start(&g_atimx_cplm_pwm_handle, ATIM_TIMX_CPLM_CHY);        /* OCy 输出使能 */
    HAL_TIMEx_PWMN_Start(&g_atimx_cplm_pwm_handle, ATIM_TIMX_CPLM_CHY);     /* OCyN 输出使能 */

}

/**
 * @brief       定时器TIMX 设置输出比较值 & 死区时间
 * @param       ccr: 输出比较值
 * @param       dtg: 死区时间
 *   @arg       dtg[7:5]=0xx时, 死区时间 = dtg[7:0] * tDTS
 *   @arg       dtg[7:5]=10x时, 死区时间 = (64 + dtg[6:0]) * 2  * tDTS
 *   @arg       dtg[7:5]=110时, 死区时间 = (32 + dtg[5:0]) * 8  * tDTS
 *   @arg       dtg[7:5]=111时, 死区时间 = (32 + dtg[5:0]) * 16 * tDTS
 *   @note      tDTS = 1 / (Ft /  CKD[1:0]) = 1 / 18M = 55.56ns
 * @retval      无
 */
void atim_timx_cplm_pwm_set(uint16_t ccr, uint8_t dtg)
{
    sBreakDeadTimeConfig.DeadTime = dtg;
    HAL_TIMEx_ConfigBreakDeadTime(&g_atimx_cplm_pwm_handle, &sBreakDeadTimeConfig);      /*重设死区时间*/
    __HAL_TIM_MOE_ENABLE(&g_atimx_cplm_pwm_handle);     /* MOE=1,使能主输出 */    
    ATIM_TIMX_CPLM_CHY_CCRY = ccr;      /* 设置比较寄存器 */

}


/*******************************高级定时器PWM输入模式程序**************************************/

/**
 * @brief       定时器TIMX 通道Y PWM输入模式 初始化函数
 * @note
 *              高级定时器的时钟来自APB2, 而PCLK2 = 72Mhz, 我们设置PPRE2不分频, 因此
 *              高级定时器时钟 = 72Mhz
 *              定时器溢出时间计算方法: Tout = ((arr + 1) * (psc + 1)) / Ft us.
 *              Ft=定时器工作频率,单位:Mhz
 *
 *              本函数初始化的时候: 使用psc=0, arr固定为65535. 得到采样时钟频率为72Mhz,精度约13.8ns
 *
 * @param       无
 * @retval      无
 */
void atim_timx_pwmin_chy_init(void)
{
    ATIM_TIMX_PWMIN_CHY_GPIO_CLK_ENABLE();

    GPIO_InitTypeDef gpio_init_struct = {0};

    gpio_init_struct.Pin = ATIM_TIMX_PWMIN_CHY_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_AF_PP; 
    gpio_init_struct.Pull = GPIO_PULLDOWN;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
    gpio_init_struct.Alternate = ATIM_TIMX_PWMIN_CHY_GPIO_AF;       /* 端口复用 */

    HAL_GPIO_Init(ATIM_TIMX_PWMIN_CHY_GPIO_PORT, &gpio_init_struct);
    
    TIM_SlaveConfigTypeDef sSlaveConfig = {0};
    TIM_IC_InitTypeDef sConfigIC = {0};
    
    ATIM_TIMX_PWMIN_CHY_CLK_ENABLE();
    
    g_timx_pwmin_chy_handle.Instance = ATIM_TIMX_PWMIN;           /* 定时器8 */
    g_timx_pwmin_chy_handle.Init.Prescaler = 0;                   /* 定时器预分频系数 */
    g_timx_pwmin_chy_handle.Init.CounterMode = TIM_COUNTERMODE_UP;/*向上计数模式*/
    g_timx_pwmin_chy_handle.Init.Period = 65535;                  /* 自动重装载值 */
    g_timx_pwmin_chy_handle.Init.ClockDivision =TIM_CLOCKDIVISION_DIV1;/*不分频*/
    g_timx_pwmin_chy_handle.Init.RepetitionCounter = 0;           /*重复计数器寄存器为0*/
    g_timx_pwmin_chy_handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE; /* 使能影子寄存器TIMx_ARR */
    HAL_TIM_IC_Init(&g_timx_pwmin_chy_handle);
    
    /* 从模式配置，IT1触发更新 */
    sSlaveConfig.SlaveMode = TIM_SLAVEMODE_RESET;/* 从模式：复位模式 */
    sSlaveConfig.InputTrigger = TIM_TS_TI1FP1;   /* 定时器输入触发源：TI1FP1 */
    sSlaveConfig.TriggerPolarity = TIM_INPUTCHANNELPOLARITY_RISING; /*上升沿检测*/
    sSlaveConfig.TriggerFilter = 0;              /* 不滤波 */
    HAL_TIM_SlaveConfigSynchro(&g_timx_pwmin_chy_handle, &sSlaveConfig);

    /* IC1捕获：上升沿触发TI1FP1 */
    sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;/* 上升沿检测 */
    sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;      /*选择输入端IC1映射到TI1*/
    sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;                /* 不分频 */
    sConfigIC.ICFilter = 0;                                /* 不滤波 */
    HAL_TIM_IC_ConfigChannel( &g_timx_pwmin_chy_handle, &sConfigIC, TIM_CHANNEL_1 );
    
    /* IC2捕获：上升沿触发TI1FP2 */
    sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_FALLING; /* 下降沿检测 */
    sConfigIC.ICSelection = TIM_ICSELECTION_INDIRECTTI;      /*选择输入端IC2映射到TI1*/
    HAL_TIM_IC_ConfigChannel(&g_timx_pwmin_chy_handle,&sConfigIC,TIM_CHANNEL_2);
    
    HAL_NVIC_SetPriority(ATIM_TIMX_PWMIN_IRQn, 1, 3);   /* 设置中断优先级，抢占优先级1，子优先级3 */
    HAL_NVIC_EnableIRQ( ATIM_TIMX_PWMIN_IRQn );         /* 开启TIMx中断 */
    
    /* TIM1/TIM8有独立的输入捕获中断服务函数 */
    if ( ATIM_TIMX_PWMIN == TIM1 || ATIM_TIMX_PWMIN == TIM8)
    {
        HAL_NVIC_SetPriority(ATIM_TIMX_PWMIN_CC_IRQn, 1, 3); /* 设置中断优先级，抢占优先级1，子优先级3 */
        HAL_NVIC_EnableIRQ(ATIM_TIMX_PWMIN_CC_IRQn);         /* 开启TIMx中断 */
    }
    
    HAL_TIM_IC_Start_IT(&g_timx_pwmin_chy_handle, TIM_CHANNEL_1);
    HAL_TIM_IC_Start_IT(&g_timx_pwmin_chy_handle, TIM_CHANNEL_2);
}

/**
 * @brief       定时器TIMX 更新/溢出 中断服务函数
 *   @note      TIM1/TIM8的这个函数仅用于更新/溢出中断服务,捕获在另外一个函数!
 *              其他普通定时器则更新/溢出/捕获,都在这个函数里面处理!
 * @param       无
 * @retval      无
 */
void ATIM_TIMX_PWMIN_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&g_timx_pwmin_chy_handle);/* 定时器共用处理函数 */
}

/**
 * @brief       定时器TIMX 输入捕获 中断服务函数
 *   @note      仅TIM1/TIM8有这个函数,其他普通定时器没有这个中断服务函数!
 * @param       无
 * @retval      无
 */
void ATIM_TIMX_PWMIN_CC_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&g_timx_pwmin_chy_handle);/* 定时器共用处理函数 */
}

/**
 * @brief       定时器TIMX PWM输入模式 重新启动捕获
 * @param       无
 * @retval      无
 */
void atim_timx_pwmin_chy_restart(void)
{
    sys_intx_disable();                     /* 关闭中断 */

    g_atimxchy_pwmin_sta = 0;                /* 清零状态,重新开始检测 */
    g_atimxchy_pwmin_hval=0;
    g_atimxchy_pwmin_cval=0;

    sys_intx_enable();                      /* 打开中断 */
}

/**
 * @brief       定时器输入捕获中断处理回调函数
 * @param       htim:定时器句柄指针
 * @note        该函数在HAL_TIM_IRQHandler中会被调用
 *              此函数是定时器共同调用的回调函数，为了防止重定义错误，
 *              我们把gtim.c的HAL_TIM_IC_CaptureCallback()函数先屏蔽掉.
 * @retval      无
 */
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    
    if (g_atimxchy_pwmin_sta == 0)   /* 还没有成功捕获 */
    {
        if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1)
        {  
            g_atimxchy_pwmin_hval = HAL_TIM_ReadCapturedValue(&g_timx_pwmin_chy_handle,TIM_CHANNEL_2)+2; /* 修正系数为2, 加1 */
            g_atimxchy_pwmin_cval = HAL_TIM_ReadCapturedValue(&g_timx_pwmin_chy_handle,TIM_CHANNEL_1)+2; /* 修正系数为2, 加1 */
            g_atimxchy_pwmin_sta = 1;        /* 标记捕获成功 */
        }
    }
}

