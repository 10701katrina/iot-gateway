/**
 ****************************************************************************************************
 * @file        adc3.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2021-10-23
 * @brief       ADC3 驱动代码
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:正点原子 探索者 F407开发板
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 * 公司网址:www.alientek.com
 * 购买地址:openedv.taobao.com
 *
 * 修改说明
 * V1.0 20211023
 * 第一次发布
 ****************************************************************************************************
 */

#include "./BSP/ADC/adc3.h"
#include "./SYSTEM/delay/delay.h"


ADC_HandleTypeDef g_adc3_handle;        /* ADC句柄 */

/********************************************************************/
/**
 * @brief       ADC3初始化函数
 *   @note      本函数支持ADC1/ADC2任意通道, 但是不支持ADC3
 *              我们使用12位精度, ADC采样时钟=21M, 转换时间为: 采样周期 + 12.5个ADC周期
 *              设置最大采样周期: 239.5, 则转换时间 = 252 个ADC周期 = 21us
 * @param       无
 * @retval      无
 */
void adc3_init(void)
{
    ADC3_CHY_CLK_ENABLE();  /* ADC时钟使能 */

    g_adc3_handle.Instance = ADC_ADCX;
    g_adc3_handle.Init.ClockPrescaler = ADC_CLOCKPRESCALER_PCLK_DIV4;   /* 4分频，ADCCLK=PCLK2/4=84/4=21MHz */
    g_adc3_handle.Init.Resolution = ADC_RESOLUTION12b;          /* 12位模式 */
    g_adc3_handle.Init.DataAlign = ADC_DATAALIGN_RIGHT;         /* 右对齐 */
    g_adc3_handle.Init.ScanConvMode = DISABLE;                  /* 非扫描模式 */
    g_adc3_handle.Init.EOCSelection = DISABLE;                  /* 关闭EOC中断 */

    g_adc3_handle.Init.ContinuousConvMode = DISABLE;            /* 关闭连续转换 */
    g_adc3_handle.Init.NbrOfConversion = 1;                     /* 1个转换在规则序列中 也就是只转换规则序列1 */
    g_adc3_handle.Init.DiscontinuousConvMode = DISABLE;         /* 禁止不连续采样模式 */
    g_adc3_handle.Init.NbrOfDiscConversion = 0;                 /* 不连续采样通道数为0 */
    g_adc3_handle.Init.ExternalTrigConv = ADC_SOFTWARE_START;   /* 软件触发 */
    g_adc3_handle.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;    /* 使用软件触发 */
    HAL_ADC_Init(&g_adc3_handle);   /* 初始化 */
}

/**
 * @brief       设置ADC通道采样时间
 * @param       adcx : adc句柄指针,ADC_HandleTypeDef
 * @param       ch   : 通道号, ADC_CHANNEL_0~ADC_CHANNEL_17
 * @param       stime: 采样时间  0~7, 对应关系为:
 *   @arg       ADC_SAMPLETIME_3CYCLES,  3个ADC时钟周期        ADC_SAMPLETIME_15CYCLES, 15个ADC时钟周期
 *   @arg       ADC_SAMPLETIME_28CYCLES, 28个ADC时钟周期       ADC_SAMPLETIME_56CYCLES, 56个ADC时钟周期
 *   @arg       ADC_SAMPLETIME_84CYCLES, 84个ADC时钟周期       ADC_SAMPLETIME_112CYCLES,112个ADC时钟周期
 *   @arg       ADC_SAMPLETIME_144CYCLES,144个ADC时钟周期      ADC_SAMPLETIME_480CYCLES,480个ADC时钟周期
 * @param       rank: 多通道采集时需要设置的采集编号,
 *              假设你定义channel1的rank=1，channel2 的rank=2，
 *              那么对应你在DMA缓存空间的变量数组AdcDMA[0] 就i是channel1的转换结果，AdcDMA[1]就是通道2的转换结果。 
 *              单通道DMA设置为 ADC_REGULAR_RANK_1
 *   @arg       编号1~16：ADC_REGULAR_RANK_1~ADC_REGULAR_RANK_16
 * @retval      无
 */
void adc3_channel_set(ADC_HandleTypeDef *adc_handle, uint32_t ch,uint32_t rank, uint32_t stime)
{
    /* 配置对应ADC通道 */
    ADC_ChannelConfTypeDef adc_channel;
    adc_channel.Channel = ch;
    adc_channel.Rank = rank;
    adc_channel.SamplingTime = stime;
    HAL_ADC_ConfigChannel( adc_handle, &adc_channel);    /* 设置ADCX对通道采样时间 */
}

/**
 * @brief       获得ADC转换后的结果
 * @param       ch: 通道值 0~17，取值范围为：ADC_CHANNEL_0~ADC_CHANNEL_17
 * @retval      无
 */
uint32_t adc3_get_result(uint32_t ch)
{
    adc3_channel_set(&g_adc3_handle , ch, 1, ADC_SAMPLETIME_480CYCLES);    /* 设置通道，序列和采样时间 */
    HAL_ADC_Start(&g_adc3_handle);                            /* 开启ADC */
    HAL_ADC_PollForConversion(&g_adc3_handle, 10);            /* 轮询转换 */

    return (uint16_t)HAL_ADC_GetValue(&g_adc3_handle);        /* 返回最近一次ADC1规则组的转换结果 */
}

/**
 * @brief       获取通道ch的转换值,取times次,然后平均
 * @param       ch      : 通道号, 0~17
 * @param       times   : 获取次数
 * @retval      通道ch的times次转换结果平均值
 */
uint32_t adc3_get_result_average(uint32_t ch, uint8_t times)
{
    uint32_t temp_val = 0;
    uint8_t t;

    for (t = 0; t < times; t++)     /* 获取times次数据 */
    {
        temp_val += adc3_get_result(ch);
        delay_ms(5);
    }

    return temp_val / times;        /* 返回平均值 */
}






