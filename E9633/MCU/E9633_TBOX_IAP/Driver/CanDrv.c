/*
********************************************************************************
*  Copyright (C), 2009-2012, ����˼׳�����з�����
*
*  ��Ŀ���ƣ�VS331
*  
*  �ļ�����: CanDrv.c
*
*  �ļ�������CANӲ������Դ����
*             
*             
*  �� �� ��: �ʺ���
*
*  �������ڣ�2014-04-04 
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

#define CANDRV_GLOBALS

#include "CanDrv.h"
#include "Shell.h"
#include <string.h>

// CAN������ж���
#if  LIST_CAN1_IN_NUM > 4
static CANQUEUELIST  QueCAN1In ;      /* CAN1������� */
static CANQUEDATA    CAN1InBuffer[LIST_CAN1_IN_NUM];
#endif

#if  LIST_CAN2_IN_NUM > 4
static CANQUEUELIST  QueCAN2In ;      /* CAN2������� */
static CANQUEDATA    CAN2InBuffer[LIST_CAN2_IN_NUM];
#endif

// CAN������ж���
#if LIST_CAN1_OUT_NUM > 4
static  CANQUEUELIST  QueCAN1Out;      /* CAN1������� */
static  CANQUEDATA    CAN1OutBuffer[LIST_CAN1_OUT_NUM];
#endif

#if LIST_CAN2_OUT_NUM > 4
static  CANQUEUELIST  QueCAN2Out;      /* CAN2������� */
static  CANQUEDATA    CAN2OutBuffer[LIST_CAN2_OUT_NUM];
#endif

/*
********************************************************************************
*  ��������: CANQueListInit
*
*  ��������: ���г�ʼ��
*
*  �������: plist  ����ָ��
*            pbuf   �������ݻ�����ָ��
*            len    ���е����ݻ���������
*
*  �������: ��
*
*  �� �� ֵ: ��
*
********************************************************************************
*/
void CANQueListInit(CANQUEUELIST *plist, CANQUEDATA * pbuf, u16 len)
{
    // �����ж�
    if(plist == NULL)
    {
        return;
    }

    // ��ʼ������
    plist->head  = 0;
    plist->trail = 0;
    plist->pdata = pbuf;

    // ����ָ��Ϊ��ʱ�������г�������Ϊ0
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
*  ��������: CANQueListGet
*
*  ��������: ��CAN��������л�ȡһ������
*
*  �������: pmsg_queue  CAN����ָ��
*            rdata       ��ȡ����ָ��
*
*  �������: ��
*
*  �� �� ֵ: 0 - �����������ݣ���ȡʧ��  1 - ��ȡ���ݳɹ�
*
********************************************************************************
*/
u8  CANQueListGet(CANQUEUELIST *pmsg_queue, CANQUEDATA *rdata)
{
    u16 i;
    CANQUEDATA * pcur;
    
    // �������
    if(pmsg_queue == NULL || pmsg_queue->pdata == NULL || pmsg_queue->lenmax == 0)
    {
        return 0;
    }

    // ����Ϊ��
    if(pmsg_queue->head == pmsg_queue->trail)
    {
        return 0;
    }

    // ȡ������
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
*  ��������: CANQueListPost
*
*  ��������: ��CAN�������������һ������
*
*  �������: pmsg_queue  CAN����ָ��
*            dat         Ҫд�������
*
*  �������: ��
*
*  �� �� ֵ: 1 - д��ʧ��  0 - д��ɹ�
*
********************************************************************************
*/
u8  CANQueListPost(CANQUEUELIST *pmsg_queue, CANQUEDATA *dat)
{
    u16 tmp, i;
    CANQUEDATA * pcur;
    
    // �������
    if(pmsg_queue == NULL || pmsg_queue->pdata == NULL || pmsg_queue->lenmax == 0)
    {
        return 1;
    }

    // �����Ѿ���
    tmp = (pmsg_queue->trail + 1) % pmsg_queue->lenmax;
    if(tmp == pmsg_queue->head)
    {
        return 1;
    }

    // д������
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
*  ��������: CAN1Init
*
*  ��������: CAN1Ӳ����ʼ��
*
*  �������: ��
*
*  �������: ��
*
*  �� �� ֵ: ��
*
********************************************************************************
*/
void CAN1Init(CANX *pcan, CANXBAUD baud)
{
    u8 i;
    GPIO_InitTypeDef        GPIO_InitStructure;
    CAN_FilterInitTypeDef   CAN_FilterInitStructure;
    CAN_InitTypeDef         CAN_InitStructure;
    NVIC_InitTypeDef        NVIC_InitStructure;

        // ������г�ʼ��
#if LIST_CAN1_IN_NUM > 4   
    CANQueListInit(&QueCAN1In, CAN1InBuffer, LIST_CAN1_IN_NUM);
    pcan->pQueueIn = &QueCAN1In;
#else
    pcan->pQueueIn = NULL;
#endif
    
    pcan->pCanx = CAN1;

    // ʹ������ʱ��
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);
    
    // ���ùܽŵ�AF����
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_8;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // ����ӳ��CAN1���ţ�RX->PB8 TX->PB9
    GPIO_PinRemapConfig(GPIO_Remap1_CAN1, ENABLE);

    // �ж�����
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

    // ��������ȫ����ֹ
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
    // ���ڶ��ֽ���������ź���
    pcan->pWrBufSem = OSSemCreate(1);
#endif
    
    // ��������ʼ��
#if LIST_CAN1_OUT_NUM > 4  
    CAN_ITConfig(CAN1, CAN_IT_TME, ENABLE);
    CANQueListInit(&QueCAN1Out, CAN1OutBuffer, LIST_CAN1_OUT_NUM);
    pcan->pQueueOut = &QueCAN1Out; 
#else
    CAN_ITConfig(CAN1, CAN_IT_TME, DISABLE);
    pcan->pQueueOut = NULL;
#endif

    // ���뻺���ʼ��
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
		// ����������ж�
    // arg CAN_IT_FMP0: FIFO 0 message pending Interrupt 
		// FIFO 0 message pending�ж�
    // arg CAN_IT_FF0: FIFO 0 full Interrupt
		// FIFO 0 full�ж�
    // arg CAN_IT_FOV0: FIFO 0 overrun Interrupt
		// FIFO 0 overrun�ж�
    // arg CAN_IT_FMP1: FIFO 1 message pending Interrupt 
		// FIFO 1 message pending�ж�
    // arg CAN_IT_FF1: FIFO 1 full Interrupt
		// FIFO 1 full�ж�
    // arg CAN_IT_FOV1: FIFO 1 overrun Interrupt
		// FIFO 1 ����ж�
    // arg CAN_IT_WKU: Wake-up Interrupt
		// �����ж�
    // arg CAN_IT_SLK: Sleep acknowledge Interrupt
    // Sleep acknowledge Interrupt		
    // arg CAN_IT_EWG: Error warning Interrupt
		// ���󾯸��ж�
    // arg CAN_IT_EPV: Error passive Interrupt
		// �����жϴ���
    // arg CAN_IT_BOF: Bus-off Interrupt
    // Bus-off Interrupt		
    // arg CAN_IT_LEC: Last error code Interrupt
		// ���һ����������ж�
    // arg CAN_IT_ERR: Error Interrupt
		// �����ж�
    CAN_ITConfig(CAN1, CAN_IT_EPV, ENABLE);
    CAN_ITConfig(CAN1, CAN_IT_EWG, ENABLE);
    CAN_ITConfig(CAN1, CAN_IT_BOF, ENABLE);
    CAN_ITConfig(CAN1, CAN_IT_LEC, ENABLE);
    CAN_ITConfig(CAN1, CAN_IT_ERR, ENABLE);

    CAN_OperatingModeRequest(CAN1, CAN_OperatingMode_Normal);
}


/*
********************************************************************************
*  ��������: CAN1Init
*
*  ��������: CAN1Ӳ����ʼ��
*
*  �������: ��
*
*  �������: ��
*
*  �� �� ֵ: ��
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

        // ������г�ʼ��
#if LIST_CAN2_IN_NUM > 4   
    CANQueListInit(&QueCAN2In, CAN2InBuffer, LIST_CAN2_IN_NUM);
    pcan->pQueueIn = &QueCAN2In;
#else
    pcan->pQueueIn = NULL;
#endif
    
    pcan->pCanx = CAN2;

    // ʹ������ʱ��
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN2, ENABLE);
    
    // ���ùܽŵ�AF����
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_12;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_13;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // �ж�����
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

    // ��������ȫ����ֹ
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
    // ���ڶ��ֽ���������ź���
    pcan->pWrBufSem = OSSemCreate(1);
#endif
    
    // ��������ʼ��
#if LIST_CAN2_OUT_NUM > 4  
    CAN_ITConfig(CAN2, CAN_IT_TME, ENABLE);
    CANQueListInit(&QueCAN2Out, CAN2OutBuffer, LIST_CAN2_OUT_NUM);
    pcan->pQueueOut = &QueCAN2Out; 
#else
    CAN_ITConfig(CAN2, CAN_IT_TME, DISABLE);
    pcan->pQueueOut = NULL;
#endif

    // ���뻺���ʼ��
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
	int rtr;
	int ext;

	rtr = pCAN->Id & 0x02;
	if (rtr != 0)
	{
		pTxMsg->RTR = CAN_RTR_Remote;
	}
	else
	{
		pTxMsg->RTR = CAN_RTR_Data;
	}

	ext = pCAN->Id & 0x04;
	if (ext != 0)
	{
		pTxMsg->IDE = CAN_Id_Extended;
		pTxMsg->ExtId = pCAN->Id >> 3;
	}
	else
	{
		pTxMsg->IDE = CAN_Id_Standard;
		pTxMsg->StdId = pCAN->Id >> 21;
	}

	pTxMsg->DLC = pCAN->Dlc;
	memcpy(pTxMsg->Data, pCAN->Data, pCAN->Dlc);
}

/*
********************************************************************************
*  ��������: CanTransmit
*
*  ��������: CAN1��������ж�
*
*  �������: ��
*
*  �������: ��
*
*  �� �� ֵ: 0 ����ʧ��  1 ���ͳɹ�
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
    
    // �����η����ж�
    ier = pcan->pCanx->IER;
    CAN_ITConfig(pcan->pCanx, CAN_IT_TME, DISABLE);

    // ��������
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
*  ��������: CanReceive
*
*  ��������: CAN�������ݺ���
*
*  �������: ��
*
*  �������: ��
*
*  �� �� ֵ: ��
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
    
    // ��ȡ��Ӧ���ڻ������е�����
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
        // ���ؽ��
        return res;
    }
}

/*
********************************************************************************
*  ��������: CAN1_TX_IRQHandler
*
*  ��������: CAN1��������ж�
*
*  �������: ��
*
*  �������: ��
*
*  �� �� ֵ: ��
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
    // ֪ͨ����ϵͳ�����ж�
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

    // ����п��е����䲢����������Ҫ�����������������
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
    // �˳��ж�
    OSIntExit();
#endif
}

/*
********************************************************************************
*  ��������: CAN1_RX0_IRQHandler
*
*  ��������: CAN1�����ж�0
*
*  �������: ��
*
*  �������: ��
*
*  �� �� ֵ: ��
*
********************************************************************************
*/
void CAN1_RX0_IRQHandler(void)
{
    CANQUEDATA dat;

#ifdef CANQUE_USEOS
    // ֪ͨ����ϵͳ�����ж�
    OSIntEnter();
#endif

    CAN_ITConfig(CAN1, CAN_IT_FF1, DISABLE);
    CAN_ITConfig(CAN1, CAN_IT_FOV1, DISABLE);

    // �Ƿ���FIFO Message 0 Pending�ж�
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

    // ���FIFO ����FIFO����ж�
    CAN_ClearITPendingBit(CAN1, CAN_IT_FF0);              
    CAN_ClearITPendingBit(CAN1, CAN_IT_FOV0);

    CAN_ITConfig(CAN1, CAN_IT_FF1, ENABLE);
    CAN_ITConfig(CAN1, CAN_IT_FOV1, ENABLE);

#ifdef CANQUE_USEOS
    // �˳��ж�
    OSIntExit();
#endif
}

/*
********************************************************************************
*  ��������: CAN1_RX1_IRQHandler
*
*  ��������: CAN1�����ж�1
*
*  �������: ��
*
*  �������: ��
*
*  �� �� ֵ: ��
*
********************************************************************************
*/
void CAN1_RX1_IRQHandler(void)
{
    CANQUEDATA dat;

#ifdef CANQUE_USEOS
    // ֪ͨ����ϵͳ�����ж�
    OSIntEnter();
#endif

    CAN_ITConfig(CAN1, CAN_IT_FF0, DISABLE);
    CAN_ITConfig(CAN1, CAN_IT_FOV0, DISABLE);

    // �Ƿ���FIFO Message 0 Pending�ж�
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

    // ���FIFO ����FIFO����ж�
    CAN_ClearITPendingBit(CAN1, CAN_IT_FF1);              
    CAN_ClearITPendingBit(CAN1, CAN_IT_FOV1);

    CAN_ITConfig(CAN1, CAN_IT_FF0, ENABLE);
    CAN_ITConfig(CAN1, CAN_IT_FOV0, ENABLE);

#ifdef CANQUE_USEOS
    // �˳��ж�
    OSIntExit();
#endif
}

/********************************************************************************
*  ��������: CAN1_SCE_IRQHandler
*
*  ��������: CAN1״̬�ı�������жϴ�������
*
*  �������: ��
*
*  �������: ��
*
*  �� �� ֵ: ��
*
********************************************************************************
*/
void CAN1_SCE_IRQHandler(void)
{
    u8 errcode;

#ifdef CANQUE_USEOS
    // ֪ͨ����ϵͳ�����ж�
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

    // �����жϺͷ����жϼ���ֵ
    //RTU_DEBUG("CAN1 SCE: ERROR CONUTER RX=%d, TX=%d\r\n", 
    //    CAN_GetReceiveErrorCounter(CAN1),
    //    CAN_GetLSBTransmitErrorCounter(CAN1));

    CAN_ClearITPendingBit(CAN1, CAN_IT_ERR);

#ifdef CANQUE_USEOS
    // �˳��ж�
    OSIntExit();
#endif
}


/*
********************************************************************************
*  ��������: CAN2_TX_IRQHandler
*
*  ��������: CAN2��������ж�
*
*  �������: ��
*
*  �������: ��
*
*  �� �� ֵ: ��
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
    // ֪ͨ����ϵͳ�����ж�
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

    // ����п��е����䲢����������Ҫ�����������������
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
    // �˳��ж�
    OSIntExit();
#endif
}

/*
********************************************************************************
*  ��������: CAN2_RX0_IRQHandler
*
*  ��������: CAN2�����ж�0
*
*  �������: ��
*
*  �������: ��
*
*  �� �� ֵ: ��
*
********************************************************************************
*/
void CAN2_RX0_IRQHandler(void)
{
    CANQUEDATA dat;

#ifdef CANQUE_USEOS
    // ֪ͨ����ϵͳ�����ж�
    OSIntEnter();
#endif

    CAN_ITConfig(CAN2, CAN_IT_FF1, DISABLE);
    CAN_ITConfig(CAN2, CAN_IT_FOV1, DISABLE);

    // �Ƿ���FIFO Message 0 Pending�ж�
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

    // ���FIFO ����FIFO����ж�
    CAN_ClearITPendingBit(CAN2, CAN_IT_FF0);              
    CAN_ClearITPendingBit(CAN2, CAN_IT_FOV0);

    CAN_ITConfig(CAN2, CAN_IT_FF1, ENABLE);
    CAN_ITConfig(CAN2, CAN_IT_FOV1, ENABLE);

#ifdef CANQUE_USEOS
    // �˳��ж�
    OSIntExit();
#endif
}

/*
********************************************************************************
*  ��������: CAN2_RX1_IRQHandler
*
*  ��������: CAN2�����ж�1
*
*  �������: ��
*
*  �������: ��
*
*  �� �� ֵ: ��
*
********************************************************************************
*/
void CAN2_RX1_IRQHandler(void)
{
    CANQUEDATA dat;

#ifdef CANQUE_USEOS
    // ֪ͨ����ϵͳ�����ж�
    OSIntEnter();
#endif

    CAN_ITConfig(CAN2, CAN_IT_FF0, DISABLE);
    CAN_ITConfig(CAN2, CAN_IT_FOV0, DISABLE);

    // �Ƿ���FIFO Message 0 Pending�ж�
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

    // ���FIFO ����FIFO����ж�
    CAN_ClearITPendingBit(CAN2, CAN_IT_FF1);              
    CAN_ClearITPendingBit(CAN2, CAN_IT_FOV1);

    CAN_ITConfig(CAN2, CAN_IT_FF0, ENABLE);
    CAN_ITConfig(CAN2, CAN_IT_FOV0, ENABLE);

#ifdef CANQUE_USEOS
    // �˳��ж�
    OSIntExit();
#endif
}

/********************************************************************************
*  ��������: CAN2_SCE_IRQHandler
*
*  ��������: CAN2״̬�ı�������жϴ�������
*
*  �������: ��
*
*  �������: ��
*
*  �� �� ֵ: ��
*
********************************************************************************
*/
void CAN2_SCE_IRQHandler(void)
{
    u8 errcode;

#ifdef CANQUE_USEOS
    // ֪ͨ����ϵͳ�����ж�
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

    // �����жϺͷ����жϼ���ֵ
    //RTU_DEBUG("CAN2 SCE: ERROR CONUTER RX=%d, TX=%d\r\n", 
    //    CAN_GetReceiveErrorCounter(CAN2),
    //    CAN_GetLSBTransmitErrorCounter(CAN2));

    CAN_ClearITPendingBit(CAN2, CAN_IT_ERR);

#ifdef CANQUE_USEOS
    // �˳��ж�
    OSIntExit();
#endif
}



