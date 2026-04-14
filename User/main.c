#include "stm32f10x.h"
#include "Delay.h"
#include "OLED.h"
#include "LED.h"
#include "Buzzer.h"
#include "bsp_dht11.h"
#include "Key.h"
#include "PWM.h"
#include <stdbool.h>

/*================ 全局变量定义 ================*/
int a1 = 0;      // 主菜单当前选项索引 (0-4): 0=Hello, 1=蜂鸣器, 2=温湿度, 3=LED, 4=计数器
int a2 = 0;      // 子菜单状态: 0=主菜单, 1=Hello, 2=蜂鸣器, 3=温湿度, 4=LED, 5=计数器
int keynum = 0;  // LED控制状态: 0=无操作, 1=开启, 2=关闭
uint8_t count = 0;  // 计数器值 (0-99), 用于质数判断功能
extern unsigned int rec_data[4];  // DHT11传感器数据: [0]=湿度整数, [1]=湿度小数, [2]=温度整数, [3]=温度小数

// 记录上一次的菜单状态，用于判断是否需要刷新OLED（避免闪烁）
static int last_a1 = -1;
static int last_a2 = -1;
static const char *const main_menu_items[5] = {"Hello!", "Buzzer", "Temp&Hum", "LED", "Count"};

/*================ 函数声明 ================*/
bool Is_Prime(uint8_t n);  // 质数判断函数
void menu(void);           // 菜单逻辑函数

static const char *Menu_GetItemLabel(int index)
{
    return main_menu_items[index % 5];
}

/**
 * @brief  按键功能1：调整主菜单选项（a1）
 * @note   使用PA0按键(KEY1)向上翻页，PC13按键(KEY2)向下翻页
 *         采用软件消抖(20ms延时) + 等待按键释放的阻塞式检测
 * @param  无
 * @retval 无
 */
void Key_GetNum1(void)
{
    // 检测PA0按键(向上翻页)
    if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 1)
    {
        Delay_ms(20);  // 软件消抖
        if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 1)
        {
            while (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 1);  // 等待按键释放
            Delay_ms(20);  // 释放消抖
            a1++;  // 菜单索引加1
            if (a1 >= 5) { a1 = 0; }  // 循环回0
        }
    }
    // 检测PC13按键(向下翻页)
    else if (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_13) == 1)
    {
        Delay_ms(20);  // 软件消抖
        if (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_13) == 1)
        {
            while (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_13) == 1);  // 等待按键释放
            Delay_ms(20);  // 释放消抖
            a1--;  // 菜单索引减1
            if (a1 < 0) { a1 = 4; }  // 循环到4
        }
    }
}

/**
 * @brief  按键功能2：控制子菜单切换（a2）
 * @note   使用PB15按键(KEY3)进入/退出子菜单
 *         按下KEY3时：如果a2=0(主菜单)则进入对应子菜单，否则返回主菜单
 * @param  无
 * @retval 无
 */
void Key_GetNum2(void)
{
    if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_15) == 1)
    {
        Delay_ms(20);  // 软件消抖
        while (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_15) == 1);  // 等待按键释放
        Delay_ms(20);  // 释放消抖
        OLED_Clear();  // 清屏，准备显示新界面
        last_a1 = -1;  // 重置菜单状态，强制刷新
        last_a2 = -1;

        if (a2 == 0)
        {
            // 从主菜单进入子菜单：根据当前a1值跳转到对应功能
            // a1=0→a2=1(Hello), a1=1→a2=2(蜂鸣器), a1=2→a2=3(温湿度), a1=3→a2=4(LED), a1=4→a2=5(计数器)
            a2 = a2 + 1 + a1;
        }
        else
        {
            // 从子菜单返回主菜单
            Buzzer_OFF();
            LED1_OFF();
            LED2_OFF();
            a2 = 0;
            keynum = 0;  // 重置LED状态
        }
    }
}

/**
 * @brief  主函数，系统初始化和主循环
 * @note   执行流程：
 *         1. 初始化OLED显示和PWM输出
 *         2. 显示开机画面："weijikeshe" 居中显示3秒，第二行显示团队成员姓名首字母
 *         3. 初始化蜂鸣器和按键
 *         4. 进入无限循环，持续调用menu()处理用户交互
 * @param  无
 * @retval 无
 */
int main(void)
{
    OLED_Init();  // 初始化OLED显示屏(I2C通信)
    PWM_Init();   // 初始化PWM输出(TIM2_CH2/CH3)，用于控制LED亮度

    // 开机画面：第一行居中显示课程名称，第二行显示团队成员姓名首字母
    OLED_ShowString(1, 4, "WeiJiKeShe");
    OLED_ShowString(2, 1, "ZZK XHB LP LBY");
    Delay_ms(3000);  // 显示3秒
    OLED_Clear();    // 清屏
    Delay_ms(100);

    Buzzer_Init();   // 初始化蜂鸣器控制引脚
    Key_Init();      // 初始化按键GPIO
    Buzzer_OFF();    // 确保蜂鸣器初始为关闭状态

    while (1)
    {
        menu();  // 主循环：处理菜单逻辑和用户输入
    }
}

/**
 * @brief 菜单逻辑函数，控制主菜单及子功能的显示与操作
 *
 * OLED 布局设计（128x64, 8x16字体, 16列x4行）:
 *
 * 主菜单:
 *   Line1: "[SYS] MAIN HUB "
 *   Line2: "> Hello!       "  (当前选项，">"指示器)
 *   Line3: "  Buzzer       "  (下一项预览)
 *   Line4: "K1K2:NAV K3:OK"
 *
 * 子菜单:
 *   Line1: 功能标题
 *   Line2: 功能内容
 *   Line3: 功能内容
 *   Line4: 操作提示 (含 3:Back)
 */
void menu(void)
{
    LED_Update();

    // 仅在主菜单时响应翻页按键
    if (a2 == 0)
    {
        Key_GetNum1();
    }
    Key_GetNum2();

    // ===== 主菜单（a2==0），共5项 =====
    if (a2 == 0)
    {
        if (a1 != last_a1 || a2 != last_a2)
        {
            last_a1 = a1;
            last_a2 = a2;

            OLED_ShowString(1, 1, "[SYS] MAIN HUB ");
            OLED_ShowString(2, 1, ">               ");
            OLED_ShowString(2, 3, (char *)Menu_GetItemLabel(a1));
            OLED_ShowString(3, 1, "                ");
            OLED_ShowString(3, 3, (char *)Menu_GetItemLabel(a1 + 1));
            OLED_ShowString(4, 1, "K1K2:NAV K3:OK");
        }
        Delay_ms(50);
    }
    // ===== Hello 子菜单 =====
    else if (a2 == 1)
    {
        if (a2 != last_a2)
        {
            last_a2 = a2;
            OLED_ShowString(1, 1, "[SYS] HELLO!  ");
            OLED_ShowString(2, 1, " WEIJI READY  ");
            OLED_ShowString(3, 1, " STM32 ONLINE ");
            OLED_ShowString(4, 1, "K3:BACK       ");
        }
    }
    // ===== 蜂鸣器子菜单 =====
    else if (a2 == 2)
    {
        if (a2 != last_a2)
        {
            last_a2 = a2;
            OLED_ShowString(1, 1, "[AUDIO] BUZZER ");
            OLED_ShowString(2, 1, " MODE: STBY    ");
            OLED_ShowString(3, 1, " KEY1 ON KEY2OFF");
            OLED_ShowString(4, 1, "K1ON K2OFF K3BK");
        }

        // 按键关闭蜂鸣器
        if (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_13) == 1)
        {
            Delay_ms(20);
            if (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_13) == 1)
            {
                while (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_13) == 1);
                Delay_ms(20);
                OLED_ShowString(2, 1, " MODE: OFF     ");
                OLED_ShowString(3, 1, " AUDIO: MUTE   ");
                Buzzer_OFF();
            }
        }
        // 按键打开蜂鸣器
        if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 1)
        {
            Delay_ms(20);
            if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 1)
            {
                while (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 1);
                Delay_ms(20);
                OLED_ShowString(2, 1, " MODE: ON      ");
                OLED_ShowString(3, 1, " AUDIO: LIVE   ");
                Buzzer_ON();
            }
        }
    }
    // ===== 温湿度子菜单 =====
    else if (a2 == 3)
    {
        if (a2 != last_a2)
        {
            last_a2 = a2;
            OLED_ShowString(1, 1, "[SENSE] TEMP  ");
            OLED_ShowString(4, 1, "LIVE  K3:BACK ");
        }

        Delay_ms(500);
        DHT11_REC_Data();

        OLED_ShowString(2, 1, "TEMP    .      ");
        OLED_ShowNum(2, 5, rec_data[2], 2);
        OLED_ShowString(2, 8, ".");
        OLED_ShowNum(2, 9, rec_data[3], 1);
        OLED_ShowCelsiusSymbol(2, 10);

        OLED_ShowString(3, 1, "HUM :  . %     ");
        OLED_ShowNum(3, 5, rec_data[0], 2);
        OLED_ShowString(3, 8, ".");
        OLED_ShowNum(3, 9, rec_data[1], 2);
    }
    // ===== LED控制子菜单 =====
    else if (a2 == 4)
    {
        if (a2 != last_a2)
        {
            last_a2 = a2;
            OLED_ShowString(1, 1, "[LIGHT] LED   ");
            OLED_ShowString(2, 1, " MODE: STBY    ");
            OLED_ShowString(3, 1, " KEY1 ON KEY2OFF");
            OLED_ShowString(4, 1, "K1ON K2OFF K3BK");
        }

        // 按键关闭 LED
        if (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_13) == 1)
        {
            Delay_ms(20);
            if (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_13) == 1)
            {
                while (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_13) == 1);
                keynum = 2;
            }
        }
        // 按键打开 LED
        else if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 1)
        {
            Delay_ms(20);
            if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 1)
            {
                while (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 1);
                keynum = 1;
            }
        }

        if (keynum == 1)
        {
            OLED_ShowString(2, 1, " MODE: ON      ");
            OLED_ShowString(3, 1, " LIGHT: ACTIVE ");
            LED1_ON();
            LED2_ON();
        }
        else if (keynum == 2)
        {
            OLED_ShowString(2, 1, " MODE: OFF     ");
            OLED_ShowString(3, 1, " LIGHT: IDLE   ");
            LED1_OFF();
            LED2_OFF();
        }

        Delay_ms(10);
    }
    // ===== 计数与质数判断子菜单 =====
    else if (a2 == 5)
    {
        if (a2 != last_a2)
        {
            last_a2 = a2;
            OLED_ShowString(1, 1, "[COUNT] PRIME ");
            OLED_ShowString(4, 1, "K1:+1 K2:CLR ");
        }

        // 按键清零
        if (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_13) == 1)
        {
            Delay_ms(20);
            if (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_13) == 1)
            {
                while (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_13) == 1);
                Delay_ms(20);
                count = 0;
            }
        }

        // 按键递增
        if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 1)
        {
            Delay_ms(20);
            if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 1)
            {
                while (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 1);
                Delay_ms(20);
                count++;
                if (count >= 100)
                {
                    count = 0;
                }
            }
        }

        OLED_ShowString(2, 1, "NUM:           ");
        OLED_ShowNum(2, 5, count, 3);

        if (Is_Prime(count))
        {
            OLED_ShowString(3, 1, "FLAG: PRIME    ");
        }
        else
        {
            OLED_ShowString(3, 1, "FLAG: NORMAL   ");
        }
        Delay_ms(100);
    }
}

// 判断一个数是否为质数
bool Is_Prime(uint8_t n)
{
    if (n <= 1) return false;
    if (n == 2) return true;
    if (n % 2 == 0) return false;
    for (uint8_t i = 3; i * i <= n; i += 2)
    {
        if (n % i == 0) return false;
    }
    return true;
}
