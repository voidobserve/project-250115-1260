#include "include.h"
#include "main.h"
u16 ntc_val;
u16 adjust_val;
u16 ntc_val_all;
u16 adjust_val_all;
u8 adc_cnt=0;
u8 loop_cnt=0;
u16 adc0_val;
u16 adc1_val;
//    P1_MD0 &=  ~(GPIO_P13_MODE_SEL(0x3));
//    P1_PU |=  (GPIO_P13_PULL_UP(0x1));
//    P1_PD |=    (GPIO_P13_PULL_PD(1));
void adc_init()
{
	    // ����P27 p30Ϊģ������ģʽ ��������
    P2_MD0   |= GPIO_P27_MODE_SEL(0x3);                       // P27��Ϊģ��ģʽ  PIN9
    P3_MD0   |= GPIO_P30_MODE_SEL(0x3);                       // P30��Ϊģ��ģʽ  PIN8
	  P2_PU |= (GPIO_P27_PULL_UP(0x1));                         // P27ʹ������      PIN9  ���ڳ�ʼ���ж��Ƿ����ntc                 
    P2_PD |= (GPIO_P27_PULL_PD(0x0));                         // P27�ر�����
	  P3_PU |= (GPIO_P30_PULL_UP(0x1));                         // P30ʹ������      PIN8  ���ڳ�ʼ���ж��Ƿ�����λ��
    P3_PD |= (GPIO_P30_PULL_PD(0x0));                         // P30�ر�����
    //ADC����
    ADC_ACON1 &= ~(ADC_VREF_SEL(0x7) | ADC_EXREF_SEL(0x1));   // �ر��ⲿ�ο���ѹ
    ADC_ACON1 |= ADC_VREF_SEL(0x6) |                          // ѡ���ڲ��ο���ѹVCCA  5v
                 ADC_TEN_SEL(0x3);
    ADC_ACON0  = ADC_CMP_EN(0x1)  |                           // ��ADC�е�CMPʹ���ź�
                 ADC_BIAS_EN(0x1) |                           // ��ADCƫ�õ�����ʹ�ź�
                 ADC_BIAS_SEL(0x1);
	
    ADC_CHS0   = ADC_ANALOG_CHAN(0x17) |                      // P27ͨ·
                 ADC_EXT_SEL(0x0);                            // ѡ���ⲿͨ·
    ADC_CFG0  |= ADC_CHAN0_EN(0x1) |                          // ʹ��ͨ��0ת��
                 ADC_EN(0x1);                                 // ʹ��A/Dת��
	
    ADC_CHS1   = ADC_ANALOG_CHAN(0x18) |                      // P30ͨ·
                 ADC_EXT_SEL(0x0);                            // ѡ���ⲿͨ·
    ADC_CFG0  |= ADC_CHAN1_EN(0x1) |                          // ʹ��ͨ��1ת��
                 ADC_EN(0x1);                                 // ʹ��A/Dת��
	
	  delay_ms(1);                                              // �ȴ�ADCģ�������ȶ�����Ҫ�ȴ�20us����
}

void adc_val()
{
	      ADC_CFG0 |= ADC_CHAN0_TRG(0x1);                       // ����ADC0ת��
        while(!(ADC_STA & ADC_CHAN0_DONE(0x1)));              // �ȴ�ת�����
        ADC_STA  = ADC_CHAN0_DONE(0x1);                       // ���ADC0ת����ɱ�־λ
        adc0_val = (ADC_DATAH0 << 4) | (ADC_DATAL0 >> 4);     // ��ȡchannel0��ֵ
	      
	      
        ADC_CFG0 |= ADC_CHAN1_TRG(0x1);                       // ����ADC0ת��
        while(!(ADC_STA & ADC_CHAN1_DONE(0x1)));              // �ȴ�ת�����
        ADC_STA  = ADC_CHAN1_DONE(0x1);                       // ���ADC0ת����ɱ�־λ
			  adc1_val = (ADC_DATAH1 << 4) | (ADC_DATAL1 >> 4);     // ��ȡchannel1��ֵ

	      ntc_val_all+=adc0_val;                        
	      adjust_val_all+=adc1_val;
	     
         
	      adc_cnt++;
        if(adc_cnt==10)//��ֵ�˲�
				{
					ntc_val = ntc_val_all/10;
				  adjust_val=adjust_val_all/10;
					adc_cnt=0;
					ntc_val_all=0;
					adjust_val_all=0;

				}
				
}
