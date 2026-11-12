#ifndef USER_MAIN_H
#define USER_MAIN_H

typedef enum {
    LED_STATE_IDLE = 0, // 空闲状态
    LED_STATE_POWER_ON, // 上电缓启动状态，暂不响应调光
    LED_STATE_INCREASE, // 增加亮度
    LED_STATE_DECREASE  // 降低亮度
} LedState;

extern LedState led_state; // LED 当前控制状态

#endif
