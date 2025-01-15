#include "include.h"
#include "main.h"
#include <math.h>
#include <stdio.h>
extern void pwm_in_handler(void);
extern u16 ntc_val;
extern u16 adc0_val;
extern u16 adc1_val;

// 输出占空比100%时，对应的占空比参数
#define MAX_DUTY (6000)
#define PER_DUTY (10) //
#define _75_DUTY 4500 // 75%亮度
#define _70_DUTY 4200
#define _60_DUTY 3600
#define _50_DUTY 3000
#define _30_DUTY 1800

#define _75_TEMP 2420              // 75摄氏度对应的ADC值
#define _3_1V 3000                 // 3.4V对应的ADC值
#define L_PWR_T (1 * 60 * 60)      // 降功率
#define L_TEMP_PWR_T (1 * 30 * 60) // 过温半小时降功率
// #define LEVLE_PER 184              // 每个档位对应的AD值 （20级挡位调节）
#define LEVLE_PER 92 // 每个档位对应的AD值 (40级挡位调节)
// #define LEVLE_PER 23 // 每个档位对应的AD值 (160级挡位调节)
#define ADJUST_STEP 40 // 调节级数，例如 20级、40级、120级
#define DEAD_ZONE 5    // adc死区范围± 5

bit ex_temp_en; // 过温降功率标志位
// #define L_PWR_T     (1*10)

/* LIN 改的地方有：
                 增加adc.c文件
                 加入了电位器值的判断                void adjust_pwm()
                 加入了过温降功率函数                ex_temp_adjust_timer()
                                 LIN 改 加入了电位器 将时控数值改动  void t_ctrl_timer_handler(void)
                                 加入了ntc和电位器的调节标志位判断   option_fun(void)
                                 一小时降功率中加入了电位器值转换    l_pwr_timer_handler(void)
                                 缓启动无三合一时将最大亮度改为电位器的值   void led_pwr_on(void)
*/
typedef struct
{
    u32 t;    // 计时器
    u8 level; // 时空列表执行的哪一个时间段
    u8 list;  // 时控列表，5个列表
    u8 en;    // 使能
} t_ctrl_t;   // 时控结构体

typedef struct
{
    u8 en;
    u8 act; // 动作，执行降功率动作：1
    u32 t;  // 秒

} l_pwr_t; // 降功率结构体

typedef struct
{
    u8 en;
} _3_1_t; // 三合一结构体

u8 cap_10ms_cnt; // 10ms获取一次捕获占空比
u8 pwr_on_cnt;
u16 c_duty = 0;      // 当前设置的占空比
u32 pwm_in_duty = 0; // 三合一PWM输入的占空比
u32 pwm_in_duty_xuwei = 0;
u32 ex_pwm_in_duty = 0;
u16 ex_max_duty = 0;
u16 max_duty = 6000; // 可调的最大占空比
t_ctrl_t t_ctrl;
l_pwr_t l_pwr;
_3_1_t _3_1;
bit dly_pwr_on = 0;
u8 dly_pwr_on_cnt = 0;
bit is_pwr_down = 0; // 1：正在降功率，调用缓升缓降逻辑
bit adjust_en;
// 时控模式0：100%6H 50%6H 0%
// 时控模式1：100%4H 50%8H 0%
// 时控模式2：100%5H 70%3H 50%4H 0%
// 时控模式3：100%4H 70%4H 50%4H 0%
// 时控模式4：100%4H 60%2H 30%3H 60%2H 100%1H 0%
void flag_init();
typedef struct
{
    u16 b; // 当前亮度
    u8 t;  // 当前亮度保持时间,255时控结束
} t_ctrl_list_t;

t_ctrl_list_t t_ctrl_lis_0[3] =
    {
        {MAX_DUTY, 6}, {_50_DUTY, 6}, {0, 255} // 6 6
};

t_ctrl_list_t code t_ctrl_lis_1[3] =
    {
        {MAX_DUTY, 4}, {_50_DUTY, 8}, {0, 255}};

t_ctrl_list_t code t_ctrl_lis_2[4] =
    {
        {MAX_DUTY, 5}, {_70_DUTY, 3}, {_50_DUTY, 4}, {0, 255} // 534
};

t_ctrl_list_t code t_ctrl_lis_3[4] =
    {
        {MAX_DUTY, 4}, {_70_DUTY, 4}, {_50_DUTY, 4}, {0, 255} // 444
};

t_ctrl_list_t code t_ctrl_lis_4[6] =
    {
        {MAX_DUTY, 4}, {_60_DUTY, 2}, {_30_DUTY, 3}, {_60_DUTY, 2}, {MAX_DUTY, 1}, {0, 255} // 42321
};

t_ctrl_list_t *p_list;

extern u16 cap0_val;
extern u16 cap1_val;

extern void led_pwr_on(void);
extern void cmp_duty(void);
extern void led_plus(void);
extern void led_sub(void);
extern void cal_pwm_in_duty(void);

extern u16 adjust_val; // 电位器的ADC值 0~3686---->60%~100%---->3600~6000
u16 ajust_duty;
// 20档电位器调节pwm   60%-62%-64%-66%- 68%-70%-72%-74%-76%6-7896-80%--82. 5%-85%-87.5%-90%-92%-94%-96%-98%-100%
// 修改为40档电位器调节 pwm
void adjust_pwm()
{
    static u16 last_level = 0xFF;           // 记录之前的挡位
    u16 cur_level = adjust_val / LEVLE_PER; // 计算当前挡位

    if (adjust_en == 1)
    {
#if 0 // 20级挡位调节：
        if (adjust_val > 3680) // 4.5V对应的AD值
        {
            ajust_duty = 6000;
        }
        if (0 < adjust_val && adjust_val < LEVLE_PER)
        {
            ajust_duty = 3600;
        }
        else if (LEVLE_PER < adjust_val && adjust_val < LEVLE_PER * 2)
        {
            ajust_duty = 3720;
        }
        else if (2 * LEVLE_PER < adjust_val && adjust_val < LEVLE_PER * 3)
        {
            ajust_duty = 3840;
        }
        else if (3 * LEVLE_PER < adjust_val && adjust_val < LEVLE_PER * 4)
        {
            ajust_duty = 3960;
        }
        else if (4 * LEVLE_PER < adjust_val && adjust_val < LEVLE_PER * 5)
        {
            ajust_duty = 4080;
        }
        else if (5 * LEVLE_PER < adjust_val && adjust_val < LEVLE_PER * 6)
        {
            ajust_duty = 4200;
        }
        else if (6 * LEVLE_PER < adjust_val && adjust_val < LEVLE_PER * 7)
        {
            ajust_duty = 4320;
        }
        else if (7 * LEVLE_PER < adjust_val && adjust_val < LEVLE_PER * 8)
        {
            ajust_duty = 4440;
        }
        else if (8 * LEVLE_PER < adjust_val && adjust_val < LEVLE_PER * 9)
        {
            ajust_duty = 4560;
        }
        else if (9 * LEVLE_PER < adjust_val && adjust_val < LEVLE_PER * 10)
        {
            ajust_duty = 4680;
        }
        else if (10 * LEVLE_PER < adjust_val && adjust_val < LEVLE_PER * 11)
        {
            ajust_duty = 4800;
        }
        else if (11 * LEVLE_PER < adjust_val && adjust_val < LEVLE_PER * 12)
        {
            ajust_duty = 4950;
        }
        else if (12 * LEVLE_PER < adjust_val && adjust_val < LEVLE_PER * 13)
        {
            ajust_duty = 5100;
        }
        else if (13 * LEVLE_PER < adjust_val && adjust_val < LEVLE_PER * 14)
        {
            ajust_duty = 5250;
        }
        else if (14 * LEVLE_PER < adjust_val && adjust_val < LEVLE_PER * 15)
        {
            ajust_duty = 5400;
        }
        else if (15 * LEVLE_PER < adjust_val && adjust_val < LEVLE_PER * 16)
        {
            ajust_duty = 5520;
        }
        else if (16 * LEVLE_PER < adjust_val && adjust_val < LEVLE_PER * 17)
        {
            ajust_duty = 5640;
        }
        else if (17 * LEVLE_PER < adjust_val && adjust_val < LEVLE_PER * 18)
        {
            ajust_duty = 5760;
        }
        else if (18 * LEVLE_PER < adjust_val && adjust_val < LEVLE_PER * 19)
        {
            ajust_duty = 5880;
        }
        else if (19 * LEVLE_PER < adjust_val && adjust_val < LEVLE_PER * 20)
        {
            ajust_duty = 6000;
        }
        // ajust_duty=5760;
#endif

        if (adjust_val > 3680) // 超出了4.5V对应的AD值
        {
            ajust_duty = 6000;
            last_level = cur_level;
            return;
        }

        if (0 == cur_level) // 最小挡位
        {
            ajust_duty = 3600;
            last_level = cur_level;
            return;
        }

        if (cur_level >= ADJUST_STEP) // 最大挡位
        {
            ajust_duty = 6000;
            last_level = cur_level;
            return;
        }

        if (last_level == cur_level)
        {
            // 之前的挡位和当前计算得出的挡位一样，不调节
            return;
        }

        ajust_duty = 3600 + cur_level * ((6000 - 3600) / ADJUST_STEP);
        last_level = cur_level;
    }
}

//
u16 ex_temp_t = 0;
u8 ex_temp_cnt = 0; // 过温降功率处理次数，第一次降到90% 第二次80%-----
u8 ex_temp_state = 1;

#if 0 // 歧义 过温降功率是否降为当前值的90%

void ex_temp_adjust_timer()//过温降功率定时处理
{
	if(ex_temp_en==1)//判断过温降功率标志
	{
		if(adjust_en==0)//判断电位器
			{
				if(ntc_val<_75_TEMP&&ex_temp_state==1)//半小时处理一次 AD值越小 温度越高
				{
					if(pwm_in_duty>(6000*ex_temp_cnt/10))
					{
						max_duty=6000*ex_temp_cnt/10;//第一次降到90%，之后每次将10% 10%对应600
						ex_temp_state=0;//处理完成 半小时后再开启
					}
					else
					{
						ex_temp_cnt--;
						ex_temp_state=1;
					}
				}
				ex_temp_t++;
				if(ex_temp_t==L_TEMP_PWR_T)
				{
					ex_temp_t=0;//半小时计数清零
					ex_temp_state=1;
					ex_temp_cnt--;//每半小时计数一次
				}
				if(ntc_val>_75_TEMP)//判断温度是否下降到75以下
				{
					ex_temp_cnt=9;
				}
			}
			if(adjust_en==1)//判断电位器
			{
				if(ntc_val<_75_TEMP&&ex_temp_state==1)//半小时处理一次 AD值越小 温度越高
				{
					if(pwm_in_duty>(ajust_duty*ex_temp_cnt/10))
					{
						max_duty=ajust_duty*ex_temp_cnt/10;//第一次降到90%，之后80% 70% 
						ex_temp_state=0;//处理完成 半小时后再开启
					}
					else
					{
						ex_temp_cnt--;
						ex_temp_state=1;
					}
				}
				ex_temp_t++;
				if(ex_temp_t==L_TEMP_PWR_T)
				{
					ex_temp_t=0;//半小时计数清零
					ex_temp_state=1;
					ex_temp_cnt--;//每半小时计数一次
				}
				if(ntc_val>_75_TEMP)//判断温度是否下降到75以下
				{
					ex_temp_cnt=9;
				}
			}
	}
}
#endif
u16 pwm_save[10] = 0;
u16 ex_duty = 0;
bit extemp_flag = 0;
bit extemp_tctrl_flag = 0;
void ex_temp_adjust_timer() // 过温降功率 定时判断 每秒执行一次
{
    // printf("ntc_val=%d  ex_temp_cnt=%d   ex_temp_t=%d  \r\n",(int)pwm_in_duty,(int)c_duty,(int)ex_temp_t);
    if (ex_temp_en == 1 && t_ctrl.en == 0)
    {
        // ntc_val=adjust_val;
        if (extemp_tctrl_flag == 1)
        {
            pwm_save[0] = max_duty;
            ex_temp_cnt = 0;
            extemp_tctrl_flag = 0;
        }
        if (ntc_val < _3_1V && ex_temp_state == 1) // 半小时处理一次 AD值越小 温度越高
        {
            pwm_save[ex_temp_cnt] = pwm_in_duty; // 用数组存住第一个值

            ex_duty = pwm_save[0] - (pwm_save[0] / 10 * (ex_temp_cnt + 1)); // 第一次降到90%，之后每次将10%
            ex_temp_state = 0;                                              // 处理完成 半小时后再开启
            ex_temp_t = 0;

            max_duty = ex_duty;
            ex_temp_cnt++;
            if (ex_temp_cnt > 9)
            {
                max_duty = 0;
            }
            extemp_flag = 1;
        }
        if (ntc_val > _3_1V) // 温度降下来后 不干预最大值 并且计数置0 下次过温再从90%开始
        {
            ex_temp_cnt = 0;
            if (extemp_flag == 1 && ex_temp_state == 1)
            {
                if (extemp_tctrl_flag == 0)
                {
                    if (adjust_en == 1)
                    {
                        if (_3_1.en == 1)
                            max_duty = pwm_in_duty_xuwei;
                        else
                            max_duty = ajust_duty;
                    }
                    if (adjust_en == 0)
                    {
                        if (_3_1.en == 1)
                            max_duty = ex_pwm_in_duty;
                        else
                            max_duty = pwm_save[0];
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
                    extemp_flag = 0;
                    ex_temp_cnt = 0;
                    return;
                }
            }
        }
        ex_temp_t++;
        if (ex_temp_t == L_TEMP_PWR_T)
        {
            ex_temp_t = 0; // 半小时计数清零
            ex_temp_state = 1;
            // extemp_flag=0;
        }

        //    if(extemp_tctrl_flag==1)
        //			{
        //				pwm_save[0]=max_duty;
        //			}
    }
    is_pwr_down = 1;
    // printf("ex_temp_state=%d\r\n",(int)ex_temp_state);
    // printf("pwm_in_duty= %d  t= %d  ajust_duty=%d  c_duty=%d adjust_val=%d pwm_save[0]=%d\r\n", (int)pwm_in_duty,(int)t_ctrl.t,(int)ajust_duty,(int)c_duty,(int)adjust_val,(int)pwm_save[0]);
}

void set_pwm_duty(void)
{
    u16 duty_tmp;
    //    if(c_duty != 0)
    {

        duty_tmp = MAX_DUTY - c_duty;
        // c_duty = 10;
        STMR0_CMPAH = STMR_CMPA_VAL_H(((c_duty) >> 8) & 0xFF); // 比较值
        STMR0_CMPAL = STMR_CMPA_VAL_L(((c_duty) >> 0) & 0xFF); // 比较值
        STMR_LOADEN |= STMR_0_LOAD_EN(0x1);                    // 自动装载使能
    }
    //    else
    {
    }

    // TMR2_PWMH  = TMR_PWM_VAL_H((duty_tmp >> 8) & 0xFF);                      // 占空比设置值
    // TMR2_PWML  = TMR_PWM_VAL_L((duty_tmp >> 0) & 0xFF);
}

u8 adjust_ms = 0;
void cap_timer(void)
{
    if (cap_10ms_cnt < 10)
    {
        cap_10ms_cnt++;
    }
    pwr_on_cnt++;
    adjust_ms++;
}

// 放在while,没10ms获取一次PWM占空比
// 连续获取5次，占空比稳定才输出
u16 duty_buff[5];
u8 duty_cnt = 0;
// 上电采样5次完成标志为
u8 simple_over = 0;
// 放主循环
void get_duty(void)
{
    // 10ms获取一次
    if (cap_10ms_cnt >= 10)
    {
        cap_10ms_cnt = 0;
        if (_3_1.en == 1) // 三合一功能开启才调光
        {
            // 非三合一不会更新pwm_in_duty
            cal_pwm_in_duty();
        }
        // 处理降功率，时控激活，限制可调的占空比范围
        // 延时开机模式，强制pwm_in_duty=0

        pwm_in_handler();
        cmp_duty();
    }

    if (adjust_ms == 3)
    {
        adjust_ms = 0;
        led_plus();
        led_sub();
    }

    led_pwr_on();
}

// 计算三合一的占空比,储存pwm_in_duty
void cal_pwm_in_duty(void)
{
    u8 i;
    if (cap1_val == 0 && cap0_val == 0)
    {
        if (P14 == 0)
        {
            // 占空比 0
            pwm_in_duty = 0;
        }
        if (P14 == 1)
        {
            pwm_in_duty = 6000;
        }
    }
    else
    {
        pwm_in_duty = (u32)MAX_DUTY * (cap1_val + 1);
        pwm_in_duty = pwm_in_duty / (cap0_val + 1);
    }

    cap1_val = 0;
    cap0_val = 0;
    duty_buff[duty_cnt] = pwm_in_duty;
    duty_cnt++;
    if (duty_cnt == 5)
    {
        simple_over = 1;
        duty_cnt = 0;
    }
    if (simple_over == 1)
    {
        pwm_in_duty = 0;
        for (i = 0; i < 5; i++)
        {
            pwm_in_duty += duty_buff[i];
        }
        // 求平均
        pwm_in_duty = pwm_in_duty / 5;
    }
    // printf("3_1_pwm_in_duty= %d    c_duty=%d\r\n",(int)pwm_in_duty,(int)c_duty);
    // pwm_in_duty=5000;

    if (adjust_en == 1)
    {
        pwm_in_duty_xuwei = pwm_in_duty * ajust_duty / 6000;
        pwm_in_duty = pwm_in_duty_xuwei;
    }
    ex_pwm_in_duty = pwm_in_duty;
}

// 对比当前的duty是否发生变化
void cmp_duty(void)
{
    if (led_state == LED_PWR_ON)
        return;

    if (pwm_in_duty > 10)
    {
        if ((pwm_in_duty - 10) > c_duty)
        {
            led_state = LED_ADJ_P;
        }
        if ((pwm_in_duty + 10) < c_duty)
        {
            led_state = LED_ADJ_S;
        }
    }
    else
    {
        if (pwm_in_duty > c_duty)
        {
            led_state = LED_ADJ_P;
        }
        if (pwm_in_duty < c_duty)
        {
            led_state = LED_ADJ_S;
        }
    }
}

// 上电led缓慢启动
// 每10ms执行一次，c_duty每次+30
float gamma = 1;
void debug_putchar(u8 uart_data);

// float step = 90;
float step = 70;

float mi;  // 幂
float rus; // 10的幂次方
float r_ms = 0;
u16 rus_duty; // 放大60倍

// 1ms调用一次     LIN 改 加入电位器判断
void led_pwr_on(void)
{
    if (led_state == LED_PWR_ON && simple_over == 1)
    {
        // 没有电位器时
        // 没有三合一功能，上电默认最大占空比
        //  有三合一功能，按采用的占空比调光
        if (adjust_en == 0)
        {
            if (_3_1.en == 0)
            {
                pwm_in_duty = max_duty;
            }
            r_ms = step / 12;
            if (r_ms < 1)
                r_ms = 10;
            if (pwr_on_cnt < r_ms)
                return; // 时间未到不调整
            pwr_on_cnt = 0;
            if (c_duty < pwm_in_duty)
            {
                mi = (step - 1) / (253 / 3) - 1;
                step += 0.5;
                c_duty = pow(5, mi) * 60;
            }
            if (c_duty >= pwm_in_duty)
            {
                c_duty = pwm_in_duty;
                led_state = LED_NULL;
            }
            set_pwm_duty();
        }
        if (adjust_en == 1) // 有没有三合一功能，上电都默认电位器的最大占空比
        {
            if (_3_1.en == 0)
            {
                pwm_in_duty = ajust_duty;
            }
            if (_3_1.en == 1)
            {
                pwm_in_duty = pwm_in_duty_xuwei;
            }
            {
                r_ms = step / 12;
                if (r_ms < 1)
                    r_ms = 10;
                if (pwr_on_cnt < r_ms)
                    return; // 时间未到不调整
                pwr_on_cnt = 0;
                if (c_duty < pwm_in_duty)
                {
                    mi = (step - 1) / (253 / 3) - 1;
                    step += 0.5;
                    c_duty = pow(5, mi) * 60;
                }
                if (c_duty >= pwm_in_duty)
                {
                    c_duty = pwm_in_duty;
                    led_state = LED_NULL;
                }
                set_pwm_duty();
            }
        }
    }
    // printf("pwm_in_duty= %d   t= %d    t_ctrl.level=%d    c_duty=%d\r\n", (int)pwm_in_duty,(int)t_ctrl.t,(int)ajust_duty,(int)c_duty);
}

void led_plus(void)
{
    if (led_state != LED_ADJ_P)
        return;
    if (c_duty < pwm_in_duty)
    {
        if (is_pwr_down) // 缓慢升降
            c_duty += 1;
        else
            c_duty += 10;
    }

    if (c_duty >= pwm_in_duty)
    {
        c_duty = pwm_in_duty;
        led_state = LED_NULL;
        gamma = c_duty;
        is_pwr_down = 0; // 完成缓慢升降
    }
    set_pwm_duty();
}

void led_sub(void)
{
    if (led_state != LED_ADJ_S)
        return;

    if (c_duty > pwm_in_duty)
    {
        if (is_pwr_down)
            c_duty -= 1;
        else
        {
            if (c_duty > 50)
            {

                c_duty -= 10;
            }
            else
            {
                c_duty -= 1;
            }
        }
    }
    if (c_duty <= pwm_in_duty)
    {
        c_duty = pwm_in_duty;
        gamma = c_duty;
        is_pwr_down = 0; // 完成缓慢升降
        led_state = LED_NULL;
    }
    set_pwm_duty();
}

// 有没有降功率
#define LOW_PWR_R7 P00
// 有没有三合一
#define _3_1_R14 P02
// 有没有时空
#define T_C_R11 P03

#define T_C_R8 P13
#define T_C_R9 P12
#define T_C_R10 P11

// 降功率、时空、三合一功能
void option_fun(void)
{
    flag_init();
    // p13输入上拉，时空组合R8
    P1_MD0 &= ~(GPIO_P13_MODE_SEL(0x3));
    P1_PU |= (GPIO_P13_PULL_UP(0x1));
    P1_PD |= (GPIO_P13_PULL_PD(1));
    //   时空组合R9
    P1_MD0 &= ~(GPIO_P12_MODE_SEL(0x3));
    P1_PU |= (GPIO_P12_PULL_UP(0x1));
    // 时空组合R10
    P1_MD0 &= ~(GPIO_P11_MODE_SEL(0x3));
    P1_PU |= (GPIO_P11_PULL_UP(0x1));
    // 降功率有/无 R7
    P0_MD0 &= ~(GPIO_P00_MODE_SEL(0x3));
    P0_PU |= (GPIO_P00_PULL_UP(0x1));
    // 三合一有/无 R14
    P0_MD0 &= ~(GPIO_P02_MODE_SEL(0x3));
    P0_PU |= (GPIO_P02_PULL_UP(0x1));
    // 时空有/无 R11
    P0_MD0 &= ~(GPIO_P03_MODE_SEL(0x3));
    P0_PU |= (GPIO_P03_PULL_UP(0x1));

    if (LOW_PWR_R7 == 0) // 低电平有降功率
    {
        l_pwr.en = 1;
        printf("l_pwr.en = 1\r\n");
    }
    else
    {
        l_pwr.en = 0;
        printf("l_pwr.en = 0\r\n");
    }
    if (adc0_val > 4000) // LIN 改 加入了ntc和电位器AD值的判断 ntc和电位器都上拉
    {
        ex_temp_en = 0;
        printf("ex_temp_en = 0\r\n");
    }
    else
    {
        ex_temp_en = 1;
        printf("ex_temp_en = 1\r\n");
    }
    if (adc1_val > 4000)
    {
        adjust_en = 0;
        printf("adjust_en = 0\r\n");
    }
    else
    {
        adjust_en = 1;
        printf("adjust_en = 1\r\n");
    }
    if (_3_1_R14 == 1) // 高电平有三合一
    {
        _3_1.en = 1;
        // simple_over = 1;
        printf("_3_1.en  = 1\r\n");
    }
    else
    {
        _3_1.en = 0;
        simple_over = 1;
        printf("_3_1.en  = 0\r\n");
    }

    // 低电平时空
    if (T_C_R11 == 0) //  0
    {
        t_ctrl.en = 1;
        printf("t_ctrl.en = 1\r\n");
    }
    else
    {
        t_ctrl.en = 0;
        printf("t_ctrl.en = 0\r\n");
    }

    // 时控模式0：100%6H 50%6H 0%
    // 时控模式1：100%4H 50%8H 0%
    // 时控模式2：100%5H 70%3H 50%4H 0%
    // 时控模式3：100%4H 70%4H 50%4H 0%
    // 时控模式4：100%4H 60%2H 30%3H 60%2H 100%1H 0%

    if (T_C_R8 == 1 && T_C_R9 == 1 && T_C_R10 == 1)
    {
        t_ctrl.list = 0;
        p_list = &t_ctrl_lis_0;
        printf("t_ctrl.list = 0 \r\n");
    }
    else if (T_C_R8 == 0 && T_C_R9 == 1 && T_C_R10 == 1)
    {
        t_ctrl.list = 1;
        p_list = &t_ctrl_lis_1;
        printf("t_ctrl.list = 1\r\n ");
    }
    else if (T_C_R8 == 1 && T_C_R9 == 0 && T_C_R10 == 1)
    {
        t_ctrl.list = 2;
        p_list = &t_ctrl_lis_2;
        printf("t_ctrl.list = 2\r\n ");
    }
    else if (T_C_R8 == 1 && T_C_R9 == 1 && T_C_R10 == 0)
    {
        t_ctrl.list = 3;
        p_list = &t_ctrl_lis_3;
        printf("t_ctrl.list = 3\r\n ");
    }
    else if (T_C_R8 == 0 && T_C_R9 == 0 && T_C_R10 == 1)
    {
        t_ctrl.list = 4;
        p_list = &t_ctrl_lis_4;
        printf("t_ctrl.list = 4\r\n");
    }
}

// 降功率计时处理
void l_pwr_timer_handler(void)
{
    if (l_pwr.en == 1)
    {
        if (l_pwr.t < L_PWR_T) // 小时
        {
            l_pwr.t++;
        }
        else
        {
            extemp_tctrl_flag = 1;
            if (adjust_en == 1) // 电位器使能
            {
                if (_3_1.en == 1) // 有三合一功能和电位器
                {
                    // 判断PWM_in占空比大于74
                    if (pwm_in_duty_xuwei > ajust_duty / 100 * 75) // LIN 改  75%改成电位器的75%
                    {
                        max_duty = ajust_duty / 100 * 75;
                        l_pwr.act = 1; // 降功率状态
                        l_pwr.en = 0;  // 处理完成，使能降功率
                        is_pwr_down = 1;
                    }
                }
                else
                {
                    max_duty = ajust_duty / 100 * 75;
                    l_pwr.act = 1; // 降功率状态
                    l_pwr.en = 0;  // 处理完成，使能降功率
                    is_pwr_down = 1;
                }
            }

            if (adjust_en == 0) // 无电位器
            {
                if (_3_1.en) // 有三合一功能
                {
                    // 判断PWM_in占空比大于74
                    if (pwm_in_duty > _75_DUTY)
                    {
                        max_duty = _75_DUTY;
                        l_pwr.act = 1; // 降功率状态
                        l_pwr.en = 0;  // 处理完成，使能降功率
                        is_pwr_down = 1;
                    }
                }
                else
                {
                    max_duty = _75_DUTY;
                    l_pwr.act = 1; // 降功率状态
                    l_pwr.en = 0;  // 处理完成，使能降功率
                    is_pwr_down = 1;
                }
            }
            l_pwr.en = 0;
            ex_max_duty = max_duty;
        }
    }
}

// 处理降功率，时控时，可调的占空比范围
// 延时开机模式，强制pwm_in_duty=0
void pwm_in_handler(void)
{
    // if(_3_1.en)     //有三合一功能
    {
        // 时控和降功率激活状态，最大占空比不超过降功率，时控, LIN 改 过温降功率设定的占空比
        //			if(t_ctrl.en == 1)
        //			{
        //				if(_3_1.en==1)
        //                {
        //                    pwm_in_duty = pwm_in_duty;
        //                }
        //
        //				if(adjust_en==1)
        //				{
        //					pwm_in_duty = ajust_duty;
        //				}
        //				if(_3_1.en==0&&adjust_en==0)
        //				{
        //					pwm_in_duty = max_duty;
        //				}
        //			}
        if (t_ctrl.en == 0 && adjust_en == 1)
        {
            if (_3_1.en == 1)
            {
                pwm_in_duty = pwm_in_duty_xuwei;
            }
            else
            {
                pwm_in_duty = ajust_duty;
            }
        }
        if (l_pwr.act == 1 || t_ctrl.en == 1)
        {
            // 进入降功率模式

            if (adjust_en == 1 && _3_1.en == 1)
            {

                if (pwm_in_duty_xuwei > max_duty)
                {
                    pwm_in_duty_xuwei = max_duty;
                }
                pwm_in_duty = pwm_in_duty_xuwei;
            }
            else
            {
                if (pwm_in_duty > max_duty)
                {
                    pwm_in_duty = max_duty;
                }
            }
        }
        if (ex_temp_en == 1 && t_ctrl.en == 0)
        {
            if (adjust_en == 0 && _3_1.en == 0)
            {
                pwm_in_duty = max_duty;
            }
            if (pwm_in_duty > max_duty)
            {
                pwm_in_duty = max_duty;
            }
            if (max_duty == 0)
            {
                pwm_in_duty = 0;
            }
        }
    }
    // is_pwr_down=0;
    //    if(dly_pwr_on == 1)
    //    {
    //        // 延时启动，强制pwm 0
    //        pwm_in_duty = 0;
    //    }
}

// 1s调用一次
void t_ctrl_timer_handler(void)
{
    if (t_ctrl.en == 1)

    {
        // 当前列表时间非255，如果是255则说明时空结束
        if (p_list[t_ctrl.level].t != 255)
        {
            //			t_ctrl.list = 4;
            //        p_list = &t_ctrl_lis_4;
            t_ctrl.t++;
            if (t_ctrl.t >= (p_list[t_ctrl.level].t * 60 * 60)) // 改成10s
            // if(t_ctrl.t >= (p_list[t_ctrl.level].t * 10) )
            {
                t_ctrl.level++;
                is_pwr_down = 1;
                t_ctrl.t = 0;
                max_duty = p_list[t_ctrl.level].b;
                extemp_tctrl_flag = 1;
                if (adjust_en == 1) // LIN 改 加入电位器时 将最大值变成电位器的最大值
                {
                    switch (max_duty)
                    {
                    case 6000:
                        max_duty = ajust_duty;
                        break;
                    case 4200:
                        max_duty = ajust_duty / 10 * 7;
                        break;
                    case 3600:
                        max_duty = ajust_duty / 10 * 6;
                        break;
                    case 3000:
                        max_duty = ajust_duty / 2;
                        break;
                    case 1800:
                        max_duty = ajust_duty / 10 * 3;
                        break;
                    default:
                        break;
                    }
                }
                if (_3_1.en == 1 && ex_temp_en == 1 && t_ctrl.en == 0)
                {
                    //	if(adjust_en==0)
                    {
                        if (ex_pwm_in_duty < max_duty)
                        {
                            max_duty = ex_pwm_in_duty;
                        }
                    }
                    //								else
                    //								{
                    //									if(pwm_in_duty_xuwei<max_duty)
                    //									{
                    //										max_duty=pwm_in_duty_xuwei;
                    //									}
                    //								}
                }
                pwm_in_duty = max_duty; // 修复t_ctrl_lis_4，功率降到30%升不了60%
                if (adjust_en == 1 && _3_1.en == 1)
                {
                    pwm_in_duty_xuwei = max_duty;
                }
            }
        }

        //				if(p_list[t_ctrl.level].t != 0)
        //				{
        //					t_ctrl.level=0;
        //				}
        // is_pwr_dow=0;
    }
    // printf("pwm_in_duty= %d  t= %d  ajust_duty=%d  max_duty=%d adjust_val=%d pwm_save[0]=%d\r\n", (int)pwm_in_duty,(int)t_ctrl.t,(int)ajust_duty,(int)c_duty,(int)adjust_val,(int)pwm_save[0]);
    // printf("pwm_in_duty= %d   t= %d    t_ctrl.level=%d    c_duty=%d\r\n", (int)pwm_in_duty,(int)t_ctrl.t,(int)ajust_duty,(int)c_duty);
}

// 1秒调用一次
extern u8 lvd_mode;
extern volatile u8 lvd_flag;
void dly_pwr_on_handler(void)
{
    if (dly_pwr_on == 1) // 延时启动
    {
        user_printf("lvd pwr low \n");
        dly_pwr_on_cnt++;
        if (dly_pwr_on_cnt >= 5)
        {
            lvd_flag = 0;
            dly_pwr_on_cnt = 0;

            dly_pwr_on = 0;

            c_duty = 0;
            max_duty = MAX_DUTY;

            t_ctrl.list = 0;
            t_ctrl.t = 0;
            t_ctrl.en = 0;
            t_ctrl.level = 0;

            l_pwr.en = 0;
            l_pwr.act = 0;
            l_pwr.t = 0;

            _3_1.en = 0;
            pwm_in_duty = 0;
            ex_temp_en = 0;
            adjust_en = 0;
            gamma = 2;
            option_fun();

            led_state = LED_PWR_ON;
            printf("LED_PWR_ON");
        }
    }
}
void flag_init()
{
    lvd_flag = 0;
    dly_pwr_on_cnt = 0;

    dly_pwr_on = 0;

    c_duty = 0;
    max_duty = MAX_DUTY;

    t_ctrl.list = 0;
    t_ctrl.t = 0;
    t_ctrl.en = 0;
    t_ctrl.level = 0;

    l_pwr.en = 0;
    l_pwr.act = 0;
    l_pwr.t = 0;

    _3_1.en = 0;
    pwm_in_duty = 0;
    ex_temp_en = 0;
    adjust_en = 0;
    gamma = 2;
}