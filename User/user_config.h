#ifndef __USER_CONFIG_H
#define __USER_CONFIG_H

#include "include.h"

// 输出占空比100%时，对应的占空比参数
#define MAX_DUTY (6000)
#define PER_DUTY (10) //
#define _75_DUTY 4500 // 75%亮度
#define _70_DUTY 4200
#define _60_DUTY 3600
#define _50_DUTY 3000
#define _30_DUTY 1800

enum
{
    PWM_DUTY_100_PERCENT = MAX_DUTY,             // 100%占空比
    PWM_DUTY_80_PERCENT = (MAX_DUTY * 8 / 10),   // 80%
    PWM_DUTY_75_PERCENT = (MAX_DUTY * 75 / 100), // 75%占空比
    PWM_DUTY_60_PERCENT = (MAX_DUTY * 60 / 100), // 60%占空比
    PWM_DUTY_50_PERCENT = (MAX_DUTY * 50 / 100), // 50%占空比
    PWM_DUTY_30_PERCENT = (MAX_DUTY * 30 / 100), // 30%占空比
    PWM_DUTY_25_PERCENT = (MAX_DUTY * 25 / 100), // 25%占空比
};

#define _75_TEMP 2420                   // 75摄氏度对应的ADC值
#define _3_1V 3000                      // 3.4V对应的ADC值
#define L_PWR_T (1 * 60 * 60)           // 降功率
#define L_TEMP_PWR_T ((u16)1 * 30 * 60) // 过温半小时降功率
// #define LEVLE_PER 184              // 每个档位对应的AD值 （20级挡位调节）
// #define LEVLE_PER 92 // 每个档位对应的AD值 (40级挡位调节)
#define LEVLE_PER 23    // 每个档位对应的AD值 (160级挡位调节)
#define ADJUST_STEP 160 // 调节级数，例如 20级、40级、160级
#define DEAD_ZONE 5     // adc死区范围± 5

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

#endif
