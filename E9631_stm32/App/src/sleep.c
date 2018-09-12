/*
********************************************************************************
*  Copyright (C), 2009-2013, ����˼׳�����з�����
*
*  ��Ŀ���ƣ�xxxx
*  
*  �ļ�����: xxxx.c
*
*  �ļ�������xxxx
*             
*             
*  �� �� ��: Τ����
*
*  �������ڣ�2013-03-18 
*
*  �� �� �ţ�V1.0
*
*  �޸ļ�¼�� 
*             
*      1. ��    �ڣ� 
*         �� �� �ˣ� 
*         �����޸ģ� 
*      2. ...
********************************************************************************
*/

#include "shell.h"
#include "sleep.h"
#include "stm32f10x_pwr.h"


SLEEP_T sleepmode;

//extern void AppInit(void);

#define RCC_PLLSource_HSE_Div1           ((u32)0x00010000)

/**********************************************************************
* ����:RCC_Configuration()
* ����:����ʱ��
* ��ڲ���: 
* ���ڲ���:
-----------------------------------------------------------------------
* ˵��:
***********************************************************************/
void RCC_Configuration(void)
{
    ErrorStatus HSEStartUpStatus;

    //ʹ���ⲿ����
    RCC_HSEConfig(RCC_HSE_ON);
    //�ȴ��ⲿ�����ȶ�
    HSEStartUpStatus = RCC_WaitForHSEStartUp();
    //����ⲿ���������ɹ����������һ������
    if(HSEStartUpStatus==SUCCESS)
    {
        //����HCLK(AHBʱ��)=SYSCLK
        RCC_HCLKConfig(RCC_SYSCLK_Div1);

        //PCLK1(APB1) = HCLK/2
        RCC_PCLK1Config(RCC_HCLK_Div2);

        //PCLK2(APB2) = HCLK
        RCC_PCLK2Config(RCC_HCLK_Div1);
        
 
        RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_4);
        //����PLL
        RCC_PLLCmd(ENABLE);
        //�ȴ�PLL�ȶ�
        while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET);
        //ϵͳʱ��SYSCLK����PLL���
        RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
        //�л�ʱ�Ӻ�ȴ�ϵͳʱ���ȶ�
        while(RCC_GetSYSCLKSource()!=0x08);  
     }
	  /* RCC system reset(for debug purpose) */ 
}



//����
void mcu_makeup(void)
{
		RCC_Configuration();
		//AppInit();
		RTU_DEBUG("mcu_makeup\r\n");
		sleepmode = SYSTEM_RUN;
}

void mcu_sleep(void)
{
	  sleepmode = SYSTEM_SLEEP;
		//����ֹͣģʽ(�͹���)��ֱ���ⲿ�жϴ���ʱ������
		PWR_EnterSTOPMode(PWR_Regulator_LowPower,PWR_STOPEntry_WFI);
}






