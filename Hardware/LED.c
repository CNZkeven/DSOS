
#include "stm32f10x.h"
#include "PWM.h"

static uint8_t led_enabled = 0;
static uint8_t led_counter = 0;

void LED1_ON(void)
{
    led_enabled = 1;
}

void LED1_OFF(void)
{
    led_enabled = 0;
    led_counter = 0;
    TIM_SetCompare2(TIM2, 100);
    TIM_SetCompare3(TIM2, 100);
}

void LED2_ON(void)
{
    led_enabled = 1;
}

void LED2_OFF(void)
{
    led_enabled = 0;
    led_counter = 0;
    TIM_SetCompare2(TIM2, 100);
    TIM_SetCompare3(TIM2, 100);
}

void LED_Update(void)
{
    if (!led_enabled)
    {
        return;
    }

    led_counter++;

    if (led_counter < 3)
    {
        TIM_SetCompare2(TIM2, 0);
        TIM_SetCompare3(TIM2, 100);
    }
    else
    {
        TIM_SetCompare2(TIM2, 100);
        TIM_SetCompare3(TIM2, 0);
    }

    if (led_counter >= 6)
    {
        led_counter = 0;
    }
}
