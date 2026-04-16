#!/bin/bash

# 测试脚本：LED控制检查
# 验证LED非阻塞状态机函数是否存在

echo "========================================="
echo "测试 3: LED控制检查"
echo "========================================="

LED_C="Hardware/LED.c"
MAIN_C="User/main.c"
PWM_C="Hardware/PWM.c"
PASSED=0
FAILED=0

# 检查文件是否存在
for file in "$LED_C" "$MAIN_C" "$PWM_C"; do
    if [ ! -f "$file" ]; then
        echo "❌ 错误: 找不到 $file"
        exit 1
    fi
done

# 测试用例1: 检查LED_Update函数
echo "[1/7] 检查LED_Update函数..."
if grep -q "void LED_Update(void)" "$LED_C"; then
    echo "   ✅ 通过: 找到LED_Update函数"
    ((PASSED++))
else
    echo "   ❌ 失败: 未找到LED_Update函数"
    ((FAILED++))
fi

# 测试用例2: 检查非阻塞设计（无Delay调用）
echo "[2/7] 检查非阻塞设计..."
if ! grep -q "Delay_" "$LED_C"; then
    echo "   ✅ 通过: LED.c中无阻塞延时调用"
    ((PASSED++))
else
    echo "   ❌ 失败: LED.c中存在阻塞延时"
    ((FAILED++))
fi

# 测试用例3: 检查状态变量
echo "[3/7] 检查状态变量..."
if grep -q "led_enabled" "$LED_C" && grep -q "led_counter" "$LED_C"; then
    echo "   ✅ 通过: 找到led_enabled和led_counter状态变量"
    ((PASSED++))
else
    echo "   ❌ 失败: 未找到状态变量"
    ((FAILED++))
fi

# 测试用例4: 检查PWM控制
echo "[4/7] 检查PWM控制..."
if grep -q "TIM_SetCompare2" "$LED_C" && grep -q "TIM_SetCompare3" "$LED_C"; then
    echo "   ✅ 通过: 找到PWM比较值设置"
    ((PASSED++))
else
    echo "   ❌ 失败: 未找到PWM控制"
    ((FAILED++))
fi

# 测试用例5: 检查LED开关函数
echo "[5/7] 检查LED开关函数..."
if grep -q "void LED1_ON" "$LED_C" && grep -q "void LED1_OFF" "$LED_C" && \
   grep -q "void LED2_ON" "$LED_C" && grep -q "void LED2_OFF" "$LED_C"; then
    echo "   ✅ 通过: 找到所有LED开关函数"
    ((PASSED++))
else
    echo "   ❌ 失败: 未找到完整的LED开关函数"
    ((FAILED++))
fi

# 测试用例6: 检查main.c中LED_Update调用
echo "[6/7] 检查main.c中LED_Update调用..."
if grep -q "LED_Update()" "$MAIN_C"; then
    echo "   ✅ 通过: 找到LED_Update调用"
    ((PASSED++))
else
    echo "   ❌ 失败: 未找到LED_Update调用"
    ((FAILED++))
fi

# 测试用例7: 检查PWM初始化
echo "[7/7] 检查PWM初始化..."
if grep -q "TIM2" "$PWM_C" && grep -q "PWM_Init" "$PWM_C"; then
    echo "   ✅ 通过: 找到TIM2 PWM初始化"
    ((PASSED++))
else
    echo "   ❌ 失败: 未找到PWM初始化"
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
