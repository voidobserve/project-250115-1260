#ifndef __ADC_H
#define __ADC_H

#include "include.h" // 芯片官方提供的头文件

// 定义adc的采集通道，对应片上外设的通道，不是引脚
enum
{
    ADC_CHANNEL_0 = 0, // adc通道0
    ADC_CHANNEL_1, // adc通道1 
};


extern u16 ntc_val; // 存放求平均之后，得到的ad值（从热敏电阻一端采集的ad值）
extern u16 adjust_val; // 电位器的ADC值 0~3686---->60%~100%---->3600~6000

extern u16 adc0_val;
extern u16 adc1_val;

#endif