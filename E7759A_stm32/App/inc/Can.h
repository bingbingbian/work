/*
**********************************************************************************************************
*  Copyright (C), 2009-2013, 合众思壮西安研发中心
*
*  项目名称： xxxx
*  
*  文件名称:  xxxx.h
*
*  文件描述： xxxx
*             
*             
*  创 建 者: 皇海明
*
*  创建日期：2013-03-18 
*
*  版 本 号：V1.0
*
*  修改记录： 
*             
*      1. 日    期： 
*         修 改 人： 
*         所作修改： 
*      2. ...
**********************************************************************************************************
*/
#ifndef __CAN_H
#define __CAN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "Uart.h"
#include "CanDrv.h"	
#include "stm32f10x.h"
#include "Obd.h"
#include "J1939.h"
	
#define CANCHANNEL 1
#define CANID 4
/*
*********************************************************************
*  全局宏定义
*********************************************************************
*/
 



/*
*********************************************************************
*  类、结构体类型定义
*********************************************************************
*/


/*
*********************************************************************
*  外部引用变量声明
*********************************************************************
*/

/*
*********************************************************************
*  外部引用函数声明
*********************************************************************
*/
void CanToUtcp(u8 channel, CANX * pcan, COMX * pcom);
void UartDataToCanData(u8 len, u8 *Data);
void Send_singleframe(void *handle);
DiagnoticIdType Get_Id_type(u32 id);
void J1939_CAN_Transmit(J1939_MESSAGE_T *MsgPtr,void *handle);
extern void J1939_CAN_SetFilterExt(u32 ID);
void J1939_ReceiveMessages(u8 channel, CANX * pcan, COMX * pcom);
#ifdef __cplusplus
}
#endif

#endif /* __CAN_H */

