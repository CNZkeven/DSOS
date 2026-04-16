#!/bin/bash

# 测试脚本：主菜单UI检查
# 验证main.c中是否包含预期的菜单字符串和结构

echo "========================================="
echo "测试 1: 主菜单UI检查"
echo "========================================="

MAIN_C="User/main.c"
PASSED=0
FAILED=0

# 检查文件是否存在
if [ ! -f "$MAIN_C" ]; then
    echo "❌ 错误: 找不到 $MAIN_C"
    exit 1
fi

# 测试用例1: 检查主菜单标题
echo "[1/6] 检查主菜单标题..."
if grep -q "\[SYS\] MAIN HUB" "$MAIN_C"; then
    echo "   ✅ 通过: 找到主菜单标题"
    ((PASSED++))
else
    echo "   ❌ 失败: 未找到主菜单标题"
    ((FAILED++))
fi

# 测试用例2: 检查菜单选项
echo "[2/6] 检查菜单选项..."
MENU_ITEMS=("Hello!" "Buzzer" "Temp&Hum" "LED" "Count")
ALL_FOUND=1
for item in "${MENU_ITEMS[@]}"; do
    if ! grep -q "\"$item\"" "$MAIN_C"; then
        echo "   ❌ 失败: 未找到菜单项 '$item'"
        ALL_FOUND=0
    fi
done
if [ $ALL_FOUND -eq 1 ]; then
    echo "   ✅ 通过: 所有菜单项都存在"
    ((PASSED++))
else
    ((FAILED++))
fi

# 测试用例3: 检查a1和a2状态变量
echo "[3/6] 检查菜单状态变量..."
if grep -q "int a1 = 0" "$MAIN_C" && grep -q "int a2 = 0" "$MAIN_C"; then
    echo "   ✅ 通过: 找到a1和a2状态变量"
    ((PASSED++))
else
    echo "   ❌ 失败: 未找到a1或a2状态变量"
    ((FAILED++))
fi

# 测试用例4: 检查menu()函数
echo "[4/6] 检查menu()函数..."
if grep -q "void menu(void)" "$MAIN_C"; then
    echo "   ✅ 通过: 找到menu()函数"
    ((PASSED++))
else
    echo "   ❌ 失败: 未找到menu()函数"
    ((FAILED++))
fi

# 测试用例5: 检查按键提示
echo "[5/6] 检查按键提示..."
if grep -q "K1K2:NAV K3:OK" "$MAIN_C"; then
    echo "   ✅ 通过: 找到按键提示"
    ((PASSED++))
else
    echo "   ❌ 失败: 未找到按键提示"
    ((FAILED++))
fi

# 测试用例6: 检查子菜单页面
echo "[6/6] 检查子菜单页面..."
SUBMENUS=("a2 == 1" "a2 == 2" "a2 == 3" "a2 == 4" "a2 == 5")
ALL_SUBMENUS=1
for submenu in "${SUBMENUS[@]}"; do
    if ! grep -q "$submenu" "$MAIN_C"; then
        echo "   ❌ 失败: 未找到子菜单条件 $submenu"
        ALL_SUBMENUS=0
    fi
done
if [ $ALL_SUBMENUS -eq 1 ]; then
    echo "   ✅ 通过: 所有子菜单条件都存在"
    ((PASSED++))
else
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
