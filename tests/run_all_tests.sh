#!/bin/bash

# 综合测试运行脚本
# 运行所有测试并生成汇总报告

echo "========================================="
echo "  桌面小摆件系统 - 自动化测试套件"
echo "========================================="
echo ""

# 检查是否在正确的目录
if [ ! -d "User" ] || [ ! -d "Hardware" ]; then
    echo "❌ 错误: 请在项目根目录运行此脚本"
    exit 1
fi

# 创建测试结果目录
mkdir -p test_results

# 测试脚本列表
TEST_SCRIPTS=(
    "test_menu_ui.sh"
    "test_boot_screen.sh"
    "test_led_control.sh"
    "test_buzzer_control.sh"
    "test_dht11.sh"
    "test_prime_function.sh"
)

# 统计变量
TOTAL_TESTS=0
TOTAL_PASSED=0
TOTAL_FAILED=0
ALL_PASSED=1

# 运行每个测试
for script in "${TEST_SCRIPTS[@]}"; do
    if [ -f "tests/$script" ]; then
        echo "运行: $script"
        bash "tests/$script" > "test_results/${script%.sh}.log" 2>&1
        RESULT=$?
        
        if [ $RESULT -eq 0 ]; then
            echo "✅ $script - 通过"
            ((TOTAL_PASSED++))
        else
            echo "❌ $script - 失败"
            ((TOTAL_FAILED++))
            ALL_PASSED=0
        fi
        ((TOTAL_TESTS++))
        echo ""
    else
        echo "⚠️  警告: 找不到 tests/$script"
    fi
done

# 生成汇总报告
echo "========================================="
echo "           测试汇总报告"
echo "========================================="
echo "总测试数: $TOTAL_TESTS"
echo "通过:     $TOTAL_PASSED"
echo "失败:     $TOTAL_FAILED"
echo "========================================="

if [ $ALL_PASSED -eq 1 ]; then
    echo ""
    echo "🎉 恭喜! 所有测试通过!"
    echo ""
    exit 0
else
    echo ""
    echo "❌ 部分测试失败，请查看 test_results/ 目录下的日志文件"
    echo ""
    
    # 显示失败的测试日志
    echo "失败的测试详情:"
    echo "========================================="
    for script in "${TEST_SCRIPTS[@]}"; do
        LOG_FILE="test_results/${script%.sh}.log"
        if [ -f "$LOG_FILE" ]; then
            # 检查日志是否包含失败信息
            if grep -q "❌" "$LOG_FILE" || grep -q "失败" "$LOG_FILE"; then
                echo ""
                echo "--- $script ---"
                tail -20 "$LOG_FILE"
            fi
        fi
    done
    echo ""
    
    exit 1
fi
