#ifndef __mani_h
#define __mani_h
typedef enum
{
  LED_NULL = 0,
  LED_PWR_ON, //上电模式,禁止调光
  LED_ADJ_P,    //调光模式+
  LED_ADJ_S,    //调光模式-
}LED_S;



extern LED_S led_state;



#endif