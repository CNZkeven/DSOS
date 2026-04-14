#ifndef __BUZZER_H
#define __BUZZER_H

// 音符频率定义（单位：Hz）
#define L_1  262
#define L_2  294
#define L_3  330
#define L_4  349
#define L_5  392
#define L_6  440
#define L_7  494
#define M_1  523
#define M_2  587
#define M_3  659
#define M_4  698
#define M_5  784
#define M_6  880
#define M_7  988
#define H_1  1046
#define NOTE_REST 0  // 休止符
//// 节拍时长定义（单位：ms）
//#define WHOLE_NOTE     2000
//#define HALF_NOTE      (WHOLE_NOTE/2)
//#define QUARTER_NOTE   (WHOLE_NOTE/4)
//#define EIGHTH_NOTE    (WHOLE_NOTE/8)
//#define SIXTEENTH_NOTE (WHOLE_NOTE/16)


void Buzzer_Init(void);
void Buzzer_ON(void);
void Buzzer_OFF(void);
void Buzzer_UpdatePlayback(void);
//void Buzzer_Turn(void);

#endif
