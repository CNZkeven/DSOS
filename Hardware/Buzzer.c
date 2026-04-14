#include "stm32f10x.h"
#include "misc.h"
#include "Buzzer.h"

#define BUZZER_REST_ARR      999U

static volatile uint32_t buzzer_note_cycles = 0;
static volatile uint32_t buzzer_note_target_cycles = 0;
static volatile uint8_t buzzer_playing = 0;
static uint16_t buzzer_last_freq = 0;

// C4 状态机变量
static uint32_t c4_elapsed_ms = 0;
static uint8_t  c4_is_beep = 0;
static uint16_t current_duration_ms = 0;
static uint32_t lcg_seed = 12345;

static void Buzzer_LoadFreqDuration(uint16_t freq, uint16_t duration_ms)
{
    uint16_t arr;

    current_duration_ms = duration_ms;
    buzzer_note_cycles = 0;

    if (freq == NOTE_REST)
    {
        TIM_SetAutoreload(TIM3, BUZZER_REST_ARR);
        TIM_SetCompare1(TIM3, 0);
        if (buzzer_last_freq != NOTE_REST) {
            TIM_SetCounter(TIM3, 0);
        }
        buzzer_note_target_cycles = duration_ms; // REST时定时器工作在1kHz
        buzzer_last_freq = NOTE_REST;
        return;
    }

    arr = (1000000U / freq) - 1U;
    TIM_SetAutoreload(TIM3, arr);
    
    // 50% 占空比发声
    TIM_SetCompare1(TIM3, arr / 2U);
    if (freq != buzzer_last_freq) {
        TIM_SetCounter(TIM3, 0);
    }

    // 根据频率计算需要多少个PWM周期来实现duration_ms的时长
    buzzer_note_target_cycles = (uint32_t)duration_ms * freq / 1000U;
    if (buzzer_note_target_cycles == 0)
    {
        buzzer_note_target_cycles = 1U;
    }
    
    buzzer_last_freq = freq;
}

static void C4_GenerateNext(void)
{
    c4_elapsed_ms += current_duration_ms;

    if (c4_elapsed_ms < 19000)
    {
        if (c4_is_beep == 0) // 刚刚结束了静音，现在应该滴一下
        {
            c4_is_beep = 1;
            // C4固定高音调：4000Hz，短促有力 (80ms)
            Buzzer_LoadFreqDuration(8000, 80);
        }
        else // 刚刚结束了滴声，现在应该静音
        {
            c4_is_beep = 0;
            uint32_t remaining = 19000 - c4_elapsed_ms;
            
            // 静音时间随着剩余时间减少而变短 (渐变加速)
            uint16_t rest_duration = (remaining * 960 / 19000) + 40;
            
            Buzzer_LoadFreqDuration(NOTE_REST, rest_duration);
        }
    }
    else if (c4_elapsed_ms < 20000)
    {
        // 19秒到20秒：保持相同固定音高(4000Hz)的死亡长鸣！
        uint32_t long_beep_duration = 20000 - c4_elapsed_ms;
        if (long_beep_duration == 0) long_beep_duration = 1;
        Buzzer_LoadFreqDuration(8000,long_beep_duration);
    }
    else if (c4_elapsed_ms < 23000)
    {
        // 爆炸阶段 (持续3秒)
        // 模拟爆炸声：使用伪随机数快速切换低频，制造嘈杂轰隆感 (白噪音/粉红噪音近似)
        lcg_seed = lcg_seed * 1664525 + 1013904223;
        
        uint32_t exp_time = c4_elapsed_ms - 20000;
        
        // 爆炸越往后，最高频率越低，模拟余音沉缓
        uint16_t max_freq = 400 - (exp_time * 350 / 3000); // 从400 Hz 降到 50 Hz
        uint16_t min_freq = 30;
        if (max_freq < min_freq + 10) max_freq = min_freq + 10;
        
        uint16_t freq = min_freq + (lcg_seed % (max_freq - min_freq));
        
        // 每个频率片段5到14毫秒，产生颗粒极粗的杂音
        uint16_t noise_slice_ms = 5 + (lcg_seed % 10);
        Buzzer_LoadFreqDuration(freq, noise_slice_ms);
    }
    else
    {
        // 结束
        Buzzer_OFF();
    }
}

void Buzzer_Init(void)
{
    GPIO_InitTypeDef gpio_init_structure;
    TIM_TimeBaseInitTypeDef tim_time_base_init_structure;
    TIM_OCInitTypeDef tim_oc_init_structure;
    NVIC_InitTypeDef nvic_init_structure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    gpio_init_structure.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio_init_structure.GPIO_Pin = GPIO_Pin_6;
    gpio_init_structure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio_init_structure);

    TIM_InternalClockConfig(TIM3);

    tim_time_base_init_structure.TIM_ClockDivision = TIM_CKD_DIV1;
    tim_time_base_init_structure.TIM_CounterMode = TIM_CounterMode_Up;
    tim_time_base_init_structure.TIM_Period = BUZZER_REST_ARR;
    tim_time_base_init_structure.TIM_Prescaler = 72 - 1; // 时钟分频到 1MHz
    tim_time_base_init_structure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM3, &tim_time_base_init_structure);

    TIM_OCStructInit(&tim_oc_init_structure);
    tim_oc_init_structure.TIM_OCMode = TIM_OCMode_PWM1;
    tim_oc_init_structure.TIM_OCPolarity = TIM_OCPolarity_High;
    tim_oc_init_structure.TIM_OutputState = TIM_OutputState_Enable;
    tim_oc_init_structure.TIM_Pulse = 0;
    TIM_OC1Init(TIM3, &tim_oc_init_structure);

    TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);

    nvic_init_structure.NVIC_IRQChannel = TIM3_IRQn;
    nvic_init_structure.NVIC_IRQChannelCmd = ENABLE;
    nvic_init_structure.NVIC_IRQChannelPreemptionPriority = 1;
    nvic_init_structure.NVIC_IRQChannelSubPriority = 1;
    NVIC_Init(&nvic_init_structure);

    TIM_Cmd(TIM3, DISABLE);
}

void Buzzer_UpdatePlayback(void)
{
    if (!buzzer_playing)
    {
        return;
    }

    buzzer_note_cycles++;
    if (buzzer_note_cycles < buzzer_note_target_cycles)
    {
        return;
    }

    C4_GenerateNext();
}

void Buzzer_OFF(void)
{
    buzzer_playing = 0;
    buzzer_note_cycles = 0;
    buzzer_note_target_cycles = 0;
    buzzer_last_freq = 0;

    TIM_SetCompare1(TIM3, 0);
    TIM_SetCounter(TIM3, 0);
    TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
    TIM_Cmd(TIM3, DISABLE);
}

void Buzzer_ON(void)
{
    buzzer_playing = 1;
    
    // 初始化C4状态
    c4_elapsed_ms = 0;
    c4_is_beep = 0; // 第一次调用C4_GenerateNext会立马切换为beep发出响声
    current_duration_ms = 0;
    
    C4_GenerateNext();
    TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
    TIM_Cmd(TIM3, ENABLE);
}
