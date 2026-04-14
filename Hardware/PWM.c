#include "stm32f10x.h"                  // Device header

/**
 * @brief  初始化PWM输出(TIM2_CH2和TIM2_CH3)
 * @note   用于控制LED亮度(PWM频率1KHz)
 *         时钟计算：72MHz / (719+1) / (99+1) = 1KHz
 *         PA1 = TIM2_CH2, PA2 = TIM2_CH3
 * @param  无
 * @retval 无
 */
void PWM_Init(void)
{
	// 步骤1：使能相关时钟
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);   // 使能TIM2时钟(APB1)
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);  // 使能GPIOA时钟(APB2)
	
	// 步骤2：配置GPIO为复用推挽输出(由定时器控制)
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;        // 复用推挽输出
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2; // PA1(CH2), PA2(CH3)
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	// 步骤3：选择定时器内部时钟
	TIM_InternalClockConfig(TIM2);
	
	// 步骤4：初始化时基单元(配置定时器基本参数)
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;   // 时钟分频因子=1
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;  // 向上计数模式
	TIM_TimeBaseInitStructure.TIM_Period = 100 - 1;    // ARR(自动重装载值)=99
	TIM_TimeBaseInitStructure.TIM_Prescaler = 720 - 1; // PSC(预分频器)=719
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);
	
	// 步骤5：初始化输出比较单元(配置PWM模式)
	TIM_OCInitTypeDef TIM_OCInitStructure;
	TIM_OCStructInit(&TIM_OCInitStructure);  // 使用默认值填充
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;           // PWM模式1
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;   // 输出极性：高电平有效
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;  // 使能输出
	TIM_OCInitStructure.TIM_Pulse = 100;  // CCR(比较值)，初始占空比100%
	
	// 步骤6：初始化通道2和通道3
	TIM_OC2Init(TIM2, &TIM_OCInitStructure);  // TIM2_CH2 (PA1)
	TIM_OC3Init(TIM2, &TIM_OCInitStructure);  // TIM2_CH3 (PA2)
	
	// 步骤7：启动定时器
	TIM_Cmd(TIM2, ENABLE);
}








