/**
 ****************************************************************************************************
 * @file        main.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.4
 * @date        2022-01-04
 * @brief       FreeRTOS 实验
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
 ****************************************************************************************************
 */

#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/usart/usart.h"
#include "./SYSTEM/delay/delay.h"
#include "./BSP/LED/led.h"
#include "./BSP/LCD/lcd.h"
#include "./BSP/KEY/key.h"
#include "./BSP/SRAM/sram.h"
#include "./MALLOC/malloc.h"
#include "freertos_demo.h"
#include "dht11.h"
#include "beep.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdio.h>        
#include <stdint.h>     
#include "./BSP/ADC/adc3.h"
#include "./BSP/LSENS/lsens.h"


// --- Global Queue Handles ---
QueueHandle_t Q_Msg;     
QueueHandle_t Q_Cmd;

// --- Data Structure ---
typedef struct {
    uint8_t temp;  
    uint8_t humi;
    uint16_t light; 
} SensorData_t;

TaskHandle_t Task_Sensor_Handler;
TaskHandle_t Task_UART_Handler;


void Task_Sensor(void *pvParameters) {
    SensorData_t data;
    uint8_t temperature, humidity;
	  uint8_t light_value; 
    

    static uint8_t last_temp = 0;
    static uint8_t last_humi = 0;
    
    while(1) {
        taskENTER_CRITICAL(); 
        uint8_t res = dht11_read_data(&temperature, &humidity);
        taskEXIT_CRITICAL();
        
        if (res == 0) {
           
            last_temp = temperature;
            last_humi = humidity;
           
            HAL_GPIO_WritePin(LED0_GPIO_PORT, LED0_GPIO_PIN, GPIO_PIN_RESET);
            delay_ms(50);
            HAL_GPIO_WritePin(LED0_GPIO_PORT, LED0_GPIO_PIN, GPIO_PIN_SET);
        } 
            
        
        light_value = lsens_get_val(); 
        data.temp = last_temp;
        data.humi = last_humi;
        data.light = light_value; 
        
        xQueueSend(Q_Msg, &data, 10);
        
        vTaskDelay(1500); 
    }
}


void Task_UART(void *pvParameters) {
    SensorData_t rcv_data;
    
    while(1) {
        
        if (xQueueReceive(Q_Msg, &rcv_data, portMAX_DELAY) == pdTRUE) {
            
           
            printf("$ENV,%d,%d,%d#\r\n", rcv_data.temp, rcv_data.humi, rcv_data.light);
        }
    }
}

int main(void)
{
//    HAL_Init();                         /* 初始化HAL库 */
//    sys_stm32_clock_init(336, 8, 2, 7); /* 设置时钟,168Mhz */
//    delay_init(168);                    /* 延时初始化 */
//    usart_init(115200);                 /* 串口初始化为115200 */
//    led_init();                         /* 初始化LED */
//    lcd_init();                         /* 初始化LCD */
//    key_init();                         /* 初始化按键 */
//    sram_init();                        /* SRAM初始化 */
//    my_mem_init(SRAMIN);                /* 初始化内部SRAM内存池 */
//    my_mem_init(SRAMEX);                /* 初始化外部SRAM内存池 */
//    my_mem_init(SRAMCCM);               /* 初始化内部CCM内存池 */
//    
//    freertos_demo();                    /* 运行FreeRTOS例程 */
	
		
    HAL_Init();
    
    sys_stm32_clock_init(336, 8, 2, 7);
    
    delay_init(168);
    usart_init(115200);
    led_init();
		dht11_init();
		lsens_init(); 
    printf("System Init OK...\r\n");

		
    Q_Msg = xQueueCreate(5, sizeof(SensorData_t));

    
    xTaskCreate((TaskFunction_t )Task_Sensor,  
                (const char* )"Sensor",   
                (uint16_t       )128,        
                (void* )NULL,       
                (UBaseType_t    )2,          
                (TaskHandle_t* )&Task_Sensor_Handler);   

    xTaskCreate((TaskFunction_t )Task_UART,     
                (const char* )"UART",     
                (uint16_t       )128,        
                (void* )NULL,
                (UBaseType_t    )3,          
                (TaskHandle_t* )&Task_UART_Handler);
    
   
    vTaskStartScheduler();
    
    while(1);
}
