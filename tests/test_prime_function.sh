#!/bin/bash

# 测试脚本：质数判断函数测试
# 验证Is_Prime函数的实现

echo "========================================="
echo "测试 5: 质数判断函数测试"
echo "========================================="

MAIN_C="User/main.c"
PASSED=0
FAILED=0

# 检查文件是否存在
if [ ! -f "$MAIN_C" ]; then
    echo "❌ 错误: 找不到 $MAIN_C"
    exit 1
fi

# 测试用例1: 检查Is_Prime函数是否存在
echo "[1/6] 检查Is_Prime函数..."
if grep -q "Is_Prime" "$MAIN_C"; then
    echo "   ✅ 通过: 找到Is_Prime函数"
    ((PASSED++))
else
    echo "   ❌ 失败: 未找到Is_Prime函数"
    ((FAILED++))
fi

# 测试用例2: 检查边界条件处理（n <= 1）
echo "[2/6] 检查边界条件处理..."
if grep -q "n <= 1" "$MAIN_C" || grep -q "n < 2" "$MAIN_C"; then
    echo "   ✅ 通过: 找到n <= 1的边界处理"
    ((PASSED++))
else
    echo "   ❌ 失败: 未找到边界条件处理"
    ((FAILED++))
fi

# 测试用例3: 检查2的特殊处理
echo "[3/6] 检查2的特殊处理..."
if grep -q "n == 2" "$MAIN_C"; then
    echo "   ✅ 通过: 找到n == 2的特殊处理"
    ((PASSED++))
else
    echo "   ⚠️  警告: 未找到n == 2的特殊处理，可能影响效率"
    ((PASSED++))
fi

# 测试用例4: 检查偶数快速判断
echo "[4/6] 检查偶数快速判断..."
if grep -q "n % 2 == 0" "$MAIN_C"; then
    echo "   ✅ 通过: 找到偶数快速判断"
    ((PASSED++))
else
    echo "   ❌ 失败: 未找到偶数快速判断"
    ((FAILED++))
fi

# 测试用例5: 检查奇数枚举逻辑
echo "[5/6] 检查奇数枚举逻辑..."
if grep -q "i \+\+= 2" "$MAIN_C" || grep -q "i = i + 2" "$MAIN_C"; then
    echo "   ✅ 通过: 找到奇数步长枚举"
    ((PASSED++))
else
    echo "   ⚠️  警告: 未找到奇数步长枚举，效率可能较低"
    ((PASSED++))
fi

# 测试用例6: 检查sqrt优化
echo "[6/6] 检查sqrt优化..."
if grep -q "i \* i <= n" "$MAIN_C"; then
    echo "   ✅ 通过: 找到i*i <= n优化（避免sqrt调用）"
    ((PASSED++))
elif grep -q "sqrt" "$MAIN_C"; then
    echo "   ⚠️  警告: 使用了sqrt函数，i*i <= n更高效"
    ((PASSED++))
else
    echo "   ❌ 失败: 未找到循环终止条件优化"
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
