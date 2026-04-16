#!/bin/bash

# 测试脚本：蜂鸣器控制检查
# 验证蜂鸣器是否采用TIM3中断步进实现

echo "========================================="
echo "测试 2: 蜂鸣器控制检查"
echo "========================================="

BUZZER_C="Hardware/Buzzer.c"
MAIN_C="User/main.c"
IT_C="User/stm32f10x_it.c"
PASSED=0
FAILED=0

# 检查文件是否存在
for file in "$BUZZER_C" "$MAIN_C" "$IT_C"; do
    if [ ! -f "$file" ]; then
        echo "❌ 错误: 找不到 $file"
        exit 1
    fi
done

# 测试用例1: 检查TIM3初始化
echo "[1/8] 检查TIM3初始化..."
if grep -q "TIM3" "$BUZZER_C" && grep -q "RCC_APB1Periph_TIM3" "$BUZZER_C"; then
    echo "   ✅ 通过: 找到TIM3时钟使能"
    ((PASSED++))
else
    echo "   ❌ 失败: 未找到TIM3初始化"
    ((FAILED++))
fi

# 测试用例2: 检查PWM配置
echo "[2/8] 检查PWM配置..."
if grep -q "TIM_OCMode_PWM1" "$BUZZER_C" && grep -q "GPIO_Mode_AF_PP" "$BUZZER_C"; then
    echo "   ✅ 通过: 找到PWM配置"
    ((PASSED++))
else
    echo "   ❌ 失败: 未找到PWM配置"
    ((FAILED++))
fi

# 测试用例3: 检查中断使能
echo "[3/8] 检查中断使能..."
if grep -q "TIM_ITConfig" "$BUZZER_C" && grep -q "TIM_IT_Update" "$BUZZER_C"; then
    echo "   ✅ 通过: 找到更新中断使能"
    ((PASSED++))
else
    echo "   ❌ 失败: 未找到中断使能"
    ((FAILED++))
fi

# 测试用例4: 检查NVIC配置
echo "[4/8] 检查NVIC配置..."
if grep -q "TIM3_IRQn" "$BUZZER_C"; then
    echo "   ✅ 通过: 找到TIM3_IRQn配置"
    ((PASSED++))
else
    echo "   ❌ 失败: 未找到NVIC配置"
    ((FAILED++))
fi

# 测试用例5: 检查Buzzer_UpdatePlayback函数
echo "[5/8] 检查Buzzer_UpdatePlayback函数..."
if grep -q "void Buzzer_UpdatePlayback(void)" "$BUZZER_C"; then
    echo "   ✅ 通过: 找到Buzzer_UpdatePlayback函数"
    ((PASSED++))
else
    echo "   ❌ 失败: 未找到Buzzer_UpdatePlayback函数"
    ((FAILED++))
fi

# 测试用例6: 检查C4_GenerateNext状态机
echo "[6/8] 检查C4_GenerateNext状态机..."
if grep -q "C4_GenerateNext" "$BUZZER_C"; then
    echo "   ✅ 通过: 找到C4_GenerateNext状态机"
    ((PASSED++))
else
    echo "   ❌ 失败: 未找到C4_GenerateNext状态机"
    ((FAILED++))
fi

# 测试用例7: 检查中断服务函数
echo "[7/8] 检查中断服务函数..."
if grep -q "TIM3_IRQHandler" "$IT_C"; then
    echo "   ✅ 通过: 找到TIM3_IRQHandler"
    ((PASSED++))
else
    echo "   ❌ 失败: 未找到TIM3_IRQHandler"
    ((FAILED++))
fi

# 测试用例8: 检查main.c中的蜂鸣器控制
echo "[8/8] 检查main.c中的蜂鸣器控制..."
if grep -q "Buzzer_ON" "$MAIN_C" && grep -q "Buzzer_OFF" "$MAIN_C"; then
    echo "   ✅ 通过: 找到Buzzer_ON和Buzzer_OFF调用"
    ((PASSED++))
else
    echo "   ❌ 失败: 未找到蜂鸣器控制调用"
    ((FAILED++))
fi

echo ""
echo "========================================="
echo "测试结果: $PASSED 通过, $FAILED 失败"
echo "========================================="

if [ $FAILED -eq 0 ]; then
    echo "✅ 所有测试通过!"
    exit 0
else
    echo "❌ 部分测试失败!"
    exit 1
fi
