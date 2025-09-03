#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_err.h"

// --- 1. 定义硬件和音乐参数 ---
#define BUZZER_GPIO         GPIO_NUM_10 // 您可以修改这个引脚
#define LEDC_TIMER          LEDC_TIMER_0
#define LEDC_MODE           LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL        LEDC_CHANNEL_0
#define LEDC_DUTY_RES       LEDC_TIMER_10_BIT
#define LEDC_DUTY           (512) // 50% 占空比

// 定义音调的频率 (Hz)
#define NOTE_A4  440
#define NOTE_C5  523
#define NOTE_D5  587
#define NOTE_DS5 622  // D#5 / Eb5
#define NOTE_F5  698
#define NOTE_G5  784
#define NOTE_GS5 830  // G#5 / Ab5
#define NOTE_A5  880
#define NOTE_AS5 932  // A#5 / Bb5
#define NOTE_B5  988
#define NOTE_C6  1047
#define NOTE_CS6  1109 // C#6 / Db6
#define NOTE_SIL 0  // 静音 (Silence)

// 定义基础节拍时长 (毫秒) - 可以调整这个值来改变整体速度
#define BPM 120 // Beats Per Minute
#define QUARTER_NOTE_DURATION (60000 / BPM) // 四分音符时长
#define QUARTER_POINT_NOTE_DURATION ((QUARTER_NOTE_DURATION)*1.5) // 附点四分音符时长
#define HALF_NOTE_DURATION    (QUARTER_NOTE_DURATION * 2) // 二分音符
#define EIGHTH_NOTE_DURATION  (QUARTER_NOTE_DURATION / 2) // 八分音符

// 定义音符结构体
typedef struct {
    int freq;
    int duration;
} music_note_t;

// 新的乐谱: "Mystery Melody"
music_note_t mystery_melody[] = {
    {NOTE_GS5, QUARTER_NOTE_DURATION},
    {NOTE_F5, EIGHTH_NOTE_DURATION},
    {NOTE_G5, EIGHTH_NOTE_DURATION},
    {NOTE_GS5, QUARTER_NOTE_DURATION},
    {NOTE_SIL, EIGHTH_NOTE_DURATION},
    {NOTE_AS5, QUARTER_POINT_NOTE_DURATION},
    {NOTE_G5, EIGHTH_NOTE_DURATION},
    {NOTE_GS5, EIGHTH_NOTE_DURATION},
    {NOTE_AS5, QUARTER_NOTE_DURATION},
    {NOTE_SIL, QUARTER_NOTE_DURATION},

    {NOTE_GS5, EIGHTH_NOTE_DURATION},
    {NOTE_DS5, EIGHTH_NOTE_DURATION},
    {NOTE_GS5, EIGHTH_NOTE_DURATION},
    {NOTE_AS5, QUARTER_NOTE_DURATION},
    {NOTE_GS5, EIGHTH_NOTE_DURATION},
    {NOTE_G5, EIGHTH_NOTE_DURATION},
    {NOTE_GS5, QUARTER_NOTE_DURATION},
    {NOTE_AS5, EIGHTH_NOTE_DURATION},
    {NOTE_G5, QUARTER_NOTE_DURATION},
    {NOTE_GS5, QUARTER_NOTE_DURATION},
    {NOTE_SIL, EIGHTH_NOTE_DURATION},

    {NOTE_DS5, EIGHTH_NOTE_DURATION},
    {NOTE_F5, EIGHTH_NOTE_DURATION},
    {NOTE_G5, EIGHTH_NOTE_DURATION},
    {NOTE_GS5, EIGHTH_NOTE_DURATION},
    {NOTE_AS5, QUARTER_NOTE_DURATION},
    {NOTE_GS5, EIGHTH_NOTE_DURATION},
    {NOTE_G5, EIGHTH_NOTE_DURATION},
    {NOTE_GS5, HALF_NOTE_DURATION},
    {NOTE_SIL, QUARTER_NOTE_DURATION},
    
    {NOTE_GS5, EIGHTH_NOTE_DURATION},
    {NOTE_GS5, EIGHTH_NOTE_DURATION},
    {NOTE_CS6, EIGHTH_NOTE_DURATION},
    {NOTE_C6, EIGHTH_NOTE_DURATION},
    {NOTE_GS5, EIGHTH_NOTE_DURATION},
    {NOTE_GS5, QUARTER_NOTE_DURATION},
    {NOTE_DS5, EIGHTH_NOTE_DURATION},
    {NOTE_AS5, EIGHTH_NOTE_DURATION},
    {NOTE_GS5, HALF_NOTE_DURATION},
    {NOTE_SIL, QUARTER_NOTE_DURATION},
    
};


// --- 2. 初始化LEDC外设 ---
static void buzzer_init() {
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution  = LEDC_DUTY_RES,
        .freq_hz          = 5000,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = BUZZER_GPIO,
        .duty           = 0,
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}

// --- 3. 播放和停止函数 (与之前相同) ---
void play_note(int freq, int duration_ms) {
    if (freq == NOTE_SIL) {
        ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 0);
    } else {
        ledc_set_freq(LEDC_MODE, LEDC_TIMER, freq);
        ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, LEDC_DUTY);
    }
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
    vTaskDelay(pdMS_TO_TICKS(duration_ms));
}

void stop_playing() {
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 0);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
}

void Buzzer_main(void)
{
    printf("Buzzer example: Playing the mystery melody\n");
    buzzer_init();

    int num_notes = sizeof(mystery_melody) / sizeof(music_note_t);

    while(1)
    {
        for (int i = 0; i < num_notes; i++) {
            printf("Playing note: Freq=%d, Duration=%d\n", mystery_melody[i].freq, mystery_melody[i].duration);
            play_note(mystery_melody[i].freq, mystery_melody[i].duration);
            // 每个音符之间可以加一个非常短暂的停顿，让音符分离得更清晰
            // stop_playing();
                vTaskDelay(pdMS_TO_TICKS(20));
        }

        vTaskDelay(pdMS_TO_TICKS(1000)); // 每次播放完后暂停1秒
    }


    stop_playing();
    printf("Playback finished.\n");
}
