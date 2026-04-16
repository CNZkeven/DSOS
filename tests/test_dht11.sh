#!/bin/bash

# 测试脚本：DHT11温湿度传感器检查
# 验证DHT11驱动的实现

echo "========================================="
echo "测试 6: DHT11温湿度传感器检查"
echo "========================================="

DHT11_C="Hardware/bsp_dht11.c"
MAIN_C="User/main.c"
PASSED=0
FAILED=0

# 检查文件是否存在
for file in "$DHT11_C" "$MAIN_C"; do
    if [ ! -f "$file" ]; then
        echo "❌ 错误: 找不到 $file"
        exit 1
    fi
done

# 测试用例1: 检查DHT11_REC_Data函数
echo "[1/7] 检查DHT11_REC_Data函数..."
if grep -q "DHT11_REC_Data" "$DHT11_C"; then
    echo "   ✅ 通过: 找到DHT11_REC_Data函数"
    ((PASSED++))
else
    echo "   ❌ 失败: 未找到DHT11_REC_Data函数"
    ((FAILED++))
fi

# 测试用例2: 检查超时保护
echo "[2/7] 检查超时保护..."
if grep -q "DHT11_TIMEOUT" "$DHT11_C" && grep -q "timeout" "$DHT11_C"; then
    echo "   ✅ 通过: 找到超时保护机制"
    ((PASSED++))
else
    echo "   ❌ 失败: 未找到超时保护"
    ((FAILED++))
fi

# 测试用例3: 检查数据校验
echo "[3/7] 检查数据校验..."
if grep -q "CHECK" "$DHT11_C" && grep -q "R_H + R_L + T_H + T_L" "$DHT11_C"; then
    echo "   ✅ 通过: 找到数据校验逻辑"
    ((PASSED++))
else
    echo "   ❌ 失败: 未找到数据校验"
    ((FAILED++))
fi

# 测试用例4: 检查GPIO模式切换
echo "[4/7] 检查GPIO模式切换..."
if grep -q "DHT11_GPIO_Init_OUT" "$DHT11_C" && grep -q "DHT11_GPIO_Init_IN" "$DHT11_C"; then
    echo "   ✅ 通过: 找到GPIO输入输出模式切换"
    ((PASSED++))
else
    echo "   ❌ 失败: 未找到GPIO模式切换"
    ((FAILED++))
fi

# 测试用例5: 检查起始信号
echo "[5/7] 检查起始信号..."
if grep -q "DHT11_Start" "$DHT11_C" && grep -q "Delay_ms(20)" "$DHT11_C"; then
    echo "   ✅ 通过: 找到起始信号发送"
    ((PASSED++))
else
    echo "   ❌ 失败: 未找到起始信号"
    ((FAILED++))
fi

# 测试用例6: 检查全局数据数组
echo "[6/7] 检查全局数据数组..."
if grep -q "rec_data\[4\]" "$DHT11_C"; then
    echo "   ✅ 通过: 找到rec_data全局数组"
    ((PASSED++))
else
    echo "   ❌ 失败: 未找到rec_data数组"
    ((FAILED++))
fi

# 测试用例7: 检查main.c中的调用
echo "[7/7] 检查main.c中的调用..."
if grep -q "DHT11_REC_Data()" "$MAIN_C"; then
    echo "   ✅ 通过: 找到DHT11_REC_Data调用"
    ((PASSED++))
else
    echo "   ❌ 失败: 未找到DHT11调用"
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
