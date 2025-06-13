#ifndef __ADC_H
#define __ADC_H

#include "include.h" // 芯片官方提供的头文件

// 定义adc的采集通道，对应片上外设的通道，不是引脚
enum
{
    ADC_CHANNEL_0 = 0,
    ADC_CHANNEL_1,
};


#endif