#include "stm32f10x.h"
#include "OLED_Font.h"

/*================ 引脚宏定义 ================*/
// I2C通信引脚控制宏 (PB6=SCL, PB7=SDA)
#define OLED_W_SCL(x)		GPIO_WriteBit(GPIOB, GPIO_Pin_6, (BitAction)(x))
#define OLED_W_SDA(x)		GPIO_WriteBit(GPIOB, GPIO_Pin_7, (BitAction)(x))

/**
 * @brief  初始化OLED的I2C通信引脚
 * @note   PB6(SCL)和PB7(SDA)配置为开漏输出，模拟I2C协议
 * @param  无
 * @retval 无
 */
void OLED_I2C_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);  // 使能GPIOB时钟
	
	GPIO_InitTypeDef GPIO_InitStructure;
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;  // 开漏输出(I2C要求)
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;  // SCL
 	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;  // SDA
 	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	OLED_W_SCL(1);  // 空闲时拉高
	OLED_W_SDA(1);
}

/**
 * @brief  I2C起始信号
 * @note   时序：SDA高电平时，SCL从高变低
 * @param  无
 * @retval 无
 */
void OLED_I2C_Start(void)
{
	OLED_W_SDA(1);
	OLED_W_SCL(1);
	OLED_W_SDA(0);
	OLED_W_SCL(0);
}

/**
 * @brief  I2C停止信号
 * @note   时序：SDA低电平时，SCL从低变高
 * @param  无
 * @retval 无
 */
void OLED_I2C_Stop(void)
{
	OLED_W_SDA(0);
	OLED_W_SCL(1);
	OLED_W_SDA(1);
}

/**
 * @brief  I2C发送一个字节
 * @note   从高位到低位逐位发送，MSB first
 * @param  Byte 要发送的一个字节
 * @retval 无
 */
void OLED_I2C_SendByte(uint8_t Byte)
{
	uint8_t i;
	for (i = 0; i < 8; i++)
	{
		OLED_W_SDA(Byte & (0x80 >> i));  // 发送当前位
		OLED_W_SCL(1);                   // SCL拉高，数据有效
		OLED_W_SCL(0);                   // SCL拉低，准备下一位
	}
	OLED_W_SCL(1);  // 额外的一个时钟，不处理应答信号
	OLED_W_SCL(0);
}

/**
 * @brief  OLED写命令
 * @note   I2C帧格式：[起始][0x78(地址)][0x00(命令)][Command][停止]
 * @param  Command 要写入的命令
 * @retval 无
 */
void OLED_WriteCommand(uint8_t Command)
{
	OLED_I2C_Start();
	OLED_I2C_SendByte(0x78);		// 从机地址(SSD1306默认0x3C, 写操作左移1位=0x78)
	OLED_I2C_SendByte(0x00);		// 表示接下来是命令
	OLED_I2C_SendByte(Command); 
	OLED_I2C_Stop();
}

/**
 * @brief  OLED写数据
 * @note   I2C帧格式：[起始][0x78(地址)][0x40(数据)][Data][停止]
 * @param  Data 要写入的数据
 * @retval 无
 */
void OLED_WriteData(uint8_t Data)
{
	OLED_I2C_Start();
	OLED_I2C_SendByte(0x78);		// 从机地址
	OLED_I2C_SendByte(0x40);		// 表示接下来是数据
	OLED_I2C_SendByte(Data);
	OLED_I2C_Stop();
}

/**
 * @brief  OLED设置光标位置
 * @note   SSD1308页地址模式：
 *         Y(0-7)表示页地址(8行为1页，共8页)
 *         X(0-127)表示列地址
 * @param  Y 以左上角为原点，向下方向的页地址，范围：0~7
 * @param  X 以左上角为原点，向右方向的列地址，范围：0~127
 * @retval 无
 */
void OLED_SetCursor(uint8_t Y, uint8_t X)
{
	OLED_WriteCommand(0xB0 | Y);					// 设置页地址(Y0-Y7)
	OLED_WriteCommand(0x10 | ((X & 0xF0) >> 4));	// 设置列地址高4位
	OLED_WriteCommand(0x00 | (X & 0x0F));			// 设置列地址低4位
}

/**
 * @brief  OLED清屏
 * @note   遍历8页×128列，全部写入0x00
 * @param  无
 * @retval 无
 */
void OLED_Clear(void)
{  
	uint8_t i, j;
	for (j = 0; j < 8; j++)             // 遍历8页
	{
		OLED_SetCursor(j, 0);
		for(i = 0; i < 128; i++)        // 遍历128列
		{
			OLED_WriteData(0x00);       // 写入0，熄灭像素
		}
	}
}

/**
 * @brief  OLED显示一个字符
 * @note   使用8×16字体，每个字符占用2页(16行)×8列
 *         上半部分8行 + 下半部分8行
 * @param  Line 行位置(1-4)，对应4个字符行
 * @param  Column 列位置(1-16)，对应16个字符列
 * @param  Char 要显示的字符(ASCII可见字符)
 * @retval 无
 */
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char)
{      	
	uint8_t i;
	uint16_t FontIndex = (uint8_t)(Char - ' ') * 16U;
	// 设置光标到字符上半部分(Line转页地址: Line1→页0, Line2→页2...)
	OLED_SetCursor((Line - 1) * 2, (Column - 1) * 8);
	for (i = 0; i < 8; i++)
	{
		OLED_WriteData(OLED_F8x16[FontIndex + i]);			// 显示上半部分(前8行)
	}
	// 设置光标到字符下半部分
	OLED_SetCursor((Line - 1) * 2 + 1, (Column - 1) * 8);
	for (i = 0; i < 8; i++)
	{
		OLED_WriteData(OLED_F8x16[FontIndex + i + 8U]);		// 显示下半部分(后8行)
	}
}

/**
 * @brief  OLED显示摄氏度符号
 * @note   通过自定义小圆圈字模 + ASCII 'C' 组合显示，兼容当前ASCII字库
 * @param  Line 行位置(1-4)
 * @param  Column 起始列位置(1-15)，占用2列
 * @retval 无
 */
void OLED_ShowCelsiusSymbol(uint8_t Line, uint8_t Column)
{
	uint8_t i;

	OLED_SetCursor((Line - 1) * 2, (Column - 1) * 8);
	for (i = 0; i < 8; i++)
	{
		OLED_WriteData(OLED_DegreeSymbol8x16[i]);
	}

	OLED_SetCursor((Line - 1) * 2 + 1, (Column - 1) * 8);
	for (i = 0; i < 8; i++)
	{
		OLED_WriteData(OLED_DegreeSymbol8x16[i + 8U]);
	}

	OLED_ShowChar(Line, Column + 1, 'C');
}

/**
 * @brief  OLED显示字符串
 * @note   逐个字符调用OLED_ShowChar
 * @param  Line 起始行位置(1-4)
 * @param  Column 起始列位置(1-16)
 * @param  String 要显示的字符串(以'\0'结尾)
 * @retval 无
 */
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String)
{
	uint8_t i;
	for (i = 0; String[i] != '\0'; i++)
	{
		OLED_ShowChar(Line, Column + i, String[i]);
	}
}

/**
 * @brief  OLED次方函数
 * @note   计算X的Y次方，用于数字显示时的位权计算
 * @param  X 底数
 * @param  Y 指数
 * @retval X^Y
 */
uint32_t OLED_Pow(uint32_t X, uint32_t Y)
{
	uint32_t Result = 1;
	while (Y--)
	{
		Result *= X;
	}
	return Result;
}

/**
 * @brief  OLED显示数字(十进制无符号数)
 * @note   将数字按位分解，逐位显示
 *         例如：Number=123, Length=3 → 显示"123"
 * @param  Line 起始行位置(1-4)
 * @param  Column 起始列位置(1-16)
 * @param  Number 要显示的数字(0~4294967295)
 * @param  Length 显示长度(1-10)，不足前面补空格
 * @retval 无
 */
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
	uint8_t i;
	for (i = 0; i < Length; i++)
	{
		// 提取每一位数字：先除位权取整，再模10取个位
		OLED_ShowChar(Line, Column + i, Number / OLED_Pow(10, Length - i - 1) % 10 + '0');
	}
}

/**
  * @brief  OLED显示数字（十进制，带符号数）
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  Number 要显示的数字，范围：-2147483648~2147483647
  * @param  Length 要显示数字的长度，范围：1~10
  * @retval 无
  */
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length)
{
	uint8_t i;
	uint32_t Number1;
	if (Number >= 0)
	{
		OLED_ShowChar(Line, Column, '+');
		Number1 = Number;
	}
	else
	{
		OLED_ShowChar(Line, Column, '-');
		Number1 = -Number;
	}
	for (i = 0; i < Length; i++)							
	{
		OLED_ShowChar(Line, Column + i + 1, Number1 / OLED_Pow(10, Length - i - 1) % 10 + '0');
	}
}

/**
  * @brief  OLED显示数字（十六进制，正数）
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  Number 要显示的数字，范围：0~0xFFFFFFFF
  * @param  Length 要显示数字的长度，范围：1~8
  * @retval 无
  */
void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
	uint8_t i, SingleNumber;
	for (i = 0; i < Length; i++)							
	{
		SingleNumber = Number / OLED_Pow(16, Length - i - 1) % 16;
		if (SingleNumber < 10)
		{
			OLED_ShowChar(Line, Column + i, SingleNumber + '0');
		}
		else
		{
			OLED_ShowChar(Line, Column + i, SingleNumber - 10 + 'A');
		}
	}
}

/**
  * @brief  OLED显示数字（二进制，正数）
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  Number 要显示的数字，范围：0~1111 1111 1111 1111
  * @param  Length 要显示数字的长度，范围：1~16
  * @retval 无
  */
void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
	uint8_t i;
	for (i = 0; i < Length; i++)							
	{
		OLED_ShowChar(Line, Column + i, Number / OLED_Pow(2, Length - i - 1) % 2 + '0');
	}
}

/**
 * @brief  OLED初始化(SSD1306驱动芯片)
 * @note   初始化流程：
 *         1. 上电延时(等待OLED稳定)
 *         2. I2C引脚初始化
 *         3. 发送一系列配置命令
 *         4. 清屏并开启显示
 * @param  无
 * @retval 无
 */
void OLED_Init(void)
{
	uint32_t i, j;
	
	// 步骤1：上电延时(约100ms，等待OLED电源稳定)
	for (i = 0; i < 1000; i++)
	{
		for (j = 0; j < 1000; j++);
	}
	
	// 步骤2：I2C通信引脚初始化
	OLED_I2C_Init();
	
	// 步骤3：发送SSD1306配置命令
	OLED_WriteCommand(0xAE);	// 关闭显示
	
	OLED_WriteCommand(0xD5);	// 设置显示时钟分频比/振荡器频率
	OLED_WriteCommand(0x80);	// 默认值：分频比=1，频率=正常
	
	OLED_WriteCommand(0xA8);	// 设置多路复用率
	OLED_WriteCommand(0x3F);	// 64路复用(128x64屏幕)
	
	OLED_WriteCommand(0xD3);	// 设置显示偏移
	OLED_WriteCommand(0x00);	// 无偏移
	
	OLED_WriteCommand(0x40);	// 设置显示起始行为第0行
	
	OLED_WriteCommand(0xA1);	// 设置段重映射(0xA1正常, 0xA0左右翻转)
	
	OLED_WriteCommand(0xC8);	// 设置COM输出扫描方向(0xC8正常, 0xC0上下翻转)

	OLED_WriteCommand(0xDA);	// 设置COM引脚硬件配置
	OLED_WriteCommand(0x12);	// 启用备用配置
	
	OLED_WriteCommand(0x81);	// 设置对比度控制
	OLED_WriteCommand(0xCF);	// 对比度值=207(中等亮度)

	OLED_WriteCommand(0xD9);	// 设置预充电周期
	OLED_WriteCommand(0xF1);	// 阶段1=15时钟, 阶段2=1时钟

	OLED_WriteCommand(0xDB);	// 设置VCOMH取消选择级别
	OLED_WriteCommand(0x30);	// VCOMH电压=0.83×VCC

	OLED_WriteCommand(0xA4);	// 恢复RAM内容显示(非全亮)

	OLED_WriteCommand(0xA6);	// 设置正常显示(0xA7为反色)

	OLED_WriteCommand(0x8D);	// 启用电荷泵
	OLED_WriteCommand(0x14);	// 使能电荷泵(必须开启才能显示)

	OLED_WriteCommand(0xAF);	// 开启显示
		
	// 步骤4：清屏
	OLED_Clear();
}
