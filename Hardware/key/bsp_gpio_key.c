/**
  ******************************************************************************
  * @file    bsp_gpio_led.c
  * @author     embedfire
  * @version     V1.0
  * @date        2024
  * @brief   扫描按键函数接口
  ******************************************************************************
  * @attention
  *
  * 实验平台  ：野火 STM32F103C8T6-STM32开发板 
  * 论坛      ：http://www.firebbs.cn
  * 官网      ：https://embedfire.com/
  * 淘宝      ：https://yehuosm.tmall.com/
  *
  ******************************************************************************
  */
  
  
#include "key/bsp_gpio_key.h"


/**
 * @brief  初始化按键GPIO引脚
 * @note   配置WK_UP(PA0)和KEY2(PC13)为浮空输入模式
 * @param  无
 * @retval 无
 */
void KEY_GPIO_Config(void)
{
    /* 定义GPIO初始化结构体 */
    GPIO_InitTypeDef gpio_initstruct = {0};
    
    /* 使能按键相关的GPIO时钟 */
    RCC_APB2PeriphClockCmd(WKUP_KEY1_GPIO_CLK_PORT, ENABLE);  // PA0时钟
    RCC_APB2PeriphClockCmd(KEY2_GPIO_CLK_PORT, ENABLE);       // PC13时钟
    
    /* 配置WK_UP按键(PA0) */
    gpio_initstruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;        // 浮空输入
    gpio_initstruct.GPIO_Pin = WKUP_KEY1_GPIO_PIN;            // PA0
    gpio_initstruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(WKUP_KEY1_GPIO_PORT, &gpio_initstruct);
    
    /* 配置KEY2按键(PC13) */
    gpio_initstruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;        // 浮空输入
    gpio_initstruct.GPIO_Pin = KEY2_GPIO_PIN;                 // PC13
    gpio_initstruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(KEY2_GPIO_PORT, &gpio_initstruct);
}

/**
 * @brief  按键扫描函数(阻塞式)
 * @note   检测按键是否按下，采用等待释放的方式
 *         注意：此函数会阻塞直到按键释放
 * @param  GPIOx：x可以是A、B、C、D或E
 * @param  GPIO_Pin：待读取的引脚
 * @retval KEY_ON(按下) 或 KEY_OFF(未按下)
 */
uint32_t KEY_SCAN(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{
    if (GPIO_ReadInputDataBit(GPIOx, GPIO_Pin) == KEY_ON)
    {
        // 检测到按键按下，等待释放
        while (GPIO_ReadInputDataBit(GPIOx, GPIO_Pin) == KEY_ON);
        return KEY_ON;  // 返回按下状态
    }
    return KEY_OFF;  // 未按下
}




