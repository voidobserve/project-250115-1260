#include "user_config.h"
#include "temperature_adjust.h"

extern bit ex_temp_en; // 过温降功率标志位
extern bit ex_temp_en; // 过温降功率标志位
extern struct
{
    u32 t;    // 计时器
    u8 level; // 时空列表执行的哪一个时间段
    u8 list;  // 时控列表，5个列表
    u8 en;    // 使能
} t_ctrl;
extern bit extemp_tctrl_flag;
extern u16 ntc_val; // 存放求平均之后，得到的ad值
// extern u8 ex_temp_state; // 标志位，温度扫描的半个小时是否到来
extern u16 max_duty; // 可调的最大占空比
// extern u16 ex_duty;      // 温度控制中，存放经过温度限制后，可调的、最大的pwm占空比
// extern bit extemp_flag;  // 标志位，是否检测到温度过热
extern bit adjust_en;
extern u32 pwm_in_duty_xuwei;
extern struct
{
    u8 en;
} _3_1;
extern u16 ajust_duty;
extern u32 ex_pwm_in_duty;
extern u16 ex_max_duty;
extern bit is_pwr_down;

// volatile u32 temperature_scan_cnt = 0;                        // 温度扫描时间计数，在定时器中累加
volatile u32 temperature_over_heat_cnt = 0;                   // 温度过热时间计数，在定时器中累加
volatile u8 cur_temperature_status = TEMPERATURE_STATUS_NONE; // 状态机，记录当前温度状态

// 过温降功率 定时判断 每秒执行一次
// 温度检测，进而调节pwm的功能，由定时器调用扫描
void temperature_scan_isr(void)
{
    static u16 __sec_cnt = 0; // 为了适配原有的框架，加入了该变量，进行半小时计时
    if (cur_temperature_status == TEMPERATURE_STATUS_NONE)
    {
        if (__sec_cnt < 65535) // 防止计数溢出
        {
            __sec_cnt++;
        }
    }
    else
    {
        // 如果温度以及过热，不使用该变量
        __sec_cnt = 0;
    }

    // 如果使能了温度检测功能 并且 时控功能 未使能
    if (ex_temp_en == 1 && t_ctrl.en == 0)
    {
        if (TEMPERATURE_STATUS_NONE == cur_temperature_status) // 如果温度未过热
        {
            // 如果检测到温度过大
            if (ntc_val < _3_1V)
            {
                // 更新可调的最大占空比
                max_duty = PWM_DUTY_50_PERCENT;
                cur_temperature_status = TEMPERATURE_STATUS_OVER_HEAT_0MIN;
            }
            else // 温度正常 不干预最大值 并且计数置0
            {
                if (__sec_cnt >= ((u16)60 * 30)) // 60 sec * 30 == 30 min
                {
                    // 如果之前检测到温度过热，并且半个小时的扫描周期到来
                    __sec_cnt = 0;

                    if (extemp_tctrl_flag == 0)
                    {
                        if (adjust_en == 1)
                        {
                            if (_3_1.en == 1)
                            {
                                max_duty = pwm_in_duty_xuwei;
                            }
                            else
                            {
                                max_duty = ajust_duty;
                            }
                        }

                        if (adjust_en == 0)
                        {
                            if (_3_1.en == 1)
                            {
                                max_duty = ex_pwm_in_duty;
                            }
                            else
                            {
                                // max_duty = pwm_save[0];
                            }
                        }

                        if (P00 == 0)
                        {
                            if (max_duty > ex_max_duty)
                            {
                                max_duty = ex_max_duty;
                            }
                        }
                    }

                    if (extemp_tctrl_flag == 1)
                    {
                        extemp_tctrl_flag = 0;
                        return;
                    }
                } // 如果之前检测到温度过热，并且半个小时的扫描周期到来
            }
        }
        else if (TEMPERATURE_STATUS_OVER_HEAT_0MIN == cur_temperature_status) // 检测到温度过热一次
        {
            // 原来的框架里有给 max_duty 赋值的语句，不知道会不会有冲突，这里一直给 max_duty 赋值
            max_duty = PWM_DUTY_50_PERCENT;

            // if (temperature_over_heat_cnt >= ((u32)300000)) // 5min
            if (temperature_over_heat_cnt >= ((u32)5 * 60 * 1000)) // 5min
            {
                // 过热5min后，跳转到对应的语句块进行处理
                cur_temperature_status = TEMPERATURE_STATUS_OVER_HEAT_5MIN;
            }
        }
        else if (TEMPERATURE_STATUS_OVER_HEAT_5MIN == cur_temperature_status) // 检测到温度过热累计5min
        {
            /*
                标志位，是否真的过热
                在该语句块内反复对比 检测到的ad值，如果仍过热，将pwm限制为 25%
            */
            static bit flag_is_really_over_heat = 0;

            if (ntc_val < _3_1V) // 如果仍过热
            {
                flag_is_really_over_heat = 1;
            }

            if (flag_is_really_over_heat)
            {
                max_duty = PWM_DUTY_25_PERCENT;
            }
        }

        if (extemp_tctrl_flag == 1)
        {
            extemp_tctrl_flag = 0;
        }
    }

    is_pwr_down = 1; // 使能缓慢升降功率，给标志位置一
}