#ifndef __BSP_GPIO_KEY_H
#define __BSP_GPIO_KEY_H

#include "stm32f10x.h"


/* 定义LED连接的GPIO端口, 用户只需要修改下面的代码即可改变控制的LED引脚 */
// WKUP/KEY1按键
#define WKUP_KEY1_GPIO_PORT    			GPIOA			                /* GPIO端口 */
#define WKUP_KEY1_GPIO_CLK_PORT 	    RCC_APB2Periph_GPIOA			/* GPIO端口时钟 */
#define WKUP_KEY1_GPIO_PIN			    GPIO_Pin_0	       				/* 连接到GPIO */

// KEY2按键
#define KEY2_GPIO_PORT    			    GPIOC			                /* GPIO端口 */
#define KEY2_GPIO_CLK_PORT 	            RCC_APB2Periph_GPIOC			/* GPIO端口时钟 */
#define KEY2_GPIO_PIN			        GPIO_Pin_13	       				/* 连接到GPIO */

// KEY3按键
#define KEY3_GPIO_PORT    			    GPIOB			                /* GPIO端口 */
#define KEY3_GPIO_CLK_PORT 	            RCC_APB2Periph_GPIOB			/* GPIO端口时钟 */
#define KEY3_GPIO_PIN			        GPIO_Pin_15	       				/* 连接到GPIO */


/** 触发 板载按键 条件 
  * 0 - OFF
  * 1 - ON
  */
/** 按键按下标置宏
 *  按键按下为高电平，设置 KEY_ON=1， KEY_OFF=0
 *  若按键按下为低电平，把宏设置成KEY_ON=0 ，KEY_OFF=1 即可
 */ 
#define KEY_ON  1
#define KEY_OFF 0

void KEY_GPIO_Config(void);
uint32_t KEY_SCAN(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);

#endif /* __BSP_GPIO_KEY_H  */
