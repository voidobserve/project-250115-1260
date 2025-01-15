/**
  ******************************************************************************
  * @file    system.c
  * @author  HUGE-IC Application Team
  * @version V1.0.0
  * @date    01-05-2021
  * @brief   Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2021 HUGE-IC</center></h2>
  *
  *
  *
  ******************************************************************************
  */ 

/* Includes ------------------------------------------------------------------*/
#include "include.h"

/** @addtogroup Template_Project
  * @{
  */
  
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Delay function
  * @param  cnt : the number of cnt to delay
  * @retval None
  */
void delay(u16 cnt)
{
    while(cnt--) {
    }
}

/**
  * @brief  Delay function
  * @param  ms : the number of ms to delay
  * @retval None
  */
void delay_ms(u16 ms)
{
    while(ms--) {
        delay(1450);
    }
}

/**
  * @brief  System initialization function
  * @param  None
  * @retval None
  */
void system_init(void)
{
    u8 i;
    
    CLK_ACON0 |= CLK_AIP_HRC_EN(0x1);           // ʹ�� HIRC
    __HIRC_DELAY;                               // ���ӳٲ���ɾ��,�ȴ�HIRC�ȶ����ܱ�֤��¼�ȶ���
    FLASH_TIMEREG0 = 0x55;
    FLASH_TIMEREG1 = 0x58;                      // FLASH�����ٶ� = ϵͳʱ��/3
    CLK_CON2 = 0x50 | CLK_SYS_DIV(0x0);         // ϵͳʱ�Ӳ���Ƶ
    CLK_CON0 = CLK_SYSCLK_SEL(0x3);             // ϵͳʱ��ѡ�� HIRC48M
    CLK_CON6 = CLK_FLASH_DIV(47);               // FLASH��дʱ��48��Ƶ��1M
    
    // ��ʼ���Ĵ���
    // FOUT_S00 ~ FOUT_SEL
    for(i=0x18; i<=0x32; i++) {
        *(unsigned char volatile xdata *)(IOFUNC_BASE + i) = 0x0;
    }
    
    __LOADER_CHIP_TRIM;                         // װ��оƬ����ģ��У׼ֵ
}

/*************************** (C) COPYRIGHT 2021 HUGE-IC ***** END OF FILE *****/
