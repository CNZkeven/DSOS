######################################
# 项目名称
######################################
TARGET = Project

######################################
# 编译工具链
######################################
PREFIX = arm-none-eabi-
CC  = $(PREFIX)gcc
AS  = $(PREFIX)gcc -x assembler-with-cpp
CP  = $(PREFIX)objcopy
SZ  = $(PREFIX)size
HEX = $(CP) -O ihex
BIN = $(CP) -O binary -S

######################################
# 编译输出目录
######################################
BUILD_DIR = build

######################################
# 源文件
######################################

# C 源文件
C_SOURCES = \
Start/core_cm3.c \
Start/system_stm32f10x.c \
Library/misc.c \
Library/stm32f10x_adc.c \
Library/stm32f10x_bkp.c \
Library/stm32f10x_can.c \
Library/stm32f10x_cec.c \
Library/stm32f10x_crc.c \
Library/stm32f10x_dac.c \
Library/stm32f10x_dbgmcu.c \
Library/stm32f10x_dma.c \
Library/stm32f10x_exti.c \
Library/stm32f10x_flash.c \
Library/stm32f10x_fsmc.c \
Library/stm32f10x_gpio.c \
Library/stm32f10x_i2c.c \
Library/stm32f10x_iwdg.c \
Library/stm32f10x_pwr.c \
Library/stm32f10x_rcc.c \
Library/stm32f10x_rtc.c \
Library/stm32f10x_sdio.c \
Library/stm32f10x_spi.c \
Library/stm32f10x_tim.c \
Library/stm32f10x_usart.c \
Library/stm32f10x_wwdg.c \
System/Delay.c \
Hardware/LED.c \
Hardware/OLED.c \
Hardware/Buzzer.c \
Hardware/Key.c \
Hardware/PWM.c \
Hardware/bsp_dht11.c \
Hardware/key/bsp_gpio_key.c \
User/main.c \
User/stm32f10x_it.c

# ASM 源文件 (GCC 格式，大写 .S 会自动预处理)
ASM_SOURCES = \
Start/startup_stm32f10x_md.S

######################################
# 头文件目录
######################################
C_INCLUDES = \
-IStart \
-ILibrary \
-IUser \
-ISystem \
-IHardware \
-IHardware/key

######################################
# 预编译宏
######################################
C_DEFS = \
-DUSE_STDPERIPH_DRIVER \
-DSTM32F10X_MD

######################################
# MCU 配置
######################################
CPU = -mcpu=cortex-m3
FPU =
FLOAT-ABI =
MCU = $(CPU) -mthumb $(FPU) $(FLOAT-ABI)

######################################
# 编译选项
######################################

# C 编译标志
CFLAGS = $(MCU) $(C_DEFS) $(C_INCLUDES) -std=c99 -Wall -fdata-sections -ffunction-sections

# 调试版本 (默认) / 发布版本
ifdef RELEASE
CFLAGS += -Os
else
CFLAGS += -Og -g -gdwarf-2
endif

# 生成依赖文件
CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)"

# 汇编编译标志
ASFLAGS = $(MCU) -Wall -fdata-sections -ffunction-sections

######################################
# 链接选项
######################################

# 链接脚本
LDSCRIPT = STM32F103C8Tx_FLASH.ld

# 链接库
LIBS = -lc -lm -lnosys
LIBDIR =

# 链接标志
LDFLAGS = $(MCU) -specs=nano.specs -T$(LDSCRIPT) $(LIBDIR) $(LIBS) \
          -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections

######################################
# 构建目标
######################################

# 默认目标
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin

# 目标文件列表
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))

OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.S=.o)))
vpath %.S $(sort $(dir $(ASM_SOURCES)))

# 编译 C 文件
$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) $< -o $@

# 编译汇编文件
$(BUILD_DIR)/%.o: %.S Makefile | $(BUILD_DIR)
	$(AS) -c $(ASFLAGS) $< -o $@

# 链接
$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) Makefile
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SZ) $@

# 生成 HEX 文件
$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(HEX) $< $@

# 生成 BIN 文件
$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(BIN) $< $@

# 创建输出目录
$(BUILD_DIR):
	mkdir -p $@

######################################
# 烧录 (需要 OpenOCD + ST-Link)
######################################
flash: $(BUILD_DIR)/$(TARGET).bin
	openocd -f interface/stlink-v2.cfg -f target/stm32f1x.cfg \
	        -c "program $(BUILD_DIR)/$(TARGET).elf verify reset exit"

######################################
# 下载 (需要 J-Link)
######################################
download: $(BUILD_DIR)/$(TARGET).bin
	./download.sh $(BUILD_DIR)/$(TARGET).bin

######################################
# 清理
######################################
clean:
	-rm -rf $(BUILD_DIR)

######################################
# 依赖文件
######################################
-include $(wildcard $(BUILD_DIR)/*.d)

######################################
# 伪目标
######################################
.PHONY: all clean flash download
