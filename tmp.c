#define LEVLE_PER 92  // 3686/40档=92
#define DEAD_ZONE 5   // ADC死区范围±5
extern u16 adjust_val; // 电位器的ADC值 0~3686---->60%~100%---->3600~6000
u16 ajust_duty;
void adjust_pwm() // 40档电位器调节pwm 60%~100%,每档增加1%
{
    if (adjust_en == 1)
    {
        if (adjust_val > 3680) // 4.5V对应的AD值
        {
            ajust_duty = 6000;
            return;
        }

        // 计算当前档位
        u8 level = adjust_val / LEVLE_PER;
        
        // 判断是否在死区范围内
        u16 level_center = level * LEVLE_PER + LEVLE_PER/2;
        if(adjust_val > level_center - DEAD_ZONE && 
           adjust_val < level_center + DEAD_ZONE)
        {
            // 在死区内保持当前值
            return;
        }

        // 限制最小档位
        if(level == 0) {
            ajust_duty = 3600;  // 60%
            return;
        }

        // 限制最大档位
        if(level >= 40) {
            ajust_duty = 6000;  // 100%
            return;
        }

        // 计算PWM值: 3600 + level * 60
        // 3600(60%) 到 6000(100%)分40档,每档增加 (6000-3600)/40 = 60
        ajust_duty = 3600 + level * 60;
    }
}