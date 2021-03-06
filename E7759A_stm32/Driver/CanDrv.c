/*
********************************************************************************
*  Copyright (C), 2009-2012, 合众思壮西安研发中心
*
*  项目名称：VS331
*  
*  文件名称: CanDrv.c
*
*  文件描述：CAN硬件驱动源程序
*             
*             
*  创 建 者: 皇海明
*
*  创建日期：2014-04-04 
*
*  版 本 号：V1.0
*
*  修改记录： 
*             
*      1. 日    期： 
*         修 改 人： 
*         所作修改： 
*      2. ...
********************************************************************************
*/

#define CANDRV_GLOBALS

#include "CanDrv.h"
#include "Shell.h"
#include <string.h>

// CAN输入队列定义
#if  LIST_CAN1_IN_NUM > 4
static CANQUEUELIST  QueCAN1In ;      /* CAN1输入队列 */
static CANQUEDATA    CAN1InBuffer[LIST_CAN1_IN_NUM];
#endif

#if  LIST_CAN2_IN_NUM > 4
static CANQUEUELIST  QueCAN2In ;      /* CAN2输入队列 */
static CANQUEDATA    CAN2InBuffer[LIST_CAN2_IN_NUM];
#endif

// CAN输出队列定义
#if LIST_CAN1_OUT_NUM > 4
static  CANQUEUELIST  QueCAN1Out;      /* CAN1输出队列 */
static  CANQUEDATA    CAN1OutBuffer[LIST_CAN1_OUT_NUM];
#endif

#if LIST_CAN2_OUT_NUM > 4
static  CANQUEUELIST  QueCAN2Out;      /* CAN2输出队列 */
static  CANQUEDATA    CAN2OutBuffer[LIST_CAN2_OUT_NUM];
#endif

/*
********************************************************************************
*  函数名称: CANQueListInit
*
*  功能描述: 对列初始化
*
*  输入参数: plist  队列指针
*            pbuf   对列数据缓冲区指针
*            len    对列的数据缓冲区长度
*
*  输出参数: 无
*
*  返 回 值: 无
*
********************************************************************************
*/
void CANQueListInit(CANQUEUELIST *plist, CANQUEDATA * pbuf, u16 len)
{
    // 参数判断
    if(plist == NULL)
    {
        return;
    }

    // 初始化队列
    plist->head  = 0;
    plist->trail = 0;
    plist->pdata = pbuf;

    // 数据指针为空时，将队列长度设置为0
    if (pbuf == NULL)
    {
        plist->lenmax = 0;
    }
    else
    {
        plist->lenmax = len;
    }   
}

/*
********************************************************************************
*  函数名称: CANQueListGet
*
*  功能描述: 从CAN输出队列中获取一组数据
*
*  输入参数: pmsg_queue  CAN队列指针
*            rdata       获取数据指针
*
*  输出参数: 无
*
*  返 回 值: 0 - 队列中无数据，获取失败  1 - 获取数据成功
*
********************************************************************************
*/
u8  CANQueListGet(CANQUEUELIST *pmsg_queue, CANQUEDATA *rdata)
{
    u16 i;
    CANQUEDATA * pcur;
    
    // 参数检查
    if(pmsg_queue == NULL || pmsg_queue->pdata == NULL || pmsg_queue->lenmax == 0)
    {
        return 0;
    }

    // 数据为空
    if(pmsg_queue->head == pmsg_queue->trail)
    {
        return 0;
    }

    // 取出数据
    pcur = pmsg_queue->pdata + pmsg_queue->head;
    rdata->Dlc = pcur->Dlc;
    rdata->Id  = pcur->Id;
    for(i = 0; i < rdata->Dlc; i++)
    {
        rdata->Data[i] = pcur->Data[i]; 
    }
    pmsg_queue->head = (pmsg_queue->head + 1) % pmsg_queue->lenmax;
    
    return 1;
}


/*
********************************************************************************
*  函数名称: CANQueListPost
*
*  功能描述: 向CAN输出队列中增加一组数据
*
*  输入参数: pmsg_queue  CAN队列指针
*            dat         要写入的数据
*
*  输出参数: 无
*
*  返 回 值: 1 - 写入失败  0 - 写入成功
*
********************************************************************************
*/
u8  CANQueListPost(CANQUEUELIST *pmsg_queue, CANQUEDATA *dat)
{
    u16 tmp, i;
    CANQUEDATA * pcur;
    
    // 参数检查
    if(pmsg_queue == NULL || pmsg_queue->pdata == NULL || pmsg_queue->lenmax == 0)
    {
        return 1;
    }

    // 队列已经满
    tmp = (pmsg_queue->trail + 1) % pmsg_queue->lenmax;
    if(tmp == pmsg_queue->head)
    {
        return 1;
    }

    // 写入数据
    if (dat->Dlc > 8)
    {
        dat->Dlc = 8;
    }

    pcur = pmsg_queue->pdata + pmsg_queue->trail;
    pcur->Dlc = dat->Dlc;
    pcur->Id  = dat->Id;
    for(i = 0; i < dat->Dlc; i++)
    {
        pcur->Data[i] = dat->Data[i]; 
    }
    
    pmsg_queue->trail = tmp;

    return 0;
}

/*
********************************************************************************
*  函数名称: CAN1Init
*
*  功能描述: CAN1硬件初始化
*
*  输入参数: 无
*
*  输出参数: 无
*
*  返 回 值: 无
*
********************************************************************************
*/
void CAN1Init(CANX *pcan, CANXBAUD baud,u8 isFilter)
{
    u8 i;
    GPIO_InitTypeDef        GPIO_InitStructure;
    CAN_FilterInitTypeDef   CAN_FilterInitStructure;
    CAN_InitTypeDef         CAN_InitStructure;
    NVIC_InitTypeDef        NVIC_InitStructure;

        // 输入队列初始化
#if LIST_CAN1_IN_NUM > 4   
    CANQueListInit(&QueCAN1In, CAN1InBuffer, LIST_CAN1_IN_NUM);
    pcan->pQueueIn = &QueCAN1In;
#else
    pcan->pQueueIn = NULL;
#endif
    
    pcan->pCanx = CAN1;

    // 使能外设时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);
    
    // 配置管脚到AF功能
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_8;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // 重新映射CAN1引脚，RX->PB8 TX->PB9
    GPIO_PinRemapConfig(GPIO_Remap1_CAN1, ENABLE);

    // 中断设置
    NVIC_InitStructure.NVIC_IRQChannel = CAN1_TX_IRQn ;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = PREEMPRIO_CAN1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = SUBPRIO_CAN1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    NVIC_InitStructure.NVIC_IRQChannel = CAN1_RX0_IRQn ;
    NVIC_Init(&NVIC_InitStructure);
    NVIC_InitStructure.NVIC_IRQChannel = CAN1_RX1_IRQn ;
    NVIC_Init(&NVIC_InitStructure);
    NVIC_InitStructure.NVIC_IRQChannel = CAN1_SCE_IRQn ;
    NVIC_Init(&NVIC_InitStructure);

    /* CAN register init */
    CAN_DeInit(CAN1);
    CAN_StructInit(&CAN_InitStructure);

    /* CAN1 cell init */
    CAN_InitStructure.CAN_TTCM = DISABLE;       // Enable or disable the time triggered communication mode.
    CAN_InitStructure.CAN_ABOM = ENABLE;        // Enable or disable the automatic bus-off management.
    CAN_InitStructure.CAN_AWUM = DISABLE;       // Enable or disable the automatic wake-up mode. 
    CAN_InitStructure.CAN_NART = ENABLE;        // Enable or disable the non-automatic retransmission mode.
    CAN_InitStructure.CAN_RFLM = ENABLE;        // Enable or disable the Receive FIFO Locked mode.
    CAN_InitStructure.CAN_TXFP = ENABLE;        // Enable or disable the transmit FIFO priority.
    CAN_InitStructure.CAN_Mode = CAN_Mode_Normal;
    /* BaudRate:
                36M                  1               2M
        --------------------- * ----------- =  ----------------		 //250K
        (SJW+(BS1)+(BS2))         (BRP)         CAN_Prescaler
    */
    CAN_InitStructure.CAN_SJW = CAN_SJW_1tq;
    CAN_InitStructure.CAN_BS1 = CAN_BS1_9tq;
    CAN_InitStructure.CAN_BS2 = CAN_BS2_8tq;
    CAN_InitStructure.CAN_Prescaler = 2000000 / baud;
    CAN_Init(CAN1, &CAN_InitStructure);

    // 接收屏蔽全部禁止
	if(isFilter == 1)
	{
		for(i = 0; i < 28; i++)
    	{
        	CAN_FilterInitStructure.CAN_FilterNumber = i;
        	CAN_FilterInitStructure.CAN_FilterMode   = CAN_FilterMode_IdMask;
        	CAN_FilterInitStructure.CAN_FilterScale  = CAN_FilterScale_32bit;
        	CAN_FilterInitStructure.CAN_FilterIdHigh         = 0;
        	CAN_FilterInitStructure.CAN_FilterIdLow          = 0;
        	CAN_FilterInitStructure.CAN_FilterMaskIdHigh     = 0;
        	CAN_FilterInitStructure.CAN_FilterMaskIdLow      = 0;
        	CAN_FilterInitStructure.CAN_FilterFIFOAssignment = 0;
        	CAN_FilterInitStructure.CAN_FilterActivation     = ENABLE;
        	CAN_FilterInit(&CAN_FilterInitStructure);
    	}
	}

		
#ifdef CANQUE_USEOS
    // 串口多字节输出保护信号量
    pcan->pWrBufSem = OSSemCreate(1);
#endif
    
    // 输出缓冲初始化
#if LIST_CAN1_OUT_NUM > 4  
    CAN_ITConfig(CAN1, CAN_IT_TME, ENABLE);
    CANQueListInit(&QueCAN1Out, CAN1OutBuffer, LIST_CAN1_OUT_NUM);
    pcan->pQueueOut = &QueCAN1Out; 
#else
    CAN_ITConfig(CAN1, CAN_IT_TME, DISABLE);
    pcan->pQueueOut = NULL;
#endif

    // 输入缓冲初始化
#if LIST_CAN1_IN_NUM > 4 
    CAN_ITConfig(CAN1, CAN_IT_FMP0, ENABLE);
    CAN_ITConfig(CAN1, CAN_IT_FMP1, ENABLE);
    CANQueListInit(&QueCAN1In, CAN1InBuffer, LIST_CAN1_IN_NUM);
    pcan->pQueueIn = &QueCAN1In; 
#else
    CAN_ITConfig(CAN1, CAN_IT_FMP0, DISABLE);
    CAN_ITConfig(CAN1, CAN_IT_FMP1, DISABLE);
    pcan->pQueueIn = NULL;
#endif

    /* IT Configuration for CAN1 */
    // arg CAN_IT_TME: Transmit mailbox empty Interrupt 
		// 发送邮箱空中断
    // arg CAN_IT_FMP0: FIFO 0 message pending Interrupt 
		// FIFO 0 message pending中断
    // arg CAN_IT_FF0: FIFO 0 full Interrupt
		// FIFO 0 full中断
    // arg CAN_IT_FOV0: FIFO 0 overrun Interrupt
		// FIFO 0 overrun中断
    // arg CAN_IT_FMP1: FIFO 1 message pending Interrupt 
		// FIFO 1 message pending中断
    // arg CAN_IT_FF1: FIFO 1 full Interrupt
		// FIFO 1 full中断
    // arg CAN_IT_FOV1: FIFO 1 overrun Interrupt
		// FIFO 1 溢出中断
    // arg CAN_IT_WKU: Wake-up Interrupt
		// 唤醒中断
    // arg CAN_IT_SLK: Sleep acknowledge Interrupt
    // Sleep acknowledge Interrupt		
    // arg CAN_IT_EWG: Error warning Interrupt
		// 错误警告中断
    // arg CAN_IT_EPV: Error passive Interrupt
		// 被动中断错误
    // arg CAN_IT_BOF: Bus-off Interrupt
    // Bus-off Interrupt		
    // arg CAN_IT_LEC: Last error code Interrupt
		// 最后一个错误代码中断
    // arg CAN_IT_ERR: Error Interrupt
		// 错误中断
    CAN_ITConfig(CAN1, CAN_IT_EPV, ENABLE);
    CAN_ITConfig(CAN1, CAN_IT_EWG, ENABLE);
    CAN_ITConfig(CAN1, CAN_IT_BOF, ENABLE);
    CAN_ITConfig(CAN1, CAN_IT_LEC, ENABLE);
    CAN_ITConfig(CAN1, CAN_IT_ERR, ENABLE);
    
    CAN_OperatingModeRequest(CAN1, CAN_OperatingMode_Normal);
}


/*
********************************************************************************
*  函数名称: CAN1Init
*
*  功能描述: CAN1硬件初始化
*
*  输入参数: 无
*
*  输出参数: 无
*
*  返 回 值: 无
*
********************************************************************************
*/
void CAN2Init(CANX *pcan, CANXBAUD baud)
{
    u8 i;
    GPIO_InitTypeDef        GPIO_InitStructure;
    CAN_FilterInitTypeDef   CAN_FilterInitStructure;
    CAN_InitTypeDef         CAN_InitStructure;
    NVIC_InitTypeDef        NVIC_InitStructure;

        // 输入队列初始化
#if LIST_CAN2_IN_NUM > 4   
    CANQueListInit(&QueCAN2In, CAN2InBuffer, LIST_CAN2_IN_NUM);
    pcan->pQueueIn = &QueCAN2In;
#else
    pcan->pQueueIn = NULL;
#endif
    
    pcan->pCanx = CAN2;

    // 使能外设时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN2, ENABLE);
    
    // 配置管脚到AF功能
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_12;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_13;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // 中断设置
    NVIC_InitStructure.NVIC_IRQChannel = CAN2_TX_IRQn ;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = PREEMPRIO_CAN2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = SUBPRIO_CAN2;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    NVIC_InitStructure.NVIC_IRQChannel = CAN2_RX0_IRQn ;
    NVIC_Init(&NVIC_InitStructure);
    NVIC_InitStructure.NVIC_IRQChannel = CAN2_RX1_IRQn ;
    NVIC_Init(&NVIC_InitStructure);
    NVIC_InitStructure.NVIC_IRQChannel = CAN2_SCE_IRQn ;
    NVIC_Init(&NVIC_InitStructure);

    /* CAN register init */
    CAN_DeInit(CAN2);
    CAN_StructInit(&CAN_InitStructure);

    /* CAN1 cell init */
    CAN_InitStructure.CAN_TTCM = DISABLE;       // Enable or disable the time triggered communication mode.
    CAN_InitStructure.CAN_ABOM = ENABLE;        // Enable or disable the automatic bus-off management.
    CAN_InitStructure.CAN_AWUM = DISABLE;       // Enable or disable the automatic wake-up mode. 
    CAN_InitStructure.CAN_NART = ENABLE;        // Enable or disable the non-automatic retransmission mode.
    CAN_InitStructure.CAN_RFLM = ENABLE;        // Enable or disable the Receive FIFO Locked mode.
    CAN_InitStructure.CAN_TXFP = ENABLE;        // Enable or disable the transmit FIFO priority.
    CAN_InitStructure.CAN_Mode = CAN_Mode_Normal;
    /* BaudRate:
                36M                  1               2M
        --------------------- * ----------- =  ----------------		 //250K
        (SJW+(BS1)+(BS2))         (BRP)         CAN_Prescaler
    */
    CAN_InitStructure.CAN_SJW = CAN_SJW_1tq;
    CAN_InitStructure.CAN_BS1 = CAN_BS1_9tq;
    CAN_InitStructure.CAN_BS2 = CAN_BS2_8tq;
    CAN_InitStructure.CAN_Prescaler = 2000000 / baud;
    CAN_Init(CAN2, &CAN_InitStructure);

    // 接收屏蔽全部禁止
    for(i = 0; i < 28; i++)
    {
        CAN_FilterInitStructure.CAN_FilterNumber = i;
        CAN_FilterInitStructure.CAN_FilterMode   = CAN_FilterMode_IdMask;
        CAN_FilterInitStructure.CAN_FilterScale  = CAN_FilterScale_32bit;
        CAN_FilterInitStructure.CAN_FilterIdHigh         = 0;
        CAN_FilterInitStructure.CAN_FilterIdLow          = 0;
        CAN_FilterInitStructure.CAN_FilterMaskIdHigh     = 0;
        CAN_FilterInitStructure.CAN_FilterMaskIdLow      = 0;
        CAN_FilterInitStructure.CAN_FilterFIFOAssignment = 0;
        CAN_FilterInitStructure.CAN_FilterActivation     = ENABLE;
        CAN_FilterInit(&CAN_FilterInitStructure);
    }

#ifdef CANQUE_USEOS
    // 串口多字节输出保护信号量
    pcan->pWrBufSem = OSSemCreate(1);
#endif
    
    // 输出缓冲初始化
#if LIST_CAN2_OUT_NUM > 4  
    CAN_ITConfig(CAN2, CAN_IT_TME, ENABLE);
    CANQueListInit(&QueCAN2Out, CAN2OutBuffer, LIST_CAN2_OUT_NUM);
    pcan->pQueueOut = &QueCAN2Out; 
#else
    CAN_ITConfig(CAN2, CAN_IT_TME, DISABLE);
    pcan->pQueueOut = NULL;
#endif

    // 输入缓冲初始化
#if LIST_CAN2_IN_NUM > 4  
    CAN_ITConfig(CAN2, CAN_IT_FMP0, ENABLE);
    CAN_ITConfig(CAN2, CAN_IT_FMP1, ENABLE);
    CANQueListInit(&QueCAN2In, CAN2InBuffer, LIST_CAN2_IN_NUM);
    pcan->pQueueIn = &QueCAN2In; 
#else
    CAN_ITConfig(CAN2, CAN_IT_FMP0, DISABLE);
    CAN_ITConfig(CAN2, CAN_IT_FMP1, DISABLE);
    pcan->pQueueIn = NULL;
#endif

    /* IT Configuration for CAN2 */
    // arg CAN_IT_TME: Transmit mailbox empty Interrupt 
    // arg CAN_IT_FMP0: FIFO 0 message pending Interrupt 
    // arg CAN_IT_FF0: FIFO 0 full Interrupt
    // arg CAN_IT_FOV0: FIFO 0 overrun Interrupt
    // arg CAN_IT_FMP1: FIFO 1 message pending Interrupt 
    // arg CAN_IT_FF1: FIFO 1 full Interrupt
    // arg CAN_IT_FOV1: FIFO 1 overrun Interrupt
    // arg CAN_IT_WKU: Wake-up Interrupt
    // arg CAN_IT_SLK: Sleep acknowledge Interrupt  
    // arg CAN_IT_EWG: Error warning Interrupt
    // arg CAN_IT_EPV: Error passive Interrupt
    // arg CAN_IT_BOF: Bus-off Interrupt  
    // arg CAN_IT_LEC: Last error code Interrupt
    // arg CAN_IT_ERR: Error Interrupt
    CAN_ITConfig(CAN2, CAN_IT_EPV, ENABLE);
    CAN_ITConfig(CAN2, CAN_IT_EWG, ENABLE);
    CAN_ITConfig(CAN2, CAN_IT_BOF, ENABLE);
    CAN_ITConfig(CAN2, CAN_IT_LEC, ENABLE);
    CAN_ITConfig(CAN2, CAN_IT_ERR, ENABLE);

    CAN_OperatingModeRequest(CAN2, CAN_OperatingMode_Normal);
}

void CANQUE_TO_CANTXMSG(CanTxMsg* pTxMsg, CANQUEDATA* pCAN)
{
		pTxMsg->RTR = pCAN->RTR;

		if(pCAN->RTR == CAN_RTR_Data)
		{
			pTxMsg->RTR = CAN_RTR_Data;
		}
		else if(pCAN->RTR == CAN_RTR_Remote)
		{
			pTxMsg->RTR = CAN_RTR_Remote;
		}
		if(pCAN->IDE == CAN_Id_Standard)
		{
			pTxMsg->IDE = CAN_Id_Standard;
			pTxMsg->StdId = pCAN->Id;
		}
		else if(pCAN->IDE == CAN_Id_Extended)
		{
			pTxMsg->IDE = CAN_Id_Extended;
			pTxMsg->ExtId = pCAN->Id;
		}	
	
		pTxMsg->DLC = pCAN->Dlc;
		memcpy(pTxMsg->Data, pCAN->Data, pCAN->Dlc);
}

/*
********************************************************************************
*  函数名称: CanTransmit
*
*  功能描述: CAN1发送完成中断
*
*  输入参数: 无
*
*  输出参数: 无
*
*  返 回 值: 0 发送失败  1 发送成功
*
********************************************************************************
*/
u8 CanTransmit(CANX *pcan, CANQUEDATA *pdata)
{
    CanTxMsg  TxMessage;
    u8 res;
    u32 ier;
#ifdef CANQUE_USEOS
#if OS_CRITICAL_METHOD == 3u                     /* Allocate storage for CPU status register           */
    OS_CPU_SR  cpu_sr = 0u;
#endif
#endif
    
    if(pcan == NULL || pdata == NULL)
    {
        return 1;
    }
    
    // 先屏蔽发送中断
    ier = pcan->pCanx->IER;
    CAN_ITConfig(pcan->pCanx, CAN_IT_TME, DISABLE);

    // 发送数据
    if(pcan->pQueueOut == NULL)
    {
        res = CAN_TxStatus_NoMailBox == CAN_Transmit(pcan->pCanx, &TxMessage) ? 0 : 1;
    }
    else
    {
				memset(&TxMessage, 0, sizeof(CanTxMsg));
        CANQUE_TO_CANTXMSG(&TxMessage, pdata);
        if(CAN_TxStatus_NoMailBox == CAN_Transmit(pcan->pCanx, &TxMessage))
        {
#ifdef CANQUE_USEOS
        OS_ENTER_CRITICAL();
#endif
            res = CANQueListPost(pcan->pQueueOut, pdata);

#ifdef CANQUE_USEOS
        OS_EXIT_CRITICAL();
#endif
        }
        else
        {
            res = 0;
        }
    }
    
    CAN_ITConfig(pcan->pCanx, CAN_IT_TME, ier & CAN_IT_TME);
    
    return res;
}

/*
********************************************************************************
*  函数名称: CanReceive
*
*  功能描述: CAN接收数据韩素
*
*  输入参数: 无
*
*  输出参数: 无
*
*  返 回 值: 无
*
********************************************************************************
*/
u8 CanReceive(CANX *pcan, CANQUEDATA *pdata)
{
    u8 res;
    
#ifdef CANQUE_USEOS
#if OS_CRITICAL_METHOD == 3u                     /* Allocate storage for CPU status register           */
    OS_CPU_SR  cpu_sr = 0u;
#endif
#endif

    if(pcan == NULL || pdata == NULL)
    {
        return 0;
    }
    
    // 读取相应串口缓冲区中的数据
    if(pcan->pQueueIn == NULL)
    {
        return 0;
    }
    else
    {
#ifdef CANQUE_USEOS
        OS_ENTER_CRITICAL();
#endif
        res = CANQueListGet(pcan->pQueueIn, pdata);

#ifdef CANQUE_USEOS
        OS_EXIT_CRITICAL();
#endif
        // 返回结果
        return res;
    }
}



/*
********************************************************************************
*  函数名称: CAN1_TX_IRQHandler
*
*  功能描述: CAN1发送完成中断
*
*  输入参数: 无
*
*  输出参数: 无
*
*  返 回 值: 无
*
********************************************************************************
*/
void CAN1_TX_IRQHandler(void)
{
#if LIST_CAN1_OUT_NUM > 4
    CanTxMsg   TxMessage;
    CANQUEDATA pdat;
#endif 

#ifdef CANQUE_USEOS
    // 通知操作系统进入中断
    OSIntEnter();
#endif
        
    if(CAN_GetFlagStatus(CAN1, CAN_FLAG_RQCP0) == SET)
    {
        if(CAN_TransmitStatus(CAN1, 0) == CAN_TxStatus_Failed)
        {
					
            RTU_DEBUG("CAN1 TX: MAILBOX 0 transmit failed\r\n");
        }

        CAN_ClearFlag(CAN1, CAN_FLAG_RQCP0);
    }
    if(CAN_GetFlagStatus(CAN1, CAN_FLAG_RQCP1) == SET)
    {
        if(CAN_TransmitStatus(CAN1, 1) == CAN_TxStatus_Failed)
        {
					 
            RTU_DEBUG("CAN1 TX: MAILBOX 1 transmit failed\r\n");
        }

        CAN_ClearFlag(CAN1, CAN_FLAG_RQCP1);
    }
    if(CAN_GetFlagStatus(CAN1, CAN_FLAG_RQCP2) == SET)
    {
        if(CAN_TransmitStatus(CAN1, 2) == CAN_TxStatus_Failed)
        {
					
            RTU_DEBUG("CAN1 TX: MAILBOX 2 transmit failed\r\n");
        }

        CAN_ClearFlag(CAN1, CAN_FLAG_RQCP2);
    }

    CAN_ClearITPendingBit(CAN1, CAN_IT_TME);

    // 如果有空闲的邮箱并且有数据需要发送则继续发送数据
#if LIST_CAN1_OUT_NUM > 4
    while(CAN1->TSR & CAN_TSR_TME)
    {
        if(0 == CANQueListGet(&QueCAN1Out, &pdat))
        {
            break;
        }
        
				memset(&TxMessage, 0, sizeof(CanTxMsg));
        CANQUE_TO_CANTXMSG(&TxMessage, &pdat);
        CAN_Transmit(CAN1, &TxMessage);
    }
#endif

#ifdef CANQUE_USEOS
    // 退出中断
    OSIntExit();
#endif
}

/*
********************************************************************************
*  函数名称: CAN1_RX0_IRQHandler
*
*  功能描述: CAN1接收中断0
*
*  输入参数: 无
*
*  输出参数: 无
*
*  返 回 值: 无
*
********************************************************************************
*/
void CAN1_RX0_IRQHandler(void)
{
    CANQUEDATA dat;

#ifdef CANQUE_USEOS
    // 通知操作系统进入中断
    OSIntEnter();
#endif

    CAN_ITConfig(CAN1, CAN_IT_FF1, DISABLE);
    CAN_ITConfig(CAN1, CAN_IT_FOV1, DISABLE);

    // 是否发生FIFO Message 0 Pending中断
    while(CAN_MessagePending(CAN1, CAN_FIFO0))
    {
        /* Get the Id */
        dat.Id  = CAN1->sFIFOMailBox[CAN_FIFO0].RIR;
		
        /* Get the DLC */
        dat.Dlc = (u8)0x0F & CAN1->sFIFOMailBox[CAN_FIFO0].RDTR;
        /* Get the data field */
        dat.Data[0] = (u8)0xFF & (CAN1->sFIFOMailBox[CAN_FIFO0].RDLR >> 0);
        dat.Data[1] = (u8)0xFF & (CAN1->sFIFOMailBox[CAN_FIFO0].RDLR >> 8);
        dat.Data[2] = (u8)0xFF & (CAN1->sFIFOMailBox[CAN_FIFO0].RDLR >> 16);
        dat.Data[3] = (u8)0xFF & (CAN1->sFIFOMailBox[CAN_FIFO0].RDLR >> 24);
        dat.Data[4] = (u8)0xFF & (CAN1->sFIFOMailBox[CAN_FIFO0].RDHR >> 0);
        dat.Data[5] = (u8)0xFF & (CAN1->sFIFOMailBox[CAN_FIFO0].RDHR >> 8);
        dat.Data[6] = (u8)0xFF & (CAN1->sFIFOMailBox[CAN_FIFO0].RDHR >> 16);
        dat.Data[7] = (u8)0xFF & (CAN1->sFIFOMailBox[CAN_FIFO0].RDHR >> 24);
        
        /* Release the FIFO */
        CAN_FIFORelease(CAN1, CAN_FIFO0);
        
#if LIST_CAN1_IN_NUM > 4
        CANQueListPost(Can1.pQueueIn, &dat);
#endif
    }

    // 清除FIFO 满和FIFO溢出中断
    CAN_ClearITPendingBit(CAN1, CAN_IT_FF0);              
    CAN_ClearITPendingBit(CAN1, CAN_IT_FOV0);

    CAN_ITConfig(CAN1, CAN_IT_FF1, ENABLE);
    CAN_ITConfig(CAN1, CAN_IT_FOV1, ENABLE);

#ifdef CANQUE_USEOS
    // 退出中断
    OSIntExit();
#endif
}

/*
********************************************************************************
*  函数名称: CAN1_RX1_IRQHandler
*
*  功能描述: CAN1接收中断1
*
*  输入参数: 无
*
*  输出参数: 无
*
*  返 回 值: 无
*
********************************************************************************
*/
void CAN1_RX1_IRQHandler(void)
{
    CANQUEDATA dat;

#ifdef CANQUE_USEOS
    // 通知操作系统进入中断
    OSIntEnter();
#endif

    CAN_ITConfig(CAN1, CAN_IT_FF0, DISABLE);
    CAN_ITConfig(CAN1, CAN_IT_FOV0, DISABLE);

    // 是否发生FIFO Message 0 Pending中断
    while(CAN_MessagePending(CAN1, CAN_FIFO1))
    {
        /* Get the Id */
        dat.Id  = CAN1->sFIFOMailBox[CAN_FIFO1].RIR;
			  
        /* Get the DLC */
        dat.Dlc = (u8)0x0F & CAN1->sFIFOMailBox[CAN_FIFO1].RDTR;
        /* Get the data field */
        dat.Data[0] = (u8)0xFF & (CAN1->sFIFOMailBox[CAN_FIFO1].RDLR >> 0);
        dat.Data[1] = (u8)0xFF & (CAN1->sFIFOMailBox[CAN_FIFO1].RDLR >> 8);
        dat.Data[2] = (u8)0xFF & (CAN1->sFIFOMailBox[CAN_FIFO1].RDLR >> 16);
        dat.Data[3] = (u8)0xFF & (CAN1->sFIFOMailBox[CAN_FIFO1].RDLR >> 24);
        dat.Data[4] = (u8)0xFF & (CAN1->sFIFOMailBox[CAN_FIFO1].RDHR >> 0);
        dat.Data[5] = (u8)0xFF & (CAN1->sFIFOMailBox[CAN_FIFO1].RDHR >> 8);
        dat.Data[6] = (u8)0xFF & (CAN1->sFIFOMailBox[CAN_FIFO1].RDHR >> 16);
        dat.Data[7] = (u8)0xFF & (CAN1->sFIFOMailBox[CAN_FIFO1].RDHR >> 24);
        
        /* Release the FIFO */
        CAN_FIFORelease(CAN1, CAN_FIFO1);
        
#if LIST_CAN1_IN_NUM > 4
        CANQueListPost(Can1.pQueueIn, &dat);
#endif
    }

    // 清除FIFO 满和FIFO溢出中断
    CAN_ClearITPendingBit(CAN1, CAN_IT_FF1);              
    CAN_ClearITPendingBit(CAN1, CAN_IT_FOV1);

    CAN_ITConfig(CAN1, CAN_IT_FF0, ENABLE);
    CAN_ITConfig(CAN1, CAN_IT_FOV0, ENABLE);

#ifdef CANQUE_USEOS
    // 退出中断
    OSIntExit();
#endif
}

/********************************************************************************
*  函数名称: CAN1_SCE_IRQHandler
*
*  功能描述: CAN1状态改变与错误中断处理函数
*
*  输入参数: 无
*
*  输出参数: 无
*
*  返 回 值: 无
*
********************************************************************************
*/
void CAN1_SCE_IRQHandler(void)
{
    u8 errcode;

#ifdef CANQUE_USEOS
    // 通知操作系统进入中断
    OSIntEnter();
#endif
    
    //  Error warning Interrupt
    if(CAN_GetITStatus(CAN1, CAN_IT_EWG) == SET)
    {
        RTU_DEBUG("CAN1 SCE: CAN_IT_EWG\r\n");
        CAN_ClearITPendingBit(CAN1, CAN_IT_EWG);
    }

    // Error passive Interrupt
    if(CAN_GetITStatus(CAN1, CAN_IT_EPV) == SET)
    {
        RTU_DEBUG("CAN1 SCE: CAN_IT_EPV\r\n");
        CAN_ClearITPendingBit(CAN1, CAN_IT_EPV);
    }

    // Bus-off Interrupt
    if(CAN_GetITStatus(CAN1, CAN_IT_BOF) == SET)
    {
        RTU_DEBUG("CAN1 SCE: CAN_IT_BOF\r\n");
        CAN_ClearITPendingBit(CAN1, CAN_IT_BOF);
    }

    // Last error code Interrupt
    if(CAN_GetITStatus(CAN1, CAN_IT_LEC) == SET)
    {
        errcode = CAN_GetLastErrorCode(CAN1);
        errcode >>= 4;
        RTU_DEBUG("CAN1 SCE: CAN_IT_LEC=%d\r\n", errcode);
        CAN_ClearITPendingBit(CAN1, CAN_IT_LEC);
    }

    // 接收中断和发送中断计数值
    RTU_DEBUG("CAN1 SCE: ERROR CONUTER RX=%d, TX=%d\r\n", 
        CAN_GetReceiveErrorCounter(CAN1),
        CAN_GetLSBTransmitErrorCounter(CAN1));

    CAN_ClearITPendingBit(CAN1, CAN_IT_ERR);

#ifdef CANQUE_USEOS
    // 退出中断
    OSIntExit();
#endif
}


/*
********************************************************************************
*  函数名称: CAN2_TX_IRQHandler
*
*  功能描述: CAN2发送完成中断
*
*  输入参数: 无
*
*  输出参数: 无
*
*  返 回 值: 无
*
********************************************************************************
*/
void CAN2_TX_IRQHandler(void)
{
#if LIST_CAN2_OUT_NUM > 4
    CanTxMsg   TxMessage;
    CANQUEDATA pdat;
#endif 

#ifdef CANQUE_USEOS
    // 通知操作系统进入中断
    OSIntEnter();
#endif
        
    if(CAN_GetFlagStatus(CAN2, CAN_FLAG_RQCP0) == SET)
    {
        if(CAN_TransmitStatus(CAN2, 0) == CAN_TxStatus_Failed)
        {
            RTU_DEBUG("CAN2 TX: MAILBOX 0 transmit failed\r\n");
        }

        CAN_ClearFlag(CAN2, CAN_FLAG_RQCP0);
    }
    if(CAN_GetFlagStatus(CAN2, CAN_FLAG_RQCP1) == SET)
    {
        if(CAN_TransmitStatus(CAN2, 1) == CAN_TxStatus_Failed)
        {
            RTU_DEBUG("CAN2 TX: MAILBOX 1 transmit failed\r\n");
        }

        CAN_ClearFlag(CAN2, CAN_FLAG_RQCP1);
    }
    if(CAN_GetFlagStatus(CAN2, CAN_FLAG_RQCP2) == SET)
    {
        if(CAN_TransmitStatus(CAN2, 2) == CAN_TxStatus_Failed)
        {
            RTU_DEBUG("CAN2 TX: MAILBOX 2 transmit failed\r\n");
        }

        CAN_ClearFlag(CAN2, CAN_FLAG_RQCP2);
    }

    CAN_ClearITPendingBit(CAN2, CAN_IT_TME);

    // 如果有空闲的邮箱并且有数据需要发送则继续发送数据
#if LIST_CAN2_OUT_NUM > 4
    while(CAN2->TSR & CAN_TSR_TME)
    {
        if(0 == CANQueListGet(&QueCAN2Out, &pdat))
        {
            break;
        }
        
		memset(&TxMessage, 0, sizeof(CanTxMsg));
        CANQUE_TO_CANTXMSG(&TxMessage, &pdat);
        CAN_Transmit(CAN2, &TxMessage);
    }
#endif

#ifdef CANQUE_USEOS
    // 退出中断
    OSIntExit();
#endif
}

/*
********************************************************************************
*  函数名称: CAN2_RX0_IRQHandler
*
*  功能描述: CAN2接收中断0
*
*  输入参数: 无
*
*  输出参数: 无
*
*  返 回 值: 无
*
********************************************************************************
*/
void CAN2_RX0_IRQHandler(void)
{
    CANQUEDATA dat;

#ifdef CANQUE_USEOS
    // 通知操作系统进入中断
    OSIntEnter();
#endif

    CAN_ITConfig(CAN2, CAN_IT_FF1, DISABLE);
    CAN_ITConfig(CAN2, CAN_IT_FOV1, DISABLE);

    // 是否发生FIFO Message 0 Pending中断
    while(CAN_MessagePending(CAN2, CAN_FIFO0))
    {
        /* Get the Id */
        dat.Id  = CAN2->sFIFOMailBox[CAN_FIFO0].RIR;
        /* Get the DLC */
        dat.Dlc = (u8)0x0F & CAN2->sFIFOMailBox[CAN_FIFO0].RDTR;
        /* Get the data field */
        dat.Data[0] = (u8)0xFF & (CAN2->sFIFOMailBox[CAN_FIFO0].RDLR >> 0);
        dat.Data[1] = (u8)0xFF & (CAN2->sFIFOMailBox[CAN_FIFO0].RDLR >> 8);
        dat.Data[2] = (u8)0xFF & (CAN2->sFIFOMailBox[CAN_FIFO0].RDLR >> 16);
        dat.Data[3] = (u8)0xFF & (CAN2->sFIFOMailBox[CAN_FIFO0].RDLR >> 24);
        dat.Data[4] = (u8)0xFF & (CAN2->sFIFOMailBox[CAN_FIFO0].RDHR >> 0);
        dat.Data[5] = (u8)0xFF & (CAN2->sFIFOMailBox[CAN_FIFO0].RDHR >> 8);
        dat.Data[6] = (u8)0xFF & (CAN2->sFIFOMailBox[CAN_FIFO0].RDHR >> 16);
        dat.Data[7] = (u8)0xFF & (CAN2->sFIFOMailBox[CAN_FIFO0].RDHR >> 24);
        
        /* Release the FIFO */
        CAN_FIFORelease(CAN2, CAN_FIFO0);
        
#if LIST_CAN2_IN_NUM > 4
        CANQueListPost(Can2.pQueueIn, &dat);
#endif
    }

    // 清除FIFO 满和FIFO溢出中断
    CAN_ClearITPendingBit(CAN2, CAN_IT_FF0);              
    CAN_ClearITPendingBit(CAN2, CAN_IT_FOV0);

    CAN_ITConfig(CAN2, CAN_IT_FF1, ENABLE);
    CAN_ITConfig(CAN2, CAN_IT_FOV1, ENABLE);

#ifdef CANQUE_USEOS
    // 退出中断
    OSIntExit();
#endif
}

/*
********************************************************************************
*  函数名称: CAN2_RX1_IRQHandler
*
*  功能描述: CAN2接收中断1
*
*  输入参数: 无
*
*  输出参数: 无
*
*  返 回 值: 无
*
********************************************************************************
*/
void CAN2_RX1_IRQHandler(void)
{
    CANQUEDATA dat;

#ifdef CANQUE_USEOS
    // 通知操作系统进入中断
    OSIntEnter();
#endif

    CAN_ITConfig(CAN2, CAN_IT_FF0, DISABLE);
    CAN_ITConfig(CAN2, CAN_IT_FOV0, DISABLE);

    // 是否发生FIFO Message 0 Pending中断
    while(CAN_MessagePending(CAN2, CAN_FIFO1))
    {
        /* Get the Id */
        dat.Id  = CAN2->sFIFOMailBox[CAN_FIFO1].RIR;
        /* Get the DLC */
        dat.Dlc = (u8)0x0F & CAN2->sFIFOMailBox[CAN_FIFO1].RDTR;
        /* Get the data field */
        dat.Data[0] = (u8)0xFF & (CAN2->sFIFOMailBox[CAN_FIFO1].RDLR >> 0);
        dat.Data[1] = (u8)0xFF & (CAN2->sFIFOMailBox[CAN_FIFO1].RDLR >> 8);
        dat.Data[2] = (u8)0xFF & (CAN2->sFIFOMailBox[CAN_FIFO1].RDLR >> 16);
        dat.Data[3] = (u8)0xFF & (CAN2->sFIFOMailBox[CAN_FIFO1].RDLR >> 24);
        dat.Data[4] = (u8)0xFF & (CAN2->sFIFOMailBox[CAN_FIFO1].RDHR >> 0);
        dat.Data[5] = (u8)0xFF & (CAN2->sFIFOMailBox[CAN_FIFO1].RDHR >> 8);
        dat.Data[6] = (u8)0xFF & (CAN2->sFIFOMailBox[CAN_FIFO1].RDHR >> 16);
        dat.Data[7] = (u8)0xFF & (CAN2->sFIFOMailBox[CAN_FIFO1].RDHR >> 24);
        
        /* Release the FIFO */
        CAN_FIFORelease(CAN2, CAN_FIFO1);
        
#if LIST_CAN2_IN_NUM > 4
        CANQueListPost(Can2.pQueueIn, &dat);
#endif
    }

    // 清除FIFO 满和FIFO溢出中断
    CAN_ClearITPendingBit(CAN2, CAN_IT_FF1);              
    CAN_ClearITPendingBit(CAN2, CAN_IT_FOV1);

    CAN_ITConfig(CAN2, CAN_IT_FF0, ENABLE);
    CAN_ITConfig(CAN2, CAN_IT_FOV0, ENABLE);

#ifdef CANQUE_USEOS
    // 退出中断
    OSIntExit();
#endif
}

/********************************************************************************
*  函数名称: CAN2_SCE_IRQHandler
*
*  功能描述: CAN2状态改变与错误中断处理函数
*
*  输入参数: 无
*
*  输出参数: 无
*
*  返 回 值: 无
*
********************************************************************************
*/
void CAN2_SCE_IRQHandler(void)
{
    u8 errcode;

#ifdef CANQUE_USEOS
    // 通知操作系统进入中断
    OSIntEnter();
#endif
    
    //  Error warning Interrupt
    if(CAN_GetITStatus(CAN2, CAN_IT_EWG) == SET)
    {
        RTU_DEBUG("CAN2 SCE: CAN_IT_EWG\r\n");
        CAN_ClearITPendingBit(CAN2, CAN_IT_EWG);
    }

    // Error passive Interrupt
    if(CAN_GetITStatus(CAN2, CAN_IT_EPV) == SET)
    {
        RTU_DEBUG("CAN2 SCE: CAN_IT_EPV\r\n");
        CAN_ClearITPendingBit(CAN2, CAN_IT_EPV);
    }

    // Bus-off Interrupt
    if(CAN_GetITStatus(CAN2, CAN_IT_BOF) == SET)
    {
        RTU_DEBUG("CAN2 SCE: CAN_IT_BOF\r\n");
        CAN_ClearITPendingBit(CAN2, CAN_IT_BOF);
    }

    // Last error code Interrupt
    if(CAN_GetITStatus(CAN2, CAN_IT_LEC) == SET)
    {
        errcode = CAN_GetLastErrorCode(CAN2);
        errcode >>= 4;
        RTU_DEBUG("CAN2 SCE: CAN_IT_LEC=%d\r\n", errcode);
        CAN_ClearITPendingBit(CAN2, CAN_IT_LEC);
    }

    // 接收中断和发送中断计数值
    //RTU_DEBUG("CAN2 SCE: ERROR CONUTER RX=%d, TX=%d\r\n", 
    //    CAN_GetReceiveErrorCounter(CAN2),
    //    CAN_GetLSBTransmitErrorCounter(CAN2));

    CAN_ClearITPendingBit(CAN2, CAN_IT_ERR);

#ifdef CANQUE_USEOS
    // 退出中断
    OSIntExit();
#endif
}




