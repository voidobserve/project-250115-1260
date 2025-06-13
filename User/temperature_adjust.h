#ifndef __TEMPERATURE_ADJUST_H
#define __TEMPERATURE_ADJUST_H

#include "include.h" // 芯片官方提供的头文件

// 定义温度的状态，用于状态机相关的判断中
enum
{
    TEMPERATURE_STATUS_NONE = 0,  // 温度正常
    TEMPERATURE_STATUS_OVER_HEAT_0MIN, // 检测到了一次温度过热
    /*
        检测到过热后，仍在过热且累计5min（实际是不是累计了5min，由相关判断程序决定）
        最简单的方法是过热5min后，再检测一轮，如果仍过热，则认为累计有5min过热
    */
    TEMPERATURE_STATUS_OVER_HEAT_5MIN,
};

extern volatile u32 temperature_over_heat_cnt; // 温度过热时间计数，在定时器中累加
extern volatile u8 cur_temperature_status;    // 状态机，记录当前温度状态



void temperature_scan_isr(void);















#endif