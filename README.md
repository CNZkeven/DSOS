# 桌面小摆件系统

基于 `STM32F103C8T6` 的桌面小摆件系统，采用三按键交互、OLED 菜单显示和多外设协同控制。当前工程已经实现启动界面、一级菜单、二级功能菜单、温湿度采集、蜂鸣器告警、LED 灯效以及计数/质数判断等功能，并保留了标准外设库、CMake/Make 构建链路和若干静态测试脚本。

本 README 不再只做“工程简介”，而是按课程设计说明书的写法，对系统结构、硬件接口、软件实现、关键函数和测试结果进行完整说明。文中所有描述均以当前仓库源码为准，而不是以历史版本或旧注释为准。

## 第1章 项目概述

### 1.1 项目目标

本系统面向“桌面小摆件”场景设计，目标是在一块 `STM32F103C8T6` 最小系统板上实现一个可交互、可显示、可感知环境信息的嵌入式综合实验平台。系统通过 `0.96` 寸 `OLED` 进行菜单和状态显示，通过三个独立按键完成功能切换，通过 `DHT11` 采集温湿度，通过 `TIM2` 与 `TIM3` 驱动 LED 和蜂鸣器，形成一个软硬件一体化的课程设计项目。

### 1.2 当前已实现功能

| 功能模块 | 当前实现方式 | 说明 |
| --- | --- | --- |
| 启动画面 | `main()` 中调用 `OLED_ShowString()` | 上电后显示 `"WeiJiKeShe"` 和组员缩写，停留 `3s` |
| 主菜单 | `menu()` + `a1/a2` 状态机 | 支持 5 个菜单项循环翻页和二级页面切换 |
| Hello 页面 | `a2 == 1` 分支 | 显示系统就绪信息 |
| 蜂鸣器页面 | `a2 == 2` 分支 + `TIM3_IRQHandler()` | 通过定时器中断步进播放告警节奏 |
| 温湿度页面 | `a2 == 3` 分支 + `DHT11_REC_Data()` | 每 `500ms` 读取一次温湿度并刷新显示 |
| LED 页面 | `a2 == 4` 分支 + `LED_Update()` | 双路 LED 交替闪烁，采用非阻塞状态机 |
| 计数/质数页面 | `a2 == 5` 分支 + `Is_Prime()` | 按键计数，实时判断当前值是否为质数 |

### 1.3 工程目录

```text
.
├── Start/                    # 启动文件、CMSIS、系统时钟配置
├── Library/                  # STM32F10x 标准外设库
├── System/                   # 通用系统函数（当前主要为 Delay）
├── Hardware/                 # OLED、DHT11、Buzzer、LED、PWM、Key 驱动
├── User/                     # 主程序、菜单逻辑、中断处理
├── cmake/                    # 交叉编译工具链文件
├── tests/                    # Bash 静态检查脚本
├── build/                    # 构建输出目录
├── CMakeLists.txt            # CMake 构建入口
├── Makefile                  # Make 构建入口
├── STM32F103C8Tx_FLASH.ld    # 链接脚本
└── download_cmsis_dap.sh     # 基于 pyOCD 的 CMSIS-DAP 下载脚本
```

### 1.4 主要引脚分配

| 模块 | 引脚 | 作用 | 源码位置 |
| --- | --- | --- | --- |
| KEY1 | `PA0` | 菜单向下/功能开启/计数加一 | `User/main.c`、`Hardware/Key.c` |
| KEY2 | `PC13` | 菜单向上/功能关闭/计数清零 | `User/main.c`、`Hardware/Key.c` |
| KEY3 | `PB15` | 进入二级菜单/返回主菜单 | `User/main.c`、`Hardware/Key.c` |
| OLED_SCL | `PB6` | 软件 I2C 时钟线 | `Hardware/OLED.c` |
| OLED_SDA | `PB7` | 软件 I2C 数据线 | `Hardware/OLED.c` |
| DHT11_DATA | `PB12` | 单总线温湿度数据线 | `Hardware/bsp_dht11.c` |
| BUZZER | `PA6` | `TIM3_CH1` PWM 输出 | `Hardware/Buzzer.c` |
| LED1 | `PA1` | `TIM2_CH2` PWM 输出 | `Hardware/PWM.c`、`Hardware/LED.c` |
| LED2 | `PA2` | `TIM2_CH3` PWM 输出 | `Hardware/PWM.c`、`Hardware/LED.c` |

## 第2章 总体方案设计

### 2.1 系统组成

系统整体采用“主控 + 显示 + 输入 + 传感 + 执行器”的结构：

```text
                +----------------------+
                |  STM32F103C8T6 MCU   |
                |  Cortex-M3 / 72MHz   |
                +----------+-----------+
                           |
      +--------------------+--------------------+
      |                    |                    |
  +---v---+            +---v---+            +---v---+
  | OLED  |            | Keys  |            | DHT11 |
  | PB6/7 |            | PA0   |            | PB12  |
  |        I2C(bit)    | PC13  |            | 1-wire|
  +-------+            | PB15  |            +---+---+
      |                +---+---+                |
      |                    |                    |
      |                菜单状态机            温湿度数据
      |                    |                    |
  +---v--------------------v--------------------v---+
  |                  User/main.c                   |
  |      启动画面、主菜单、二级菜单、业务逻辑        |
  +--------------------+--------------------------+
                           |
                 +---------+----------+
                 |                    |
             +---v---+            +---v---+
             | Buzzer|            |  LED  |
             | PA6   |            | PA1/2 |
             | TIM3  |            | TIM2  |
             +-------+            +-------+
```

软件上，系统采用分层设计：

1. `Start/` 负责启动、中断向量表和系统时钟。
2. `Library/` 提供 GPIO、RCC、TIM、NVIC 等标准外设库支持。
3. `System/` 提供与业务无关的公共函数，目前仅包含阻塞延时。
4. `Hardware/` 封装具体外设驱动。
5. `User/` 实现菜单状态机与业务页面逻辑。

### 2.2 系统初始化界面

系统上电后先进入 `main()`，其初始化顺序如下：

1. 调用 `OLED_Init()`，完成 OLED 上电延时、GPIO 开漏输出配置、SSD1306 命令初始化和清屏。
2. 调用 `PWM_Init()`，配置 `TIM2_CH2` 和 `TIM2_CH3` 用于 LED 输出。
3. 通过 `OLED_ShowString(1, 4, "WeiJiKeShe");` 与 `OLED_ShowString(2, 1, "ZZK XHB LP LBY");` 显示启动界面。
4. 调用 `Delay_ms(3000);` 让启动界面停留 3 秒。
5. 调用 `OLED_Clear();` 清屏，随后进入正常工作界面。
6. 再调用 `Buzzer_Init()`、`Key_Init()` 和 `Buzzer_OFF()`，确保蜂鸣器和按键处于已知初始状态。

启动界面在实现上完全由 `User/main.c` 控制，没有独立的开机界面模块，因此如果需要更换开机文字或显示时长，直接修改 `main()` 中对应的 `OLED_ShowString()` 与 `Delay_ms()` 即可。

### 2.3 系统主界面设计

主界面由 `menu()` 函数中的 `a2 == 0` 分支实现，其中：

1. 全局变量 `a1` 表示当前主菜单选项索引，取值范围 `0~4`。
2. 全局变量 `a2` 表示当前页面状态，`0` 为主菜单，`1~5` 为各二级菜单。
3. `main_menu_items` 数组保存 5 个主菜单文字：`"Hello!"`、`"Buzzer"`、`"Temp&Hum"`、`"LED"`、`"Count"`。
4. `Menu_GetItemLabel(int index)` 通过 `index % 5` 实现循环取项，因此主菜单可以显示“当前项 + 下一项预览”。
5. `last_a1` 与 `last_a2` 用于判断菜单是否真正发生变化，只有状态变化时才重绘，减少 OLED 频繁刷新造成的闪烁。

主界面的显示内容如下：

| 行号 | 显示内容 | 说明 |
| --- | --- | --- |
| 第 1 行 | `[SYS] MAIN HUB ` | 主菜单标题 |
| 第 2 行 | `> 当前菜单项` | 当前位置，前导 `>` 作为光标 |
| 第 3 行 | `  下一菜单项` | 下一项预览 |
| 第 4 行 | `K1K2:NAV K3:OK` | 按键提示 |

### 2.4 二级菜单设计

二级菜单由 `a2` 的不同取值区分，每个页面都在 `menu()` 函数内部实现。当前状态映射如下：

| `a2` 值 | 页面名称 | 触发方式 | 主要函数 |
| --- | --- | --- | --- |
| `1` | Hello 页面 | 主菜单选中 `Hello!` 后按 `KEY3` | `OLED_ShowString()` |
| `2` | 蜂鸣器页面 | 主菜单选中 `Buzzer` 后按 `KEY3` | `Buzzer_ON()`、`Buzzer_OFF()` |
| `3` | 温湿度页面 | 主菜单选中 `Temp&Hum` 后按 `KEY3` | `DHT11_REC_Data()`、`OLED_ShowNum()` |
| `4` | LED 页面 | 主菜单选中 `LED` 后按 `KEY3` | `LED1_ON()`、`LED2_ON()`、`LED_Update()` |
| `5` | 计数/质数页面 | 主菜单选中 `Count` 后按 `KEY3` | `Is_Prime()`、`OLED_ShowNum()` |

各页面的具体显示与行为如下。

#### 2.4.1 Hello 页面

当 `a2 == 1` 时，系统显示：

```text
[SYS] HELLO!
 WEIJI READY
 STM32 ONLINE
K3:BACK
```

该页面不执行外设控制，仅用于显示系统在线状态。

#### 2.4.2 蜂鸣器页面

当 `a2 == 2` 时，系统显示蜂鸣器状态界面。界面初始内容为：

```text
[AUDIO] BUZZER
 MODE: STBY
 KEY1 ON KEY2OFF
K1ON K2OFF K3BK
```

行为逻辑如下：

1. 检测 `PC13`，若按下则更新显示为 `MODE: OFF`、`AUDIO: MUTE`，并调用 `Buzzer_OFF()`。
2. 检测 `PA0`，若按下则更新显示为 `MODE: ON`、`AUDIO: LIVE`，并调用 `Buzzer_ON()`。
3. 蜂鸣器声效不是简单持续鸣叫，而是由 `TIM3` 中断驱动的节奏状态机：
   1. `0~19s` 为间歇短鸣，且静音间隔逐渐缩短。
   2. `19~20s` 为连续长鸣。
   3. `20~23s` 为伪随机低频爆炸噪声。
   4. 超过 `23s` 后自动调用 `Buzzer_OFF()` 停止。

#### 2.4.3 温湿度页面

当 `a2 == 3` 时，系统显示：

```text
[SENSE] TEMP
TEMP: xx.x C
HUM : xx.xx %
LIVE  K3:BACK
```

具体实现过程为：

1. 页面首次进入时写入标题和底部提示。
2. 每次循环先执行 `Delay_ms(500)`，形成约 500ms 的刷新周期。
3. 调用 `DHT11_REC_Data()` 完成一次 DHT11 通讯。
4. 从全局数组 `rec_data[4]` 中读出温湿度整数位和小数位。
5. 分别使用 `OLED_ShowNum()` 和 `OLED_ShowString()` 拼接显示温度与湿度。

#### 2.4.4 LED 页面

当 `a2 == 4` 时，系统显示：

```text
[LIGHT] LED
 MODE: STBY
 KEY1 ON KEY2OFF
K1ON K2OFF K3BK
```

当用户按键后：

1. 按 `KEY1`，将全局变量 `keynum` 置为 `1`，界面显示 `MODE: ON` 和 `LIGHT: ACTIVE`。
2. 按 `KEY2`，将 `keynum` 置为 `2`，界面显示 `MODE: OFF` 和 `LIGHT: IDLE`。
3. `keynum == 1` 时会调用 `LED1_ON()` 与 `LED2_ON()`，本质上是把 `led_enabled` 置为 `1`。
4. 真正的灯光变化由 `menu()` 顶部每轮调用一次的 `LED_Update()` 完成。

需要注意的是，当前 `LED_Update()` 实现的是“双灯交替闪烁”而不是呼吸灯：

1. `led_counter < 3` 时，`TIM_SetCompare2(TIM2, 0)`，`TIM_SetCompare3(TIM2, 100)`。
2. `led_counter >= 3` 时，`TIM_SetCompare2(TIM2, 100)`，`TIM_SetCompare3(TIM2, 0)`。
3. `led_counter >= 6` 时清零重新循环。

因此 LED 页面展示的是一种双通道交替闪烁效果。

#### 2.4.5 计数/质数页面

当 `a2 == 5` 时，系统显示：

```text
[COUNT] PRIME
NUM: xxx
FLAG: PRIME/NORMAL
K1:+1 K2:CLR
```

页面逻辑如下：

1. `PC13` 按下时将全局变量 `count` 清零。
2. `PA0` 按下时将 `count` 自增，若加到 `100` 则重新回到 `0`。
3. 调用 `Is_Prime(count)` 判断是否为质数。
4. 若为质数则显示 `FLAG: PRIME`，否则显示 `FLAG: NORMAL`。

### 2.5 按键控制逻辑

本系统的按键控制分为两层。

第一层是 `Hardware/Key.c` 中的基础驱动：

1. `Key_Init()` 将 `PA0`、`PC13`、`PB15` 配置为浮空输入。
2. `Key_GetNum()` 是一个基础阻塞扫描函数，可识别 `PA0` 和 `PC13`，返回键值 `1` 或 `2`。

第二层是 `User/main.c` 中直接面向菜单逻辑编写的两个函数：

1. `Key_GetNum1()` 仅在主菜单下使用：
   1. `PA0` 按下后执行 `a1++`，超出 `4` 后回到 `0`。
   2. `PC13` 按下后执行 `a1--`，小于 `0` 后回到 `4`。
   3. 两个分支都采用“检测按下 -> `Delay_ms(20)` 消抖 -> 等待释放 -> 再次消抖”的阻塞式方案。
2. `Key_GetNum2()` 负责菜单层级切换：
   1. `PB15` 按下时，若当前在主菜单，则执行 `a2 = a2 + 1 + a1` 进入对应子菜单。
   2. 若当前已经处于子菜单，则执行返回主菜单逻辑。
   3. 返回主菜单前会主动调用 `Buzzer_OFF()`、`LED1_OFF()`、`LED2_OFF()`，避免外设状态遗留。
   4. 该函数同时会清屏并重置 `last_a1`、`last_a2`，保证界面切换后强制刷新。

## 第3章 桌面小摆件系统硬件电路设计

### 3.1 STM32F103C8T6 单片机

本系统主控芯片为 `STM32F103C8T6`，其核心特性如下：

| 项目 | 参数 |
| --- | --- |
| CPU 内核 | ARM Cortex-M3 |
| 最高主频 | 72MHz |
| Flash | 64KB |
| SRAM | 20KB |
| 工作电压 | 2.0V ~ 3.6V |
| 常用外设 | GPIO、TIM、SysTick、NVIC、USART、I2C、SPI 等 |

工程的系统时钟配置由 `Start/system_stm32f10x.c` 中的 `SystemInit()` 完成。该文件会调用 `SetSysClock()`，在当前配置下选择 `SetSysClockTo72()`，使用外部高速晶振 `HSE` 配合 `PLL x9` 将系统主频配置到 `72MHz`。这也是后续 `Delay_us()` 中使用 `72 * xus` 作为 SysTick 重装值的根本依据。

### 3.2 DHT11 温湿度传感器电路设计

`DHT11` 通过 `PB12` 与单片机通信，采用单总线协议。当前驱动并未使用硬件单总线模块，而是用 GPIO 输入/输出模式切换软件实现时序。

电路与通信要点如下：

1. 数据线接至 `PB12`。
2. 主机发送起始信号时，需要先把引脚设为推挽输出。
3. 等待传感器响应时，再把引脚切回浮空输入。
4. 数据高低电平时长决定当前位是 `0` 还是 `1`。

对应函数为：

1. `DHT11_GPIO_Init_OUT()`：将 `PB12` 设为推挽输出。
2. `DHT11_GPIO_Init_IN()`：将 `PB12` 设为浮空输入。
3. `DHT11_Start()`：输出起始信号，顺序为拉高 `30us`、拉低 `20ms`、拉高 `30us`、切输入。
4. `DHT11_Rec_Byte()`：逐位接收一个字节，延时 `30us` 后采样，若仍为高电平则判为 `1`。
5. `DHT11_REC_Data()`：读取 5 字节数据并进行校验，校验通过后更新 `rec_data[4]`。

驱动中定义了 `DHT11_TIMEOUT`，在等待电平变化时使用超时计数保护，避免因传感器失联导致死循环。

### 3.3 蜂鸣器电路设计

蜂鸣器连接在 `PA6`，由 `TIM3_CH1` 输出 PWM 波形进行驱动。当前驱动设计比一般“开/关蜂鸣器”稍复杂，采用了“主循环触发 + 定时器中断步进”的结构。

#### 3.3.1 硬件与定时器配置

1. `Buzzer_Init()` 中先使能 `TIM3` 与 `GPIOA` 时钟。
2. 将 `PA6` 配置为复用推挽输出。
3. 配置 `TIM3` 预分频器为 `72 - 1`，使计数时钟变为 `1MHz`。
4. `TIM_OC1Init()` 将通道 1 配置为 `PWM1` 模式。
5. 使能 `TIM_IT_Update` 更新中断，并在 `NVIC` 中开启 `TIM3_IRQn`。

#### 3.3.2 频率和节拍实现

驱动中两个核心静态函数是：

1. `Buzzer_LoadFreqDuration(uint16_t freq, uint16_t duration_ms)`：
   1. 若 `freq == NOTE_REST`，则将 `ARR` 设为 `999`，比较值 `CCR1` 设为 `0`，形成静音段。
   2. 若 `freq != NOTE_REST`，则通过 `ARR = 1000000 / freq - 1` 计算 PWM 周期。
   3. 比较值取 `ARR / 2`，形成近似 `50%` 占空比。
   4. 使用 `duration_ms * freq / 1000` 计算目标 PWM 周期数，作为该音段持续时间。
2. `C4_GenerateNext()`：
   1. 根据累计时间 `c4_elapsed_ms` 生成下一段蜂鸣器行为。
   2. 前 19 秒生成越来越密集的短鸣。
   3. 19 到 20 秒生成长鸣。
   4. 20 到 23 秒利用线性同余伪随机数生成不同低频片段，模拟爆炸噪声。

#### 3.3.3 中断驱动

1. `Buzzer_ON()` 只负责初始化状态变量并使能 `TIM3`。
2. `TIM3_IRQHandler()` 在每次更新中断到来时调用 `Buzzer_UpdatePlayback()`。
3. `Buzzer_UpdatePlayback()` 通过 `buzzer_note_cycles` 统计当前片段已走过的 PWM 周期，达到目标后再调用 `C4_GenerateNext()` 切到下一片段。
4. `Buzzer_OFF()` 会关闭输出、清零状态并禁用 `TIM3`。

也就是说，蜂鸣器页面按下 `KEY1` 后，并不是在主循环里用延时播放，而是启动一个由中断自动推进的告警状态机。

### 3.4 OLED 显示电路

OLED 模块使用 `PB6` 和 `PB7` 软件模拟 I2C，相关实现位于 `Hardware/OLED.c`。

#### 3.4.1 引脚与底层时序

1. `OLED_W_SCL(x)` 和 `OLED_W_SDA(x)` 宏分别控制时钟线和数据线电平。
2. `OLED_I2C_Init()` 把两个引脚都配置成开漏输出，并将总线空闲态拉高。
3. `OLED_I2C_Start()`、`OLED_I2C_Stop()` 分别实现起始和停止条件。
4. `OLED_I2C_SendByte()` 逐位发送 8 位数据，不处理 ACK，应答被省略。

#### 3.4.2 命令与数据显示

1. `OLED_WriteCommand()` 发送命令帧，设备地址为 `0x78`，控制字节为 `0x00`。
2. `OLED_WriteData()` 发送数据帧，设备地址同样为 `0x78`，控制字节为 `0x40`。
3. `OLED_SetCursor(Y, X)` 通过页地址和列地址设置写入位置。
4. `OLED_Clear()` 逐页逐列写入 `0x00` 实现清屏。

#### 3.4.3 字符与数字显示

1. `OLED_ShowChar()` 使用 `OLED_F8x16` 字库显示单个字符，上下半区各写 8 个字节。
2. `OLED_ShowString()` 循环调用 `OLED_ShowChar()` 显示字符串。
3. `OLED_ShowNum()`、`OLED_ShowSignedNum()`、`OLED_ShowHexNum()`、`OLED_ShowBinNum()` 分别负责十进制无符号、有符号、十六进制和二进制数显示。
4. `OLED_Pow()` 用于求位权，辅助数字拆分。

#### 3.4.4 初始化过程

`OLED_Init()` 的执行流程为：

1. 先做双重空循环延时，等待屏幕上电稳定。
2. 调用 `OLED_I2C_Init()`。
3. 发送一系列 SSD1306 配置命令，包括显示时钟、多路复用率、COM 扫描方向、电荷泵和对比度等。
4. 调用 `OLED_Clear()` 清屏。

### 3.5 LED 与按键接口设计

虽然题目给出的章节目录没有单列 LED 和按键，但从当前系统实现看，这两部分是完整系统不可缺少的组成。

#### 3.5.1 LED PWM 接口

1. `PA1` 对应 `TIM2_CH2`。
2. `PA2` 对应 `TIM2_CH3`。
3. `PWM_Init()` 将两个引脚配置为复用推挽输出，并将 `TIM2` 的预分频器设为 `720 - 1`、自动重装值设为 `100 - 1`。
4. 因此 PWM 频率为 `72MHz / 720 / 100 = 1kHz`。
5. 初始比较值为 `100`，对应默认灭灯状态。

#### 3.5.2 LED 控制接口

1. `LED1_ON()` 与 `LED2_ON()` 并不直接改占空比，只是将 `led_enabled` 置为 `1`。
2. `LED1_OFF()` 与 `LED2_OFF()` 会清零计数器，并把 `CCR2`、`CCR3` 都置为 `100`。
3. `LED_Update()` 是真正的动态控制函数，用于在系统循环中推进灯效状态。

#### 3.5.3 按键接口

1. `PA0`、`PC13`、`PB15` 都被配置为浮空输入。
2. 当前应用层统一按照“读取到高电平即视为按下”的方式处理。
3. 所有菜单按键逻辑都采用阻塞式消抖，这种方式实现简单，但会牺牲部分系统并发性。

## 第4章 桌面小摆件系统软件设计

### 4.1 软件实现功能综述

软件整体围绕 `User/main.c` 中的 `menu()` 状态机展开。系统并未采用 RTOS，也没有独立任务调度器，而是使用“无限循环 + 页面分支 + 少量中断”的结构。

软件层次可以概括为：

1. 启动层：`startup_stm32f10x_md.S` 和 `system_stm32f10x.c`。
2. 中间层：`Delay.c`、标准外设库。
3. 驱动层：`OLED.c`、`bsp_dht11.c`、`PWM.c`、`LED.c`、`Buzzer.c`、`Key.c`。
4. 应用层：`main.c` 中的主循环、菜单状态和业务逻辑。
5. 中断层：`stm32f10x_it.c` 中的异常处理与 `TIM3_IRQHandler()`。

### 4.2 系统程序流程图设计

系统的软件主流程如下：

```text
系统上电
   |
   v
SystemInit() 配置 72MHz 时钟
   |
   v
main()
   |
   +--> OLED_Init()
   +--> PWM_Init()
   +--> 启动画面显示 3s
   +--> OLED_Clear()
   +--> Buzzer_Init()
   +--> Key_Init()
   +--> Buzzer_OFF()
   |
   v
while(1)
   |
   v
menu()
   |
   +--> LED_Update()
   +--> 如果 a2 == 0，则 Key_GetNum1() 处理主菜单翻页
   +--> Key_GetNum2() 处理进入/返回
   +--> 根据 a2 进入不同页面分支
           |
           +--> a2=0: 主菜单显示
           +--> a2=1: Hello 页面
           +--> a2=2: 蜂鸣器控制
           +--> a2=3: 温湿度采集显示
           +--> a2=4: LED 控制
           +--> a2=5: 计数与质数判断
```

蜂鸣器部分还有一条独立于主循环之外的中断流程：

```text
用户在蜂鸣器页面按下 KEY1
   |
   v
Buzzer_ON()
   |
   v
TIM3 使能并周期性产生更新中断
   |
   v
TIM3_IRQHandler()
   |
   v
Buzzer_UpdatePlayback()
   |
   v
C4_GenerateNext() 生成下一段声音或静音
```

### 4.3 核心源文件与函数实现

下面按照源码文件对关键函数进行逐一说明。

#### 4.3.1 `System/Delay.c`

| 函数 | 作用 | 实现细节 |
| --- | --- | --- |
| `Delay_us(uint32_t xus)` | 微秒级延时 | 将 `SysTick->LOAD` 设为 `72 * xus`，时钟源取 `HCLK`，轮询 `COUNTFLAG`，完成一次阻塞延时 |
| `Delay_ms(uint32_t xms)` | 毫秒级延时 | 循环调用 `Delay_us(1000)` |
| `Delay_s(uint32_t xs)` | 秒级延时 | 循环调用 `Delay_ms(1000)` |

#### 4.3.2 `Hardware/OLED.c`

| 函数 | 作用 | 实现细节 |
| --- | --- | --- |
| `OLED_I2C_Init()` | 初始化软件 I2C 引脚 | `PB6/PB7` 开漏输出，空闲时拉高 |
| `OLED_I2C_Start()` | I2C 起始信号 | 先 SDA 高、SCL 高，再 SDA 拉低、SCL 拉低 |
| `OLED_I2C_Stop()` | I2C 停止信号 | SDA 低、SCL 高、SDA 高 |
| `OLED_I2C_SendByte()` | 发送 1 字节 | 从最高位到最低位逐位输出 |
| `OLED_WriteCommand()` | 发送命令 | 地址 `0x78`，控制字节 `0x00` |
| `OLED_WriteData()` | 发送数据 | 地址 `0x78`，控制字节 `0x40` |
| `OLED_SetCursor()` | 设置页和列地址 | 采用 SSD1306 页寻址模式 |
| `OLED_Clear()` | 全屏清零 | 8 页 x 128 列全部写 `0x00` |
| `OLED_ShowChar()` | 显示单字符 | 读取 `OLED_F8x16` 字库，上下两页分别写入 |
| `OLED_ShowString()` | 显示字符串 | 对字符串逐字符调用 `OLED_ShowChar()` |
| `OLED_ShowNum()` | 显示十进制无符号数 | 利用 `OLED_Pow()` 计算位权并逐位显示 |
| `OLED_ShowSignedNum()` | 显示带符号数 | 先显示正负号，再显示绝对值 |
| `OLED_ShowHexNum()` | 显示十六进制 | 大于 9 的值转成 `A~F` |
| `OLED_ShowBinNum()` | 显示二进制 | 每位仅显示 `0` 或 `1` |
| `OLED_Init()` | OLED 总初始化 | 上电等待 -> GPIO 初始化 -> SSD1306 命令配置 -> 清屏 |

#### 4.3.3 `Hardware/PWM.c`

| 函数 | 作用 | 实现细节 |
| --- | --- | --- |
| `PWM_Init()` | 初始化 TIM2 PWM 输出 | `PA1/PA2` 复用推挽输出，`TIM2` 内部时钟，`PSC=719`，`ARR=99`，`CH2/CH3` 使用 `PWM1` 模式 |

#### 4.3.4 `Hardware/LED.c`

| 函数 | 作用 | 实现细节 |
| --- | --- | --- |
| `LED1_ON()` | 使能 LED 动态效果 | 将 `led_enabled` 置为 `1` |
| `LED1_OFF()` | 关闭 LED 效果 | `led_enabled=0`，`led_counter=0`，并将 `CCR2/CCR3` 置为 `100` |
| `LED2_ON()` | 同 `LED1_ON()` | 逻辑一致 |
| `LED2_OFF()` | 同 `LED1_OFF()` | 逻辑一致 |
| `LED_Update()` | 推进 LED 效果 | 根据 `led_counter` 在两个通道间交替切换比较值，形成双灯交替闪烁 |

#### 4.3.5 `Hardware/Buzzer.c`

| 函数 | 作用 | 实现细节 |
| --- | --- | --- |
| `Buzzer_Init()` | 初始化蜂鸣器驱动 | 配置 `PA6` 为 `TIM3_CH1` 输出，打开更新中断与 NVIC |
| `Buzzer_LoadFreqDuration()` | 装载新的频率和持续时间 | 静态函数，计算 `ARR`、`CCR1` 和目标周期数 |
| `C4_GenerateNext()` | 生成下一段告警片段 | 静态函数，根据累计时长切换短鸣、长鸣、爆炸噪声 |
| `Buzzer_UpdatePlayback()` | 中断步进播放状态 | 每次更新中断累加已播放周期，到达目标后切换下一段 |
| `Buzzer_ON()` | 启动告警 | 初始化状态变量并使能 `TIM3` |
| `Buzzer_OFF()` | 停止告警 | 关闭输出、清空状态并禁用定时器 |

#### 4.3.6 `Hardware/bsp_dht11.c`

| 函数 | 作用 | 实现细节 |
| --- | --- | --- |
| `DHT11_GPIO_Init_OUT()` | 配置输出模式 | `PB12` 推挽输出 |
| `DHT11_GPIO_Init_IN()` | 配置输入模式 | `PB12` 浮空输入 |
| `DHT11_Start()` | 发送起始时序 | 拉高 `30us` -> 拉低 `20ms` -> 拉高 `30us` -> 切输入 |
| `DHT11_Rec_Byte()` | 接收 8 位数据 | 每一位在等待高电平后延时 `30us` 采样 |
| `DHT11_REC_Data()` | 接收 40 位完整数据 | 顺序读取湿度高位、湿度低位、温度高位、温度低位和校验和，校验通过后更新 `rec_data[4]` |

#### 4.3.7 `Hardware/Key.c`

| 函数 | 作用 | 实现细节 |
| --- | --- | --- |
| `Key_Init()` | 初始化按键引脚 | 配置 `PA0`、`PC13`、`PB15` 为浮空输入 |
| `Key_GetNum()` | 通用键值扫描 | 检测 `PA0` 和 `PC13`，阻塞等待释放后返回键值 |

#### 4.3.8 `User/main.c`

`User/main.c` 是整个工程最核心的应用层文件，主要包含以下内容。

| 函数/变量 | 作用 | 实现细节 |
| --- | --- | --- |
| `a1` | 主菜单索引 | `0~4`，决定当前高亮菜单项 |
| `a2` | 页面状态值 | `0` 为主菜单，`1~5` 为二级菜单 |
| `keynum` | LED 控制状态 | `1` 表示开，`2` 表示关 |
| `count` | 计数器值 | 在计数页面 `0~99` 循环 |
| `last_a1/last_a2` | 上次状态缓存 | 用于减少重复刷新 |
| `main_menu_items` | 菜单标签数组 | 保存五个一级菜单文字 |
| `Menu_GetItemLabel()` | 获取菜单标签 | 使用取模实现循环预览 |
| `Key_GetNum1()` | 一级菜单翻页 | `PA0` 前进，`PC13` 后退 |
| `Key_GetNum2()` | 菜单层级切换 | `PB15` 在进入和返回之间切换 |
| `menu()` | 菜单状态机 | 根据 `a2` 决定当前页面显示和交互 |
| `Is_Prime()` | 质数判断 | 特判 `0/1/2`，偶数快速返回，奇数从 `3` 枚举到 `sqrt(n)` |
| `main()` | 程序入口 | 负责初始化、启动界面和主循环 |

其中 `Is_Prime()` 的算法流程如下：

1. 若 `n <= 1`，直接返回 `false`。
2. 若 `n == 2`，返回 `true`。
3. 若 `n` 为偶数，直接返回 `false`。
4. 从 `3` 开始以步长 `2` 枚举奇数因子，条件为 `i * i <= n`。
5. 只要发现可整除因子就返回 `false`，否则返回 `true`。

#### 4.3.9 `User/stm32f10x_it.c`

| 函数 | 作用 | 实现细节 |
| --- | --- | --- |
| `NMI_Handler()` | NMI 异常 | 空实现 |
| `HardFault_Handler()` | 硬故障处理 | 死循环 |
| `MemManage_Handler()` | 存储器故障处理 | 死循环 |
| `BusFault_Handler()` | 总线故障处理 | 死循环 |
| `UsageFault_Handler()` | 用法故障处理 | 死循环 |
| `SVC_Handler()` | 系统服务调用 | 空实现 |
| `DebugMon_Handler()` | 调试监视器 | 空实现 |
| `PendSV_Handler()` | PendSV | 空实现 |
| `SysTick_Handler()` | SysTick 中断 | 当前为空，说明延时函数采用轮询而非中断计时 |
| `TIM3_IRQHandler()` | 蜂鸣器播放中断 | 清除更新中断标志后调用 `Buzzer_UpdatePlayback()` |

### 4.4 软件设计特点与局限

当前软件设计具有以下特点：

1. 结构清晰，适合课程设计和裸机实验教学。
2. 菜单逻辑简单直接，阅读成本低。
3. 蜂鸣器采用中断步进，比纯延时播放更灵活。
4. LED 采用非阻塞状态机，不会在点灯时完全卡住主循环。

同时也存在一些局限：

1. 大量按键逻辑使用阻塞式消抖和等待释放，会影响实时性。
2. 温湿度页面固定 `Delay_ms(500)`，进入该页面后系统响应节奏明显变慢。
3. `menu()` 单文件承担的职责较多，后续若继续扩展功能，建议拆分页面函数。
4. 仓库中的部分测试脚本与当前实现存在漂移，需同步维护。

## 第5章 桌面小摆件系统测试

### 5.1 测试环境

本仓库当前可分为两类测试环境：

1. 源码静态检查环境：
   1. 操作系统：Linux Bash 环境。
   2. 测试脚本位置：`tests/`。
   3. 测试方式：通过 `grep`、`sed` 和脚本逻辑检查源码中是否存在预期实现。
2. 硬件联调环境：
   1. `STM32F103C8T6` 最小系统板。
   2. `DHT11` 温湿度传感器。
   3. `0.96` 寸 I2C OLED。
   4. 两路 LED。
   5. 有源蜂鸣器。
   6. `CMSIS-DAP`、`ST-Link` 或其他 SWD 下载器。

本次文档整理过程中，已实际执行仓库现有 Bash 测试脚本，用于给出“当前工程状态”的真实结果。

### 5.2 测试内容

当前仓库提供的自动化脚本如下：

| 脚本 | 测试目标 |
| --- | --- |
| `tests/test_menu_ui.sh` | 检查主菜单和各页面关键字符串是否存在 |
| `tests/test_buzzer_control.sh` | 检查蜂鸣器是否采用 `TIM3` 中断步进实现 |
| `tests/test_led_control.sh` | 检查 LED 非阻塞状态机函数是否存在 |
| `tests/test_boot_screen.sh` | 检查启动界面文字和停留时间 |
| `tests/test_download.sh` | 检查下载脚本是否存在并具备预期调用流程 |

若进行板级联调，建议额外覆盖以下人工测试项：

1. 上电后启动界面是否完整显示 3 秒。
2. 主菜单是否能正常翻页并循环。
3. 各二级菜单进入和返回是否正常。
4. 温湿度数据是否能稳定刷新。
5. 蜂鸣器是否能启动告警并在返回主菜单时关闭。
6. LED 是否能在页面内交替闪烁并在返回后熄灭。
7. 计数器是否能正确加一、清零并显示质数判断结果。

### 5.3 测试结果

本次在当前仓库中执行现有 Bash 测试脚本后的结果如下：

| 测试脚本 | 结果 | 说明 |
| --- | --- | --- |
| `tests/test_menu_ui.sh` | 通过 | 当前菜单字符串与源码一致 |
| `tests/test_buzzer_control.sh` | 通过 | 已确认 `TIM3_IRQHandler()` 和 `Buzzer_UpdatePlayback()` 存在 |
| `tests/test_led_control.sh` | 通过 | 已确认 `LED_Update()` 存在且为非阻塞实现 |
| `tests/test_boot_screen.sh` | 未通过 | 脚本期望 `"weijikeshe"`，源码实际为 `"WeiJiKeShe"` |
| `tests/test_download.sh` | 未通过 | 脚本期望存在 `download.sh`，仓库实际脚本为 `download_cmsis_dap.sh` |

因此，从当前源码一致性角度看：

1. 菜单界面、蜂鸣器中断实现和 LED 状态机设计是匹配的。
2. 启动画面测试脚本需要与当前代码同步。
3. 下载脚本测试也需要与实际文件名同步。

换言之，当前自动化测试暴露的问题主要是“测试脚本与源码的命名和文案漂移”，不是核心控制逻辑缺失。

## 构建与烧录说明

### 1. CMake 构建

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

若需要发布版本：

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### 2. Make 构建

```bash
make
```

### 3. OpenOCD 烧录

若已正确安装 `openocd` 且使用 `ST-Link`，可执行：

```bash
cmake --build build --target flash
```

或：

```bash
make flash
```

### 4. CMSIS-DAP 下载

当前仓库中实际存在的下载脚本为：

```bash
./download_cmsis_dap.sh build/Project.bin
```

该脚本会：

1. 自动检测 `pyocd`。
2. 对芯片执行整片擦除。
3. 将 `Project.bin` 烧录到 `0x08000000`。
4. 发送复位命令启动目标板。

## 附录A 关键源码文件说明

| 文件 | 作用 |
| --- | --- |
| `Start/startup_stm32f10x_md.S` | 启动汇编文件，定义中断向量表并跳转到 `main()` |
| `Start/system_stm32f10x.c` | 系统时钟初始化 |
| `System/Delay.c` | 阻塞延时 |
| `Hardware/OLED.c` | OLED 软件 I2C 与显示接口 |
| `Hardware/PWM.c` | LED 对应的 `TIM2` PWM 初始化 |
| `Hardware/LED.c` | LED 动态效果状态机 |
| `Hardware/Buzzer.c` | 蜂鸣器 PWM 与中断播放状态机 |
| `Hardware/bsp_dht11.c` | DHT11 单总线驱动 |
| `Hardware/Key.c` | 按键初始化与基础扫描 |
| `User/main.c` | 菜单状态机和系统主逻辑 |
| `User/stm32f10x_it.c` | 中断服务函数 |

## 附录B 当前工程中需要注意的两处命名漂移

1. 启动画面字符串当前实现为 `"WeiJiKeShe"`，若测试脚本继续使用 `"weijikeshe"` 作为断言，测试将失败。
2. 构建系统和测试脚本里仍有部分位置引用 `download.sh`，而仓库内当前实际脚本名为 `download_cmsis_dap.sh`。

如果后续继续完善课程设计文档，建议优先同步这两处，以保证 README、源码、构建脚本和测试脚本四者保持一致。
