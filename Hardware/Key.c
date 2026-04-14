#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"

void Key_Init(void)
{
	RCC_APB2PeriphClockCmd (RCC_APB2Periph_GPIOA ,ENABLE);
	RCC_APB2PeriphClockCmd (RCC_APB2Periph_GPIOC ,ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz ;
	GPIO_Init(GPIOA ,&GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz ;
	GPIO_Init(GPIOC ,&GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz ;
	GPIO_Init(GPIOB ,&GPIO_InitStructure);
//	/* 定义一个结构体 */
//	GPIO_InitTypeDef gpio_initstruct = {0};
//	
//	/* 开启KEY相关的GPIO外设/端口时钟 */
//	RCC_APB2PeriphClockCmd(WKUP_KEY1_GPIO_CLK_PORT,ENABLE);
//	RCC_APB2PeriphClockCmd(KEY2_GPIO_CLK_PORT,ENABLE);
//	
//	/*选择要控制的GPIO引脚、设置GPIO模式为浮空输入、设置GPIO速率为50MHz*/
//	gpio_initstruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
//	gpio_initstruct.GPIO_Pin = WKUP_KEY1_GPIO_PIN;//
//	gpio_initstruct.GPIO_Speed = GPIO_Speed_50MHz;
//	GPIO_Init(WKUP_KEY1_GPIO_PORT,&gpio_initstruct);
//	
//	gpio_initstruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
//	gpio_initstruct.GPIO_Pin = KEY2_GPIO_PIN;
//	gpio_initstruct.GPIO_Speed = GPIO_Speed_50MHz;
//	GPIO_Init(KEY2_GPIO_PORT,&gpio_initstruct);

}

uint8_t Key_GetNum(void)
{
	uint8_t KeyNum = 0;
	if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 1)
	{
		Delay_ms(20);
		while (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 1)
		Delay_ms (20);
		KeyNum =1;
	}
	if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_13) == 1)
	{
		Delay_ms(20);
		while(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_13) == 1)
		Delay_ms (20);
		KeyNum =2;
	}
		
	return KeyNum;
}

/**
 * @brief 按键功能：调整主菜单选项（a1）
 */
//int Key_GetNum1(void)//按键按下时GPIO为1，松开时0
//{
//	int a1 = 0;   
//	// 检查按键 GPIOA Pin 0 是否被按下
//	if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 1)
//	{
//			Delay_ms(20);  // 消抖处理
//			if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 1) // 确认按下
//			{
//					Delay_ms(200);  // 长按延时
//					a1 += 1;        // 主菜单索引加1
//					if (a1 == 4)    // 循环限制：当索引超过最大值（3）时，重置为0
//					{
//							a1 = 0;
//					}
//			}
//	}
//	else if (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_13) == 1) // 检查 GPIOC Pin 13 按键是否被按下
//	{
//			Delay_ms(20);  // 消抖处理
//			if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_13) == 1) // 确认按下
//			{
//					Delay_ms(200);  // 长按延时
//					a1 -= 1;        // 主菜单索引减1
//					if (a1 == -1)   // 循环限制：当索引小于最小值（0）时，重置为3
//					{
//							a1 = 3;
//					}
//			}
//	}
//	
//  return a1;
//}


///**
// * @brief 按键功能：控制子菜单切换（a2）
// */
//int Key_GetNum2(void)
//{         
//	int temp = Key_GetNum1();
//  int a2 = 0; 
//	// 检查按键 GPIOB Pin 15 是否被按下
//	if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_15) == 1)
//	{
//			Delay_ms(200);  // 长按延时
//			while (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_15) == 1); // 确认按下
//			Delay_ms(500);  // 额外延时处理
//			OLED_Clear();   // 清屏，准备显示新的菜单内容

//			if (a2 == 0)    // 如果当前在主菜单
//			{
//					a2 = a2 + 1 + temp; // 切换到子菜单，a2值根据主菜单索引调整
//			}
//			else
//			{
//					a2 = 0;      // 返回主菜单
//			}
//	}

//	return a2;
//}

