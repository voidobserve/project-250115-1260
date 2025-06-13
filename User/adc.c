#include "include.h"
#include "main.h"

#include "user_config.h"
#include "adc.h"
#include "temperature_adjust.h"

u16 ntc_val;        // 存放求平均之后，得到的ad值
u16 adjust_val;     // 存放求平均之后，得到的ad值
u16 ntc_val_all;    // 存放采集的ad值总和，最后要对这个变量求平均
u16 adjust_val_all; // 存放采集的ad值总和，最后要对这个变量求平均
u8 adc_cnt = 0;
u8 loop_cnt = 0;
u16 adc0_val;
u16 adc1_val;
//    P1_MD0 &=  ~(GPIO_P13_MODE_SEL(0x3));
//    P1_PU |=  (GPIO_P13_PULL_UP(0x1));
//    P1_PD |=    (GPIO_P13_PULL_PD(1));
void adc_init()
{
    // 配置P27 p30为模拟输入模式 并且上拉
    P2_MD0 |= GPIO_P27_MODE_SEL(0x3); // P27设为模拟模式  PIN9
    P3_MD0 |= GPIO_P30_MODE_SEL(0x3); // P30设为模拟模式  PIN8
    P2_PU |= (GPIO_P27_PULL_UP(0x1)); // P27使能上拉      PIN9  用于初始化判断是否接入ntc
    P2_PD |= (GPIO_P27_PULL_PD(0x0)); // P27关闭下拉（上电默认就是0，这里或上0，操作无效）
    P3_PU |= (GPIO_P30_PULL_UP(0x1)); // P30使能上拉      PIN8  用于初始化判断是否接入电位器
    P3_PD |= (GPIO_P30_PULL_PD(0x0)); // P30关闭下拉
    // ADC配置
    ADC_ACON1 &= ~(ADC_VREF_SEL(0x7) | ADC_EXREF_SEL(0x1)); // 关闭外部参考电压
    ADC_ACON1 |= ADC_VREF_SEL(0x6) |                        // 选择内部参考电压VCCA  5v
                 ADC_TEN_SEL(0x3);
    ADC_ACON0 = ADC_CMP_EN(0x1) |  // 打开ADC中的CMP使能信号
                ADC_BIAS_EN(0x1) | // 打开ADC偏置电流能使信号
                ADC_BIAS_SEL(0x1);

    // P27 检测温度的引脚，使用adc通道0
    ADC_CHS0 = ADC_ANALOG_CHAN(0x17) | // P27通路
               ADC_EXT_SEL(0x0);       // 选择外部通路
    ADC_CFG0 |= ADC_CHAN0_EN(0x1) |    // 使能通道0转换
                ADC_EN(0x1);           // 使能A/D转换

    // P30 使用通道1
    ADC_CHS1 = ADC_ANALOG_CHAN(0x18) | // P30通路
               ADC_EXT_SEL(0x0);       // 选择外部通路
    ADC_CFG0 |= ADC_CHAN1_EN(0x1) |    // 使能通道1转换
                ADC_EN(0x1);           // 使能A/D转换

    delay_ms(1); // 等待ADC模块配置稳定，需要等待20us以上
}

/**
 * @brief 获取一次adc采集+滤波后的值
 *
 * @param adc_channel_x  ADC_CHANNEL_0  或者 是 ADC_CHANNEL_1
 * @return u16
 */
u16 adc_get_val(u8 adc_channel_x)
{
    u8 i = 0; // adc采集次数的计数
    volatile u16 g_temp_value = 0;
    volatile u32 g_tmpbuff = 0;
    volatile u16 g_adcmax = 0;
    volatile u16 g_adcmin = 0xFFFF;

    // 采集20次，去掉前两次采样，再去掉一个最大值和一个最小值，再取平均值
    for (i = 0; i < 20; i++)
    {
        if (ADC_CHANNEL_0 == adc_channel_x)
        {
            ADC_CFG0 |= ADC_CHAN0_TRG(0x1); // 触发ADC0转换
            while (!(ADC_STA & ADC_CHAN0_DONE(0x1)))
                ;                                                 // 等待转换完成
            g_temp_value = (ADC_DATAH0 << 4) | (ADC_DATAL0 >> 4); // 读取 channel0 的值
            ADC_STA = ADC_CHAN0_DONE(0x1);                        // 清除ADC0转换完成标志位
        }
        else // ADC_CHANNEL_1 ==  adc_channel_x
        {
            ADC_CFG0 |= ADC_CHAN1_TRG(0x1); // 触发ADC1转换
            while (!(ADC_STA & ADC_CHAN1_DONE(0x1)))
                ;                                                 // 等待转换完成
            g_temp_value = (ADC_DATAH1 << 4) | (ADC_DATAL1 >> 4); // 读取 channel1 的值
            ADC_STA = ADC_CHAN1_DONE(0x1);                        // 清除ADC1转换完成标志位
        }

        if (i < 2)
            continue; // 丢弃前两次采样的
        if (g_temp_value > g_adcmax)
            g_adcmax = g_temp_value; // 最大
        if (g_temp_value < g_adcmin)
            g_adcmin = g_temp_value; // 最小

        g_tmpbuff += g_temp_value;
    }

    g_tmpbuff -= g_adcmax;           // 去掉一个最大
    g_tmpbuff -= g_adcmin;           // 去掉一个最小
    g_temp_value = (g_tmpbuff >> 4); // 除以16，取平均值

    return g_temp_value;
}

void adc_val()
{
    // ADC_CFG0 |= ADC_CHAN0_TRG(0x1); // 触发ADC0转换
    // while (!(ADC_STA & ADC_CHAN0_DONE(0x1)))
    //     ;                                             // 等待转换完成
    // ADC_STA = ADC_CHAN0_DONE(0x1);                    // 清除ADC0转换完成标志位
    // adc0_val = (ADC_DATAH0 << 4) | (ADC_DATAL0 >> 4); // 读取channel0的值

    ADC_CFG0 |= ADC_CHAN1_TRG(0x1); // 触发ADC1转换
    while (!(ADC_STA & ADC_CHAN1_DONE(0x1)))
        ;                                             // 等待转换完成
    ADC_STA = ADC_CHAN1_DONE(0x1);                    // 清除ADC1转换完成标志位
    adc1_val = (ADC_DATAH1 << 4) | (ADC_DATAL1 >> 4); // 读取channel1的值

    // ntc_val_all += adc0_val;
    adjust_val_all += adc1_val;

    adc_cnt++;
    if (adc_cnt == 10) // 均值滤波
    {
        // ntc_val = ntc_val_all / 10;
        adjust_val = adjust_val_all / 10;
        adc_cnt = 0;
        // ntc_val_all = 0;
        adjust_val_all = 0;
    }

    { // 滑动平均滤波

        static bit flag_is_buff_initialized = 0; // 是否第一次进入(用于给滑动平均滤波的数组初始化)
        static u16 buff[20] = {0};               // 滑动平均滤波使用到的数组
        static u8 buff_index = 0;                // 滑动平均滤波使用到的数组下标
        u16 tmp_adc_val = adc_get_val(ADC_CHANNEL_0);

        if (0 == flag_is_buff_initialized) // 如果滑动平均滤波的数组未初始化
        {
            u8 i = 0;
            for (; i < ARRAY_SIZE(buff); i++)
            {
                buff[i] = tmp_adc_val; // 用第一次采集到的ad值 给滑动平均滤波的数组初始化
            }

            flag_is_buff_initialized = 1;
        }

        buff[buff_index] = tmp_adc_val;
        buff_index++;
        if (buff_index >= ARRAY_SIZE(buff))
        {
            buff_index = 0;
        }

        {
            u8 i = 0;
            u32 sum = 0;
            for (; i < ARRAY_SIZE(buff); i++)
            {
                sum += buff[i]; //
            }

            // sum /= (ARRAY_SIZE(buff));
            ntc_val = sum / (ARRAY_SIZE(buff)); // 得到最终滤波后的ad值
        }

    } // 滑动平均滤波
}
