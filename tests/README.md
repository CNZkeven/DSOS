# 自动化测试框架说明

本目录包含桌面小摆件系统的自动化测试脚本，用于验证项目的核心功能实现。

## 测试脚本列表

| 脚本名称 | 测试目标 | 主要检查内容 |
|---------|---------|-------------|
| `test_menu_ui.sh` | 主菜单UI | 菜单标题、菜单项、状态变量、menu()函数 |
| `test_boot_screen.sh` | 启动画面 | 启动文字、3秒延时、清屏操作 |
| `test_led_control.sh` | LED控制 | LED_Update非阻塞状态机、PWM控制 |
| `test_buzzer_control.sh` | 蜂鸣器控制 | TIM3中断、PWM配置、状态机实现 |
| `test_dht11.sh` | DHT11传感器 | 超时保护、数据校验、GPIO模式切换 |
| `test_prime_function.sh` | 质数判断 | Is_Prime函数、边界条件处理、算法优化 |
| `run_all_tests.sh` | 综合测试 | 运行所有测试并生成汇总报告 |

## 使用方法

### 运行单个测试

```bash
cd tests
bash test_menu_ui.sh
```

### 运行所有测试

```bash
cd tests
bash run_all_tests.sh
```

或在项目根目录直接运行：

```bash
bash tests/run_all_tests.sh
```

## 测试框架核心代码解析

### 1. 测试脚本通用结构

每个测试脚本都采用以下结构：

```bash
#!/bin/bash
echo "========================================="
echo "测试: xxx检查"
echo "========================================="

PASSED=0
FAILED=0

# 测试用例1
echo "[1/N] 检查xxx..."
if grep -q "pattern" "file.c"; then
    echo "   ✅ 通过"
    ((PASSED++))
else
    echo "   ❌ 失败"
    ((FAILED++))
fi

# 输出结果
echo "测试结果: $PASSED 通过, $FAILED 失败"
```

### 2. 综合测试脚本的核心代码 (`run_all_tests.sh`)

```bash
# 测试脚本列表
TEST_SCRIPTS=(
    "test_menu_ui.sh"
    "test_boot_screen.sh"
    "test_led_control.sh"
    "test_buzzer_control.sh"
    "test_dht11.sh"
    "test_prime_function.sh"
)

# 运行每个测试
for script in "${TEST_SCRIPTS[@]}"; do
    if [ -f "tests/$script" ]; then
        bash "tests/$script" > "test_results/${script%.sh}.log" 2>&1
        RESULT=$?
        if [ $RESULT -eq 0 ]; then
            ((TOTAL_PASSED++))
        else
            ((TOTAL_FAILED++))
        fi
    fi
done
```

### 3. 关键测试技术

**代码存在性检查：**
```bash
grep -q "function_name" "file.c"
```

**多条件组合检查：**
```bash
if grep -q "pattern1" "file.c" && grep -q "pattern2" "file.c"; then
```

**数组元素遍历检查：**
```bash
ITEMS=("item1" "item2" "item3")
ALL_FOUND=1
for item in "${ITEMS[@]}"; do
    if ! grep -q "\"$item\"" "file.c"; then
        ALL_FOUND=0
    fi
done
```

## 测试结果

测试通过后会在 `test_results/` 目录下生成各测试的详细日志文件。

## 扩展测试

如需添加新的测试模块，只需：
1. 在 `tests/` 目录下创建新的 `.sh` 脚本
2. 在 `run_all_tests.sh` 的 `TEST_SCRIPTS` 数组中添加脚本名称
3. 遵循现有脚本的结构和风格
