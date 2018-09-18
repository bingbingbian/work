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
*  �� �� ��: �ʺ���
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
#include "stm32f10x_adc.h"
#include "shell.h"
/*
********************************************************************************
*  ��������: ADC_Configuration
*
*  ��������:ADC ��ʼ��
*
*  �������: ��
*
*  �������: ��
*
*  �� �� ֵ: ��
*
********************************************************************************
*/

#include "Public.h"
#include <string.h>

#define ADC_RESULT_IN_MILLI_VOLTS(ADC_VALUE)\
          ((((ADC_VALUE) * 439)  * 2 )/ 1024)


#define VOLTAGE_AVG_NUM 6

u16 batt_lvl_in_milli_volts;
static uint32_t battery_voltage_arrary[VOLTAGE_AVG_NUM]={0};

u8 batteryvolt[10] = {0x33, 0x00, 0x04};


void ADC_Configuration(void)
{
	
		ADC_InitTypeDef ADC_InitStructure; 

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);	
    RCC_ADCCLKConfig(RCC_PCLK2_Div6); //ADC ʱ��12M hz
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent; 
    ADC_InitStructure.ADC_ScanConvMode =DISABLE;//��ͨ��ɨ��
    ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;//����ת�� ENABLE
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;//2//�������ⲿ����ת��
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right; //����������Ҷ���
    ADC_InitStructure.ADC_NbrOfChannel=1;//ֻ��һ��ͨ����Ҫ����
    ADC_Init(ADC1, &ADC_InitStructure);
    ADC_RegularChannelConfig(ADC1, ADC_Channel_1,1,ADC_SampleTime_55Cycles5 );//���ò���ͨ��������Ͳ���ʱ��
    ADC_Cmd(ADC1, ENABLE);//ʹ��ADC
    ADC_ResetCalibration(ADC1); //����У׼
    while(ADC_GetResetCalibrationStatus(ADC1));//�ȴ�����У׼���
    ADC_StartCalibration(ADC1);//����У׼
    while(ADC_GetCalibrationStatus(ADC1)); //�ȴ�У׼���
    ADC_SoftwareStartConvCmd(ADC1, ENABLE); //��ʼ��������ת��
}

void ADC1_SAMPLING(void)
{
    u16 rdata;
    int i,j;
	  u8 battery_voltage_arrary_index;
	  //���ڱ���ת�������ĵ�ѹֵ 	 
		float ADC_ConvertedValueLocal;
	  char batteryvoltstr[4];
	
	  batt_lvl_in_milli_volts = 0;
	  battery_voltage_arrary_index = 0;
	  ADC_ConvertedValueLocal = 0;
    while(ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);//ͨ���ж�EOC�Ƿ�����
    ADC_ClearITPendingBit(ADC1, ADC_FLAG_EOC);//���ת����־λ
	
	 
    rdata = ADC_GetConversionValue(ADC1);
    batt_lvl_in_milli_volts = ADC_RESULT_IN_MILLI_VOLTS(rdata);
    for(i = 0;i < VOLTAGE_AVG_NUM; i++)
	     battery_voltage_arrary[i] = 0;
    if( battery_voltage_arrary[0] == 0 ){
		battery_voltage_arrary_index = 0;
		for( i = 0; i < VOLTAGE_AVG_NUM; i++ ){
			battery_voltage_arrary[i] = batt_lvl_in_milli_volts;
			}
    }else{
         battery_voltage_arrary[battery_voltage_arrary_index] = batt_lvl_in_milli_volts;
         battery_voltage_arrary_index = ( battery_voltage_arrary_index + 1 )%VOLTAGE_AVG_NUM;
         batt_lvl_in_milli_volts = 0;
         for( j = 0; j < VOLTAGE_AVG_NUM; j++ ){
               batt_lvl_in_milli_volts += battery_voltage_arrary[j];
         }
         batt_lvl_in_milli_volts /= VOLTAGE_AVG_NUM;
    }
		RTU_DEBUG("batt_lvl_in_milli_volts<%D>\r\n", batt_lvl_in_milli_volts);
		ADC_ConvertedValueLocal = ((float)batt_lvl_in_milli_volts*100.0)/10000;//110.0
	
		if(ADC_ConvertedValueLocal >= 19.8&&ADC_ConvertedValueLocal<27.8)
			;
		else if(ADC_ConvertedValueLocal >= 27.8&&ADC_ConvertedValueLocal<36.0)
			ADC_ConvertedValueLocal = ADC_ConvertedValueLocal+0.2;
		else if(ADC_ConvertedValueLocal >= 10.0&&ADC_ConvertedValueLocal<27.8)
			ADC_ConvertedValueLocal = ADC_ConvertedValueLocal-0.2;
		else if(ADC_ConvertedValueLocal >= 7.0&&ADC_ConvertedValueLocal<10.0)
			;
		f2s(ADC_ConvertedValueLocal,batteryvoltstr);
		memcpy(&batteryvolt[3],batteryvoltstr,4);
		
}






