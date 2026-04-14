#include "stm32f10x.h"                  // Device header
#include "bsp_dht11.h"
#include "Delay.h"                       // 延时函数库

/*================ 全局变量与宏定义 ================*/
unsigned int rec_data[4];  // 存储DHT11读取的传感器数据
                           // [0]=湿度整数部分, [1]=湿度小数部分
                           // [2]=温度整数部分, [3]=温度小数部分

// 超时计数上限，防止死循环（约10ms，远超DHT11响应时间）
#define DHT11_TIMEOUT  10000

/**
 * @brief  初始化DHT11数据线为输出模式
 * @note   PB12配置为推挽输出，用于发送起始信号
 * @param  无
 * @retval 无
 */
void DHT11_GPIO_Init_OUT(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);  // 使能GPIOB时钟
	
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;      // 推挽输出
	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_12;           // PB12
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;      // 50MHz输出速率
	
	GPIO_Init(GPIOB, &GPIO_InitStructure);
}

/**
 * @brief  初始化DHT11数据线为输入模式
 * @note   PB12配置为浮空输入，用于读取DHT11响应和数据
 * @param  无
 * @retval 无
 */
void DHT11_GPIO_Init_IN(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);  // 使能GPIOB时钟
	
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN_FLOATING; // 浮空输入
	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_12;           // PB12
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	
	GPIO_Init(GPIOB, &GPIO_InitStructure);
}

/**
 * @brief  发送DHT11起始信号
 * @note   时序流程：
 *         1. 配置为输出模式
 *         2. 拉高30us
 *         3. 拉低至少18ms(实际20ms)作为起始信号
 *         4. 拉高30us
 *         5. 配置为输入模式，等待DHT11响应
 * @param  无
 * @retval 无
 */
void DHT11_Start(void)
{
	DHT11_GPIO_Init_OUT();  // 切换为输出模式
	
	dht11_high;             // 拉高数据线
	Delay_us(30);           // 保持30us
	
	dht11_low;              // 拉低数据线(起始信号)
	Delay_ms(20);           // 保持20ms(>18ms要求)
	
	dht11_high;             // 释放总线
	Delay_us(30);           // 保持30us
	
	DHT11_GPIO_Init_IN();   // 切换为输入模式，准备读取响应
}

/**
 * @brief  读取DHT11一个字节数据
 * @note   DHT11数据位时序：
 *         - 每个bit前都有50us低电平
 *         - 数据0：高电平持续26-28us
 *         - 数据1：高电平持续70us
 *         延时30us后采样：若为高则是1，若为低则是0
 * @param  无
 * @retval 读取到的8位数据
 */
unsigned char DHT11_Rec_Byte(void)
{
	unsigned char i = 0;
	unsigned char data = 0;              // 初始化为0，用于累积位数据
	volatile uint32_t timeout;           // 超时计数器(防止死循环)
	
	for (i = 0; i < 8; i++)             // 循环读取8个bit
	{
		// 步骤1：等待低电平结束（每个bit开始前有50us低电平）
		timeout = DHT11_TIMEOUT;
		while (Read_Data == 0 && --timeout);
		if (timeout == 0) return 0;      // 超时保护，返回0
		
		Delay_us(30); // 延时30us后判断是0还是1
		              // 如果是0：高电平只持续26-28us，此时已变低
		              // 如果是1：高电平持续70us，此时仍为高
		
		data <<= 1;   // 数据左移1位，腾出最低位
		
		if (Read_Data == 1)
		{
			data |= 1;  // 如果当前为高电平，则最低位置1
		}
		
		// 步骤2：等待高电平结束，准备读取下一个bit
		timeout = DHT11_TIMEOUT;
		while (Read_Data == 1 && --timeout);
		if (timeout == 0) return 0;      // 超时保护
	}
	
	return data;
}

/**
 * @brief  读取DHT11完整数据(40位)
 * @note   DHT11数据格式：湿度高位(8) + 湿度低位(8) + 温度高位(8) + 温度低位(8) + 校验和(8)
 *         通信流程：
 *         1. 主机发送起始信号
 *         2. DHT11响应(80us低+80us高)
 *         3. 传输40位数据
 *         4. 校验：R_H+R_L+T_H+T_L == CHECK
 * @param  无
 * @retval 无 (数据存入全局数组rec_data)
 */
void DHT11_REC_Data(void)
{
	unsigned int R_H, R_L, T_H, T_L;  // 湿度/温度的整数和小数部分
	unsigned char CHECK;               // 校验和
	volatile uint32_t timeout;
	
	DHT11_Start();  // 发送起始信号
	dht11_high;     // 释放总线
	
	if (Read_Data == 0)  // 检测到DHT11响应(拉低总线)
	{
		// 等待DHT11响应信号低电平结束(80us)
		timeout = DHT11_TIMEOUT;
		while (Read_Data == 0 && --timeout);
		if (timeout == 0) return;  // 超时保护
		
		// 等待DHT11响应信号高电平结束(80us)
		timeout = DHT11_TIMEOUT;
		while (Read_Data == 1 && --timeout);
		if (timeout == 0) return;  // 超时保护
		
		// 连续读取5个字节：湿度高8位、湿度低8位、温度高8位、温度低8位、校验和
		R_H = DHT11_Rec_Byte();
		R_L = DHT11_Rec_Byte();
		T_H = DHT11_Rec_Byte();
		T_L = DHT11_Rec_Byte();
		CHECK = DHT11_Rec_Byte();
		
		// 拉低总线55us后释放，表示读取完成
		dht11_low;
		Delay_us(55);
		dht11_high;
		
		// 校验数据：仅在校验通过时更新rec_data，否则保留上次有效数据
		if (R_H + R_L + T_H + T_L == CHECK)
		{
			rec_data[0] = R_H;  // 湿度整数部分
			rec_data[1] = R_L;  // 湿度小数部分
			rec_data[2] = T_H;  // 温度整数部分
			rec_data[3] = T_L;  // 温度小数部分
		}
	}
}

