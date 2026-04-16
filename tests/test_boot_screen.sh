#!/bin/bash

# 测试脚本：启动画面检查
# 验证启动界面文字和停留时间

echo "========================================="
echo "测试 4: 启动画面检查"
echo "========================================="

MAIN_C="User/main.c"
PASSED=0
FAILED=0

# 检查文件是否存在
if [ ! -f "$MAIN_C" ]; then
    echo "❌ 错误: 找不到 $MAIN_C"
    exit 1
fi

# 测试用例1: 检查启动画面文字
echo "[1/4] 检查启动画面文字..."
if grep -q '"WeiJiKeShe"' "$MAIN_C"; then
    echo "   ✅ 通过: 找到启动画面文字 'WeiJiKeShe'"
    ((PASSED++))
elif grep -q '"weijikeshe"' "$MAIN_C"; then
    echo "   ⚠️  警告: 找到 'weijikeshe' (小写), 但预期是 'WeiJiKeShe'"
    ((PASSED++))
else
    echo "   ❌ 失败: 未找到启动画面文字"
    ((FAILED++))
fi

# 测试用例2: 检查组员姓名显示
echo "[2/4] 检查组员姓名显示..."
if grep -q 'OLED_ShowString(2, 1' "$MAIN_C"; then
    echo "   ✅ 通过: 找到第二行显示调用"
    ((PASSED++))
else
    echo "   ❌ 失败: 未找到第二行显示"
    ((FAILED++))
fi

# 测试用例3: 检查3秒延时
echo "[3/4] 检查3秒延时..."
if grep -q "Delay_ms(3000)" "$MAIN_C"; then
    echo "   ✅ 通过: 找到3000ms延时"
    ((PASSED++))
elif grep -q "Delay_s(3)" "$MAIN_C"; then
    echo "   ✅ 通过: 找到3秒延时"
    ((PASSED++))
else
    echo "   ❌ 失败: 未找到3秒延时"
    ((FAILED++))
fi

# 测试用例4: 检查清屏操作
echo "[4/4] 检查清屏操作..."
if grep -q "OLED_Clear" "$MAIN_C"; then
    echo "   ✅ 通过: 找到OLED_Clear调用"
    ((PASSED++))
else
    echo "   ❌ 失败: 未找到清屏操作"
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
