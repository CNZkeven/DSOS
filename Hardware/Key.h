#ifndef __KEY_H
#define __KEY_H

void Key_Init(void);
/**
 * @brief 按键功能：调整主菜单选项（a1）
 */
//int Key_GetNum1(void);
///**
// * @brief 按键功能：控制子菜单切换（a2）
// */
//int Key_GetNum2(void);
uint8_t Key_GetNum(void);

#endif
