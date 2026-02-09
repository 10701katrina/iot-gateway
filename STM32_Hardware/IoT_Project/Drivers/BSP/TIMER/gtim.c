/**
 ****************************************************************************************************
 * @file        gtim.c
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

#include "./BSP/TIMER/gtim.h"
#include "./BSP/LED/led.h"

/**************************************************************************************************/

/* 定时器配置句柄 定义 */
TIM_HandleTypeDef g_timx_handle;                /* 定时器x句柄 */

/*定时器PWM*/
TIM_HandleTypeDef g_timx_pwm_chy_handle;        /* 定时器x句柄 */
TIM_OC_InitTypeDef g_timx_oc_pwm_chy_handle;    /* 定时器输出句柄 */
uint8_t g_timxchy_cap_sta = 0;                  /* 输入捕获状态 */
uint16_t g_timxchy_cap_val = 0;                 /* 输入捕获值 */

/*定时器输入边沿捕获*/
TIM_HandleTypeDef g_timx_cap_chy_handler;       /* 定时器x句柄 */
TIM_IC_InitTypeDef g_timx_ic_cap_chy_handler;

/*定时器输入计数捕获*/
/**********************************通用定时器脉冲计数实验程序**************************************/
TIM_HandleTypeDef g_timx_cnt_chy_handler; /* 定时器x句柄 */
/* 记录定时器计数器的溢出次数, 方便计算总脉冲个数 */
uint32_t g_timxchy_cnt_ofcnt = 0; /* 计数溢出次数 */

/**************************************************************************************************/
/* HAL 通用回调接口函数 */

/**************************************************************************************************/
/**
 * @brief       通用定时器输入捕获初始化接口
                HAL库调用的接口，用于配置不同的输入捕获
 * @param       htim:定时器句柄
 * @note        此函数会被gtim_timx_cap_chy_init()调用
 * @retval      无
 */
void HAL_TIM_IC_MspInit(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == GTIM_TIMX_CAP)                            /* 输入通道捕获 */
    {
        GPIO_InitTypeDef gpio_init_struct;
        GTIM_TIMX_CAP_CHY_CLK_ENABLE();                             /* 使能TIMx时钟 */
        GTIM_TIMX_CAP_CHY_GPIO_CLK_ENABLE();                        /* 开启捕获IO的时钟 */

        gpio_init_struct.Pin = GTIM_TIMX_CAP_CHY_GPIO_PIN;          /* 输入捕获的GPIO口 */
        gpio_init_struct.Mode = GPIO_MODE_AF_PP;                    /* 复用推挽输出 */
        gpio_init_struct.Pull = GPIO_PULLDOWN;                      /* 下拉 */
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;              /* 高速 */
        gpio_init_struct.Alternate = GTIM_TIMX_CAP_CHY_GPIO_AF;     /* 端口复用 */ 
        HAL_GPIO_Init(GTIM_TIMX_CAP_CHY_GPIO_PORT, &gpio_init_struct);

        HAL_NVIC_SetPriority(GTIM_TIMX_CAP_IRQn, 2, 0);             /* 抢占1，子优先级3 */
        HAL_NVIC_EnableIRQ(GTIM_TIMX_CAP_IRQn);                     /* 开启ITMx中断 */
    }
    else if (htim->Instance == GTIM_TIMX_CNT)              /*输入计数捕获*/
    {
        GPIO_InitTypeDef gpio_init_struct;
        TIM_SlaveConfigTypeDef tim_slave_config_handle = {0};
        TIM_IC_InitTypeDef timx_ic_cnt_chy_handler = {0};
        GTIM_TIMX_CNT_CHY_CLK_ENABLE();      /* 使能TIMx时钟 */
        GTIM_TIMX_CNT_CHY_GPIO_CLK_ENABLE(); /* 开启GPIOA时钟 */

        gpio_init_struct.Pin = GTIM_TIMX_CNT_CHY_GPIO_PIN; /* 输入捕获的GPIO口 */
        gpio_init_struct.Mode = GPIO_MODE_AF_PP;           /* 复用推挽输出 */
        gpio_init_struct.Pull = GPIO_PULLDOWN;             /* 下拉 */
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;     /* 高速 */
        HAL_GPIO_Init(GTIM_TIMX_CNT_CHY_GPIO_PORT, &gpio_init_struct);

        /* 从模式：外部触发模式1 */
        tim_slave_config_handle.SlaveMode = TIM_SLAVEMODE_EXTERNAL1;                                    /* 从模式：外部触发模式1 */
        tim_slave_config_handle.InputTrigger = TIM_TS_TI1FP1;                                           /* 输入触发：选择 TI1FP1(TIMX_CH1) 作为输入源 */
        tim_slave_config_handle.TriggerPolarity = TIM_TRIGGERPOLARITY_RISING;                           /* 触发极性：上升沿 */
        tim_slave_config_handle.TriggerPrescaler = TIM_TRIGGERPRESCALER_DIV1;                           /* 触发预分频：无 */
        tim_slave_config_handle.TriggerFilter = 0;                                                    /* 滤波：本例中不需要任何滤波 */
        HAL_TIM_SlaveConfigSynchro(&g_timx_cnt_chy_handler, &tim_slave_config_handle);

        /* 配置输入捕获模式 */
        timx_ic_cnt_chy_handler.ICPolarity = TIM_ICPOLARITY_RISING;                                     /* 上升沿捕获 */
        timx_ic_cnt_chy_handler.ICSelection = TIM_ICSELECTION_DIRECTTI;                                 /* 映射到TI1上 */
        timx_ic_cnt_chy_handler.ICPrescaler = TIM_ICPSC_DIV1;                                           /* 配置输入分频，不分频 */
        timx_ic_cnt_chy_handler.ICFilter = 0;                                                           /* 配置输入滤波器，不滤波 */
        HAL_TIM_IC_ConfigChannel(&g_timx_cnt_chy_handler, &timx_ic_cnt_chy_handler, GTIM_TIMX_CNT_CHY); /* 配置TIMx通道y */
        HAL_TIM_IC_Start(&g_timx_cnt_chy_handler, GTIM_TIMX_CNT_CHY);                                /* 开始捕获TIMx的通道y */

        HAL_NVIC_SetPriority(GTIM_TIMX_CNT_IRQn, 1, 3); /* 设置中断优先级，抢占优先级1，子优先级3 */
        HAL_NVIC_EnableIRQ(GTIM_TIMX_CNT_IRQn);
    }
}

/**
 * @brief       定时器更新中断回调函数
 * @param        htim:定时器句柄指针
 * @note        此函数会被定时器中断函数共同调用的
 * @retval      无
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim == (&g_timx_handle)) /* 通用定时器中断实验回调执行的内容 */
    {
        LED1_TOGGLE();
    }

    /**********************************输入捕获实验处理代码********************************/
    if (htim == (&g_timx_cap_chy_handler))
    {
        if ((g_timxchy_cap_sta & 0X80) == 0)            /* 还未成功捕获 */
        {
            if (g_timxchy_cap_sta & 0X40)               /* 已经捕获到高电平了 */
            {
                if ((g_timxchy_cap_sta & 0X3F) == 0X3F) /* 高电平太长了 */
                {
                    g_timxchy_cap_sta |= 0X80;          /* 标记成功捕获了一次 */
                    g_timxchy_cap_val = 0XFFFF;
                }
                else
                {
                    g_timxchy_cap_sta++;
                }
            }
        }
    }
     /**********************************脉冲计数实验处理代码********************************/
    if (htim == (&g_timx_cnt_chy_handler))
    {
        g_timxchy_cnt_ofcnt++;                         /* 累计溢出次数 */
    }
}

/**
 * @brief       通用定时器TIMX定时中断初始化函数
 * @note
 *              通用定时器的时钟来自APB1,当PPRE1 ≥ 2分频的时候
 *              通用定时器的时钟为APB1时钟的2倍, 而APB1为36M, 所以定时器时钟 = 72Mhz
 *              定时器溢出时间计算方法: Tout = ((arr + 1) * (psc + 1)) / Ft us.
 *              Ft=定时器工作频率,单位:Mhz
 *
 * @param       arr: 自动重装值。
 * @param       psc: 时钟预分频数
 * @retval      无
 */
void gtim_timx_int_init(uint16_t arr, uint16_t psc)
{
    GTIM_TIMX_INT_CLK_ENABLE(); /* 使能TIMx时钟 */

    g_timx_handle.Instance = GTIM_TIMX_INT;                     /* 通用定时器x */
    g_timx_handle.Init.Prescaler = psc;                         /* 分频 */
    g_timx_handle.Init.CounterMode = TIM_COUNTERMODE_UP;        /* 向上计数器 */
    g_timx_handle.Init.Period = arr;                            /* 自动装载值 */
    g_timx_handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;  /* 时钟分频因子 */
    HAL_TIM_Base_Init(&g_timx_handle);

    HAL_NVIC_SetPriority(GTIM_TIMX_INT_IRQn, 1, 3);             /* 设置中断优先级，抢占优先级1，子优先级3 */
    HAL_NVIC_EnableIRQ(GTIM_TIMX_INT_IRQn);                     /* 开启ITMx中断 */

    HAL_TIM_Base_Start_IT(&g_timx_handle);                      /* 使能定时器x和定时器x更新中断 */
}

/**
 * @brief       定时器中断服务函数
 * @param       无
 * @retval      无
 */
void GTIM_TIMX_INT_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&g_timx_handle);
}

/*********************************通用定时器PWM输出程序*************************************/
/**
 * @brief       通用定时器TIMX 通道Y PWM输出 初始化函数（使用PWM模式1）
 * @note
 *              通用定时器的时钟来自APB1,当PPRE1 ≥ 2分频的时候
 *              通用定时器的时钟为APB1时钟的2倍, 而APB1为36M, 所以定时器时钟 = 72Mhz
 *              定时器溢出时间计算方法: Tout = ((arr + 1) * (psc + 1)) / Ft us.
 *              Ft=定时器工作频率,单位:Mhz
 *
 * @param       arr: 自动重装值。
 * @param       psc: 时钟预分频数
 * @retval      无
 */
void gtim_timx_pwm_chy_init(uint16_t arr, uint16_t psc)
{

    g_timx_pwm_chy_handle.Instance = GTIM_TIMX_PWM;                    /* 定时器x */
    g_timx_pwm_chy_handle.Init.Prescaler = psc;                        /* 定时器分频 */
    g_timx_pwm_chy_handle.Init.CounterMode = TIM_COUNTERMODE_UP;       /* 向上计数模式 */
    g_timx_pwm_chy_handle.Init.Period = arr;                           /* 自动重装载值 */
    g_timx_pwm_chy_handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1; /* 时钟分频因子 */
    HAL_TIM_PWM_Init(&g_timx_pwm_chy_handle);                          /* 初始化PWM */

    g_timx_oc_pwm_chy_handle.OCMode = TIM_OCMODE_PWM1;                                               /* 模式选择PWM1 */
    g_timx_oc_pwm_chy_handle.Pulse = arr / 2;                                                        /* 设置比较值,此值用来确定占空比 */
                                                                                                     /* 默认比较值为自动重装载值的一半,即占空比为50% */
    g_timx_oc_pwm_chy_handle.OCPolarity = TIM_OCPOLARITY_LOW;                                        /* 输出比较极性为低 */
    HAL_TIM_PWM_ConfigChannel(&g_timx_pwm_chy_handle, &g_timx_oc_pwm_chy_handle, GTIM_TIMX_PWM_CHY); /* 配置TIMx通道y */
    HAL_TIM_PWM_Start(&g_timx_pwm_chy_handle, GTIM_TIMX_PWM_CHY);                                    /* 开启对应PWM通道 */
}

/**
 * @brief       定时器底层驱动，时钟使能，引脚配置
                此函数会被HAL_TIM_PWM_Init()调用
 * @param       htim:定时器句柄
 * @retval      无
 */
void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == GTIM_TIMX_PWM)
    {
        GPIO_InitTypeDef gpio_init_struct;
        GTIM_TIMX_PWM_CHY_GPIO_CLK_ENABLE();               /* 开启通道y的CPIO时钟 */
        GTIM_TIMX_PWM_CHY_CLK_ENABLE();

        gpio_init_struct.Pin = GTIM_TIMX_PWM_CHY_GPIO_PIN; /* 通道y的CPIO口 */
        gpio_init_struct.Mode = GPIO_MODE_AF_PP;           /* 复用推完输出 */
        gpio_init_struct.Pull = GPIO_PULLUP;               /* 上拉 */
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;     /* 高速 */
        gpio_init_struct.Alternate = GTIM_TIMX_PWM_CHY_GPIO_AF; /* 端口复用 */ 
        HAL_GPIO_Init(GTIM_TIMX_PWM_CHY_GPIO_PORT, &gpio_init_struct);
    }
}

/**
 * @brief       设置TIM通道1的占空比
 * @param       compare:比较值
 * @retval      无
 */
void gtim_timx_pwm_setcompare(uint32_t compare)
{
    GTIM_TIMX_PWM_CHY_CCRX = compare;
}

uint32_t get_timx_capture(void)
{
    return HAL_TIM_ReadCapturedValue(&g_timx_pwm_chy_handle, GTIM_TIMX_PWM_CHY);
}

/*********************************通用定时器输入捕获实验程序*************************************/
/**
 * @brief       通用定时器TIMX 通道Y 输入捕获 初始化函数
 * @note
 *              通用定时器的时钟来自APB1,当PPRE1 ≥ 2分频的时候
 *              通用定时器的时钟为APB1时钟的2倍, 而APB1为42M, 所以定时器时钟 = 84Mhz
 *              定时器溢出时间计算方法: Tout = ((arr + 1) * (psc + 1)) / Ft us.
 *              Ft=定时器工作频率,单位:Mhz
 *
 * @param       arr: 自动重装值
 * @param       psc: 时钟预分频数
 * @retval      无
 */
void gtim_timx_cap_chy_init(uint32_t arr, uint16_t psc)
{
    g_timx_cap_chy_handler.Instance = GTIM_TIMX_CAP;                    /* 定时器5 */
    g_timx_cap_chy_handler.Init.Prescaler = psc;                        /* 定时器分频 */
    g_timx_cap_chy_handler.Init.CounterMode = TIM_COUNTERMODE_UP;       /* 向上计数模式 */
    g_timx_cap_chy_handler.Init.Period = arr;                           /* 自动重装载值 */
    g_timx_cap_chy_handler.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1; /* 时钟分频因子 */
    HAL_TIM_IC_Init(&g_timx_cap_chy_handler);                           /* 初始化定时器 */
    
    g_timx_ic_cap_chy_handler.ICPolarity = TIM_ICPOLARITY_RISING;                                     /* 上升沿捕获 */
    g_timx_ic_cap_chy_handler.ICSelection = TIM_ICSELECTION_DIRECTTI;                                 /* 映射到TI1上 */
    g_timx_ic_cap_chy_handler.ICPrescaler = TIM_ICPSC_DIV1;                                           /* 配置输入分频，不分频 */
    g_timx_ic_cap_chy_handler.ICFilter = 0;                                                           /* 配置输入滤波器，不滤波 */
    HAL_TIM_IC_ConfigChannel(&g_timx_cap_chy_handler, &g_timx_ic_cap_chy_handler, GTIM_TIMX_CAP_CHY); /* 配置TIM5通道1 */

    HAL_TIM_IC_Start_IT(&g_timx_cap_chy_handler, GTIM_TIMX_CAP_CHY);                                  /* 开始捕获TIM5的通道1 */
    __HAL_TIM_ENABLE_IT(&g_timx_cap_chy_handler, TIM_IT_UPDATE);                                      /* 使能更新中断 */
}

/* 输入捕获状态(g_timxchy_cap_sta)
 * [7]  :0,没有成功的捕获;1,成功捕获到一次.
 * [6]  :0,还没捕获到高电平;1,已经捕获到高电平了.
 * [5:0]:捕获高电平后溢出的次数,最多溢出63次,所以最长捕获值 = 63*65536 + 65535 = 4194303
 *       注意:为了通用,我们默认ARR和CCRy都是16位寄存器,对于32位的定时器(如:TIM5),也只按16位使用
 *       按1us的计数频率,最长溢出时间为:4194303 us, 约4.19秒
 *
 *      (说明一下：正常32位定时器来说,1us计数器加1,溢出时间:4294秒)
 */

/**
 * @brief       定时器中断服务函数
 * @param       无
 * @retval      无
 */
void GTIM_TIMX_CAP_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&g_timx_cap_chy_handler);        /* 定时器共用处理函数 */
}

/**
 * @brief       定时器输入捕获中断处理回调函数
 * @param       htim:定时器句柄指针
 * @note        该函数在HAL_TIM_IRQHandler中会被调用
 * @retval      无
 */
//void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
//{
//    if ((g_timxchy_cap_sta & 0X80) == 0)                /* 还未成功捕获 */
//    {
//        if (g_timxchy_cap_sta & 0X40)                   /* 捕获到一个下降沿 */
//        {
//            g_timxchy_cap_sta |= 0X80;                                                                 /* 标记成功捕获到一次高电平脉宽 */
//            g_timxchy_cap_val = HAL_TIM_ReadCapturedValue(&g_timx_cap_chy_handler, GTIM_TIMX_CAP_CHY); /* 获取当前的捕获值 */
//            //TIM_RESET_CAPTUREPOLARITY(&g_timx_cap_chy_handler, GTIM_TIMX_CAP_CHY);                   /* 一定要先清除原来的设置 */
//            g_timx_cap_chy_handler.Instance->CCER &= ~(TIM_CCER_CC1P | TIM_CCER_CC1NP);
//            TIM_SET_CAPTUREPOLARITY(&g_timx_cap_chy_handler, GTIM_TIMX_CAP_CHY, TIM_ICPOLARITY_RISING); /* 配置TIM5通道1上升沿捕获 */
//        }
//        else                                            /* 还未开始,第一次捕获上升沿 */
//        {
//            g_timxchy_cap_sta = 0;                      /* 清空 */
//            g_timxchy_cap_val = 0;
//            g_timxchy_cap_sta |= 0X40;                  /* 标记捕获到了上升沿 */
//            __HAL_TIM_DISABLE(&g_timx_cap_chy_handler); /* 关闭定时器5 */
//            __HAL_TIM_SET_COUNTER(&g_timx_cap_chy_handler, 0);
//            //TIM_RESET_CAPTUREPOLARITY(&g_timx_cap_chy_handler, GTIM_TIMX_CAP_CHY);                     /* 一定要先清除原来的设置！！ */
//            g_timx_cap_chy_handler.Instance->CCER &= ~(TIM_CCER_CC1P | TIM_CCER_CC1NP);
//            TIM_SET_CAPTUREPOLARITY(&g_timx_cap_chy_handler, GTIM_TIMX_CAP_CHY, TIM_ICPOLARITY_FALLING); /* 定时器5通道1设置为下降沿捕获 */
//            __HAL_TIM_ENABLE(&g_timx_cap_chy_handler);                                                   /* 使能定时器5 */
//        }
//    }
//}


/*********************************通用定时器脉冲计数实验程序*************************************/
/**
 * @brief       通用定时器TIMX 通道Y 脉冲计数 初始化函数
 * @note
 *              本函数选择通用定时器的时钟选择: 外部时钟源模式1(SMS[2:0] = 111)
 *              这样CNT的计数时钟源就来自 TIMX_CH1/CH2, 可以实现外部脉冲计数(脉冲接入CH1/CH2)
 *
 *              时钟分频数 = psc, 一般设置为0, 表示每一个时钟都会计数一次, 以提高精度.
 *              通过读取CNT和溢出次数, 经过简单计算, 可以得到当前的计数值, 从而实现脉冲计数
 *
 * @param       arr: 自动重装值 
 * @retval      无
 */
void gtim_timx_cnt_chy_init(uint16_t psc)
{
    g_timx_cnt_chy_handler.Instance = GTIM_TIMX_CNT;                    /* 定时器x */
    g_timx_cnt_chy_handler.Init.Prescaler = psc;                        /* 定时器分频 */
    g_timx_cnt_chy_handler.Init.CounterMode = TIM_COUNTERMODE_UP;       /* 向上计数模式 */
    g_timx_cnt_chy_handler.Init.Period = 65535;                         /* 自动重装载值 */
    g_timx_cnt_chy_handler.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1; /* 时钟分频因子 */
    g_timx_cnt_chy_handler.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_IC_Init(&g_timx_cnt_chy_handler);
}

/**
 * @brief       通用定时器TIMX 通道Y 获取当前计数值 
 * @param       无
 * @retval      当前计数值
 */
uint32_t gtim_timx_cnt_chy_get_count(void)
{
    uint32_t count = 0;
    count = g_timxchy_cnt_ofcnt * 65536;                     /* 计算溢出次数对应的计数值 */
    count += __HAL_TIM_GET_COUNTER(&g_timx_cnt_chy_handler); /* 加上当前CNT的值 */
    return count;
}

/**
 * @brief       通用定时器TIMX 通道Y 重启计数器
 * @param       无
 * @retval      当前计数值
 */
void gtim_timx_cnt_chy_restart(void)
{
    __HAL_TIM_DISABLE(&g_timx_cnt_chy_handler);        /* 关闭定时器TIMX */
    g_timxchy_cnt_ofcnt = 0;                           /* 累加器清零 */
    __HAL_TIM_SET_COUNTER(&g_timx_cnt_chy_handler, 0); /* 计数器清零 */
    __HAL_TIM_ENABLE(&g_timx_cnt_chy_handler);         /* 使能定时器TIMX */
}

/**
 * @brief       通用定时器TIMX 脉冲计数 更新中断服务函数
 * @param       无
 * @retval      无
 */
void GTIM_TIMX_CNT_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&g_timx_cnt_chy_handler); /* 定时器共用处理函数 */
}

