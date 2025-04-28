/*
 * Copyright 2021-2023 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "semphr.h"
#include "event_groups.h"


#include "board.h"
#include "fsl_usart.h"

#include "fsl_device_registers.h"
#include "fsl_debug_console.h"

#include <stdbool.h>

#include "serial.h"
#include "SerialLink.h"


/*******************************************************************************
 * Definitions
 ******************************************************************************/
#if 1//from app_preinclude.h
#define SERIAL_READ_TASK_PRIORITY               (tskIDLE_PRIORITY + 4)
#define SERIAL_READ_TASK_STACK_SIZE             512
#define SERIAL_CALLBACK_TASK_PRIORITY           (tskIDLE_PRIORITY + 3)
#define SERIAL_CALLBACK_TASK_STACK_SIZE         512
#endif

#define SL_START_CHAR	                  0x01
#define SL_ESC_CHAR		                  0x02
#define SL_END_CHAR		                  0x03 

#define SL_MAX_MESSAGE_LENGTH             256
#define SL_MAX_MESSAGE_QUEUES             5
#define SL_MAX_CALLBACK_QUEUES            5

/** States for receive state machine */
typedef enum
{
    E_STATE_RX_WAIT_START,
    E_STATE_RX_WAIT_TYPEMSB,
    E_STATE_RX_WAIT_TYPELSB,
    E_STATE_RX_WAIT_LENMSB,
    E_STATE_RX_WAIT_LENLSB,
    E_STATE_RX_WAIT_CRC,
    E_STATE_RX_WAIT_DATA,
} teSL_RxState;


/** Forward definition of callback function entry */
struct _tsSL_CallbackEntry;


/** Linked list structure for a callback function entry */
typedef struct _tsSL_CallbackEntry
{
    uint16_t                u16Type;        /**< Message type for this callback */
    tprSL_MessageCallback   prCallback;     /**< User supplied callback function for this message type */
    void                    *pvUser;        /**< User supplied data for the callback function */
    struct _tsSL_CallbackEntry *psNext;     /**< Pointer to next in linked list */
} tsSL_CallbackEntry;


/** Structure used to contain a message */
typedef struct
{
    uint16_t u16Type;
    uint16_t u16Length;
    uint8_t  au8Message[SL_MAX_MESSAGE_LENGTH];
} tsSL_Message;


/** Structure of data for the serial link */
typedef struct
{
	SemaphoreHandle_t        txMessageMutex;
    
    struct
    {
        SemaphoreHandle_t       mutex;
        tsSL_CallbackEntry      *psListHead;
    } sCallbacks;
    
    QueueHandle_t sCallbackQueue;	
    
    // Array of listeners for messages
    // eSL_MessageWait uses this array to wait on incoming messages.
    struct 
    {
        uint16_t u16Type;
        uint16_t u16Length;
        uint8_t *pu8Message;

        SemaphoreHandle_t    mutex;
        EventGroupHandle_t   eventGroup;

    } asReaderMessageQueue[SL_MAX_MESSAGE_QUEUES];

} tsSerialLink;


/** Structure allocated and passed to callback handler thread */
typedef struct
{
    tsSL_Message            sMessage;       /** The received message */ 
    tprSL_MessageCallback   prCallback;     /**< User supplied callback function for this message type */
    void *                  pvUser;         /**< User supplied data for the callback function */
} tsCallbackTaskData;


/******************************************************************************
 * Prototypes 
 ******************************************************************************/

static uint8_t u8SL_CalculateCRC(uint16_t u16Type, uint16_t u16Length, uint8_t *pu8Data);
uint8_t u8SL_LoadByteToTxBuffer(bool bSpecialCharacter, uint8_t u8Data, uint8_t *buffer, uint8_t index);

static bool bSL_RxByte(uint8_t *pu8Data);
static teSL_Status eSL_WriteMessage(uint16_t u16Type, uint16_t u16Length, uint8_t *pu8Data);
static teSL_Status eSL_ReadMessage(uint16_t *pu16Type, uint16_t *pu16Length, uint16_t u16MaxLength, uint8_t *pu8Message);

static void serialReadTask(void * pvParameters);
static void serialCallbackTask(void * pvParameters);


/*******************************************************************************
 * Variables
 ******************************************************************************/

static tsSerialLink sSerialLink;


/*******************************************************************************
 * Code
 ******************************************************************************/

teSL_Status eSL_Init(void)
{
	eSerial_Init();
       
	/* Create send message mutux */
	sSerialLink.txMessageMutex = xSemaphoreCreateMutex();
   
    /* Initialise message callbacks */
    sSerialLink.sCallbacks.mutex = xSemaphoreCreateMutex();
    sSerialLink.sCallbacks.psListHead = NULL;
    
    /* Initialise message wait queue */
    for (uint8_t i = 0; i < SL_MAX_MESSAGE_QUEUES; i++)
    {
        sSerialLink.asReaderMessageQueue[i].u16Type = 0;
        sSerialLink.asReaderMessageQueue[i].eventGroup = xEventGroupCreate();
    }
        
    /* Initialise callback queue */
    sSerialLink.sCallbackQueue = xQueueCreate(SL_MAX_CALLBACK_QUEUES, sizeof(tsCallbackTaskData *));

    /* Start the serial reader task */
    if(pdPASS != xTaskCreate((void *)serialReadTask,
                            "serialReadTask",
                            SERIAL_READ_TASK_STACK_SIZE,
                            (void *)&sSerialLink,
                            SERIAL_READ_TASK_PRIORITY,
                            NULL))
    {
		PRINTF("\n Create serialReadTask failed\n");
		return -1;
    }

    /* Start the callback handler task */
    if(pdPASS != xTaskCreate((void *)serialCallbackTask,
                            "serialCallbackTask",
                            SERIAL_CALLBACK_TASK_STACK_SIZE,
                            (void *)&sSerialLink,
                            SERIAL_CALLBACK_TASK_PRIORITY,
                            NULL))
    {
		PRINTF("\n Create serialCallbackTask failed\n");
		return -1;
    }

    return E_SL_OK;
}



teSL_Status eSL_SendMessage(uint16_t u16Type, uint16_t u16Length, void *pvMessage, uint8_t *pu8SequenceNo)
{
    teSL_Status eStatus;

    /* Make sure there is only one thread sending messages to the node at a time. */
    xSemaphoreTake(sSerialLink.txMessageMutex, portMAX_DELAY);

    eStatus = eSL_WriteMessage(u16Type, u16Length, (uint8_t *)pvMessage);

    xSemaphoreGive(sSerialLink.txMessageMutex);

    if (eStatus == E_SL_OK)
    {
        /* Command sent successfully */
        uint16_t    u16Length;
        tsSL_Msg_Status sStatus;
        tsSL_Msg_Status *psStatus = &sStatus;
        sStatus.u16MessageType = u16Type;

        /* Expect a status response within 500ms */
        eStatus = eSL_MessageWait(E_SL_MSG_STATUS, 500, &u16Length, (void**)&psStatus);
		
		if (eStatus)
			PRINTF("\n !!! eSL_MessageWait = %d",eStatus);
		
        if (eStatus == E_SL_OK)
        {           
            eStatus = (teSL_Status)(psStatus->eStatus);
            if (eStatus == E_SL_OK)
            {
                if (pu8SequenceNo)
                {
                    *pu8SequenceNo = psStatus->u8SequenceNo;
                }
            }
            vPortFree(psStatus);
        } else {
        //    LOG(ZBSERIAL, ERR, "eSL_SendMessage() failed, reason = %d\r\n", eStatus);
        }
    }

    return eStatus;
}



teSL_Status eSL_SendMessageNoWait(uint16_t u16Type, uint16_t u16Length, void *pvMessage, uint8_t *pu8SequenceNo)
{
    teSL_Status eStatus;
    
    /* Make sure there is only one task sending messages to the node at a time. */
    xSemaphoreTake(sSerialLink.txMessageMutex, portMAX_DELAY);
    
    eStatus = eSL_WriteMessage(u16Type, u16Length, (uint8_t *)pvMessage);
    
    xSemaphoreGive(sSerialLink.txMessageMutex);
    
    return eStatus;
}



teSL_Status eSL_MessageWait(uint16_t u16Type, uint32_t u32WaitTimeout, uint16_t *pu16Length, void **ppvMessage)
{
    int i;
    tsSerialLink *psSerialLink = &sSerialLink;
    EventBits_t event_bits = 0;
    
    for (i = 0; i < SL_MAX_MESSAGE_QUEUES; i++)
    {
        taskENTER_CRITICAL();
        
        if (psSerialLink->asReaderMessageQueue[i].u16Type == 0)
        {                  
            psSerialLink->asReaderMessageQueue[i].u16Type = u16Type;
                        
            if (u16Type == E_SL_MSG_STATUS)
            {
                psSerialLink->asReaderMessageQueue[i].pu8Message = *ppvMessage;        
            }
            
            taskEXIT_CRITICAL();

            event_bits = xEventGroupWaitBits(psSerialLink->asReaderMessageQueue[i].eventGroup,    
                                             0x01,        
                                             pdTRUE,
                                             pdFALSE,
                                             pdMS_TO_TICKS(u32WaitTimeout));

            taskENTER_CRITICAL();
            psSerialLink->asReaderMessageQueue[i].u16Type = 0;
            taskEXIT_CRITICAL();
            
            if (event_bits & 0x01)
            {
                if (pu16Length != NULL ) {
                    *pu16Length = psSerialLink->asReaderMessageQueue[i].u16Length;
                }
                
                if (ppvMessage != NULL) {
                    *ppvMessage = psSerialLink->asReaderMessageQueue[i].pu8Message;
                }
                
                return E_SL_OK;      
            }
            else
            {
            //    LOG(ZBSERIAL, ERR, "Waiting Msg (0x%x) timed out\r\n", u16Type);
                
                return E_SL_NOMESSAGE;
            }
        }
        else
        {
            taskEXIT_CRITICAL();
        }
    }
    
 //   LOG(ZBSERIAL, ERR, "No free queue slots in Msg Queue\r\n");
    
    return E_SL_ERROR;
}



static teSL_Status eSL_ReadMessage(uint16_t *pu16Type, uint16_t *pu16Length, uint16_t u16MaxLength, uint8_t *pu8Message)
{

    static teSL_RxState eRxState = E_STATE_RX_WAIT_START;
    static uint8_t u8CRC;
    static uint16_t u16Bytes;
    static bool bInEsc = false;
    uint8_t u8Data;

    while(bSL_RxByte(&u8Data))
    {
        switch (u8Data)
        {

            case SL_START_CHAR:
                // Reset state machine
                u16Bytes = 0;
                bInEsc = false;
                eRxState = E_STATE_RX_WAIT_TYPEMSB;
                break;

            case SL_ESC_CHAR:
                // Escape next character
                bInEsc = true;
                break;

            case SL_END_CHAR:
                // End message
                eRxState = E_STATE_RX_WAIT_START;
                if (*pu16Length < u16MaxLength)
                {
                    if (u8CRC == u8SL_CalculateCRC(*pu16Type, *pu16Length, pu8Message))
                    {
                        /* CRC matches - valid packet */
                        return E_SL_OK;
                    }
                }
    //			LOG(ZBSERIAL, WARN, "CRC bad\r\n");
                break;

            default:
            {
                if (bInEsc)
                {
                    /* Unescape the character */
                    u8Data ^= 0x10;
                    bInEsc = false;
                }

                switch (eRxState)
                {

                    case E_STATE_RX_WAIT_START:
                        break;

                    case E_STATE_RX_WAIT_TYPEMSB:
                        *pu16Type = (uint16_t)u8Data << 8;
                        eRxState++;
                        break;

                    case E_STATE_RX_WAIT_TYPELSB:
                        *pu16Type += (uint16_t)u8Data;
                        eRxState++;
                        break;

                    case E_STATE_RX_WAIT_LENMSB:
                        *pu16Length = (uint16_t)u8Data << 8;
                        eRxState++;
                        break;

                    case E_STATE_RX_WAIT_LENLSB:
                        *pu16Length += (uint16_t)u8Data;
                        if (*pu16Length > u16MaxLength)
                        {
                            eRxState = E_STATE_RX_WAIT_START;
                        }
                        else
                        {
                            eRxState++;
                        }
                        break;

                    case E_STATE_RX_WAIT_CRC:
                        u8CRC = u8Data;
                        eRxState++;
                        break;

                    case E_STATE_RX_WAIT_DATA:
                        if (u16Bytes < *pu16Length)
                        {
                            pu8Message[u16Bytes++] = u8Data;
                        }
                        break;
                        
                    default:
                        break;
                }
                
            }
                break;
        }    
    }
    return E_SL_NOMESSAGE;
}



teSL_Status eSL_AddListener(uint16_t u16Type, tprSL_MessageCallback prCallback, void *pvUser)
{
    tsSL_CallbackEntry *psCurrentEntry;
    tsSL_CallbackEntry *psNewEntry;
        
    psNewEntry = pvPortMalloc(sizeof(tsSL_CallbackEntry));
    if (psNewEntry == NULL)
    {
        return E_SL_ERROR_NOMEM;
    }
    
    psNewEntry->u16Type     = u16Type;
    psNewEntry->prCallback  = prCallback;
    psNewEntry->pvUser      = pvUser;
    psNewEntry->psNext      = NULL;
    
    xSemaphoreTake(sSerialLink.sCallbacks.mutex, portMAX_DELAY);
    if (sSerialLink.sCallbacks.psListHead == NULL)
    {
        /* Insert at start of list */
        sSerialLink.sCallbacks.psListHead = psNewEntry;
    }
    else
    {
        /* Insert at end of list */
        psCurrentEntry = sSerialLink.sCallbacks.psListHead;
        while (psCurrentEntry->psNext)
        {
            psCurrentEntry = psCurrentEntry->psNext;
        }
        
        psCurrentEntry->psNext = psNewEntry;
    }
    xSemaphoreGive(sSerialLink.sCallbacks.mutex);
    return E_SL_OK;
}



teSL_Status eSL_WriteMessage(uint16_t u16Type, uint16_t u16Length, uint8_t *pu8Data)
{
    uint8_t n;
    uint8_t u8CRC;
	uint8_t index = 0;
	static uint8_t au8LinkTxBuffer[SL_MAX_MESSAGE_LENGTH];

    u8CRC = u8SL_CalculateCRC(u16Type, u16Length, pu8Data);

	/* Load start character */
    index = u8SL_LoadByteToTxBuffer(true, SL_START_CHAR, au8LinkTxBuffer, index);
	
	/* Load message type */
	index = u8SL_LoadByteToTxBuffer(false, (u16Type >> 8) & 0xff, au8LinkTxBuffer, index);
	index = u8SL_LoadByteToTxBuffer(false, (u16Type >> 0) & 0xff, au8LinkTxBuffer, index);

	/* Load message length */
	index = u8SL_LoadByteToTxBuffer(false, (u16Length >> 8) & 0xff, au8LinkTxBuffer, index);
	index = u8SL_LoadByteToTxBuffer(false, (u16Length >> 0) & 0xff, au8LinkTxBuffer, index);

	/* Load message checksum */
	index = u8SL_LoadByteToTxBuffer(false, u8CRC, au8LinkTxBuffer, index);

	/* Load message payload */
    for(n = 0; n < u16Length; n++)
    {
    	index = u8SL_LoadByteToTxBuffer(false, pu8Data[n], au8LinkTxBuffer, index);
    }
	
	/* Load end character */
    index = u8SL_LoadByteToTxBuffer(true, SL_END_CHAR, au8LinkTxBuffer, index);

#if (defined(CACHE_MAINTENANCE) && (CACHE_MAINTENANCE == 1))
    /* Flush Dcache before start DMA */
    DCACHE_CleanByRange((uint32_t)&au8LinkTxBuffer[0], index);
#endif
	/* Send the buffer data */
	eSerial_WriteBuffer(au8LinkTxBuffer, index);

	return E_SL_OK;
}


static uint8_t u8SL_CalculateCRC(uint16_t u16Type, uint16_t u16Length, uint8_t *pu8Data)
{

    uint8_t n;
    uint8_t u8CRC;

    u8CRC  = (u16Type   >> 0) & 0xff;
    u8CRC ^= (u16Type   >> 8) & 0xff;
    u8CRC ^= (u16Length >> 0) & 0xff;
    u8CRC ^= (u16Length >> 8) & 0xff;

    for (n = 0; n < u16Length; n++)
    {
        u8CRC ^= pu8Data[n];
    }

    return(u8CRC);
}



uint8_t u8SL_LoadByteToTxBuffer(bool bSpecialCharacter, uint8_t u8Data, uint8_t *buffer, uint8_t index)
{
    if (!bSpecialCharacter && u8Data < 0x10)
    {
        /* Load escape character and escape byte */
        u8Data ^= 0x10;
		buffer[index++] = SL_ESC_CHAR;
    }
	buffer[index++] = u8Data;
	return index;
}



static bool bSL_RxByte(uint8_t *pu8Data)
{
    if (eSerial_Read(pu8Data) == E_SERIAL_OK)
    {
        return true;
    }
    else
    {
        return false;
    }
}

static teSL_Status eSL_MessageQueue(tsSerialLink *psSerialLink, uint16_t u16Type, uint16_t u16Length, uint8_t *pu8Message)
{
    for (uint8_t i = 0; i < SL_MAX_MESSAGE_QUEUES; i++)
    {
        taskENTER_CRITICAL();
        
        if (psSerialLink->asReaderMessageQueue[i].u16Type == u16Type)
        {            
            if (u16Type == E_SL_MSG_STATUS)
            {
                tsSL_Msg_Status *psRxStatus = (tsSL_Msg_Status*)pu8Message;
                tsSL_Msg_Status *psWaitStatus = (tsSL_Msg_Status*)psSerialLink->asReaderMessageQueue[i].pu8Message;
                
                /* Also check the type of the message that this is status to. */
                if (psWaitStatus)
                {
                //    LOG(ZBSERIAL, INFO, "Status listener for message type 0x%04X, rx 0x%04X\r\n", psWaitStatus->u16MessageType, pri_ntohs(psRxStatus->u16MessageType));                                        
                    if (psWaitStatus->u16MessageType != pri_ntohs(psRxStatus->u16MessageType))
                    {
                        psSerialLink->asReaderMessageQueue[i].u16Type = 0;
                        taskEXIT_CRITICAL();
    //                    LOG(ZBSERIAL, ERR, "Not the status listener for this Msg\r\n");

                        continue;
                    }
                }
            }
            
            uint8_t *pu8MessageCopy = pvPortMalloc(u16Length);
            assert(pu8MessageCopy != NULL);
            
            memcpy(pu8MessageCopy, pu8Message, u16Length);
                        
            psSerialLink->asReaderMessageQueue[i].u16Length = u16Length;
            psSerialLink->asReaderMessageQueue[i].pu8Message = pu8MessageCopy;

            taskEXIT_CRITICAL();

            /* Signal data available */
            xEventGroupSetBits(psSerialLink->asReaderMessageQueue[i].eventGroup, 0x01);
            
            return E_SL_OK;
        }
        else
        {
            taskEXIT_CRITICAL();
        }
    }
    
    return E_SL_NOMESSAGE;
}



static void serialReadTask(void * pvParameters)
{
    tsSerialLink *psSerialLink = (tsSerialLink *)pvParameters;
	tsSL_Message sMessage;
	bool iHandled = 0;
	
	while (1) {
        /* Initialise buffer */
        memset(&sMessage, 0, sizeof(tsSL_Message));
        /* Initialise length to large value so CRC is skipped if end received */
        sMessage.u16Length = 0xFFFF;
        
       	if (eSL_ReadMessage(&sMessage.u16Type, &sMessage.u16Length, SL_MAX_MESSAGE_LENGTH, sMessage.au8Message) == E_SL_OK)
    	{	
      //      LOG(ZBSERIAL, INFO, "Receive Msg = 0x%x, Len = %d\r\n", sMessage.u16Type, sMessage.u16Length);
    		if (sMessage.u16Type == E_SL_MSG_LOG)
        	{
            	iHandled = 1; /* Message handled by logger */
        	}
			else
			{
                teSL_Status eStatus = eSL_MessageQueue(psSerialLink, sMessage.u16Type, sMessage.u16Length, sMessage.au8Message);
                if (eStatus == E_SL_OK)
                {
                    iHandled = 1;
                }
                else if (eStatus == E_SL_NOMESSAGE)
                {
                  ;//  LOG(ZBSERIAL, INFO, "No listener waiting for message type 0x%04X\r\n", sMessage.u16Type);
                }
                else
                {
       			  ;	//  LOG(ZBSERIAL, ERR, "Enqueueing message attempt\r\n");
                }				
			}

            if ((sMessage.u16Type != E_SL_MSG_NODE_CLUSTER_LIST) 
                && (sMessage.u16Type != E_SL_MSG_NODE_ATTRIBUTE_LIST)
                && (sMessage.u16Type != E_SL_MSG_NODE_COMMAND_ID_LIST))
			{
            	// Search for callback handlers for this message type
            	tsSL_CallbackEntry *psCurrentEntry;
            	/* Search through list of registered callbacks */
            	xSemaphoreTake(psSerialLink->sCallbacks.mutex, portMAX_DELAY);
            
            	for (psCurrentEntry = psSerialLink->sCallbacks.psListHead; psCurrentEntry; psCurrentEntry = psCurrentEntry->psNext)
            	{
                	if (psCurrentEntry->u16Type == sMessage.u16Type)
                	{
                   		tsCallbackTaskData *psCallbackData;
                    	
                    	// Put the message into the queue for the callback handler thread
                    	psCallbackData = pvPortMalloc(sizeof(tsCallbackTaskData));
                    	assert(psCallbackData != NULL);

                    	memcpy(&psCallbackData->sMessage, &sMessage, sizeof(tsSL_Message));
                    	psCallbackData->prCallback = psCurrentEntry->prCallback;
                    	psCallbackData->pvUser = psCurrentEntry->pvUser;
                    
                    	if (pdPASS == xQueueSend(psSerialLink->sCallbackQueue, &psCallbackData, 0))
                    	{
                       		iHandled = 1;
                    	}								
                    	else
                    	{
             //           	LOG(ZBSERIAL, WARN, "Queue callback message failed\r\n");
                        	vPortFree(psCallbackData);
                    	}

                    	break; // just a single callback for each message type
                	}
            	}
            	xSemaphoreGive(psSerialLink->sCallbacks.mutex);
        	}
			
        	if (0 == iHandled)
        	{
          //  	LOG(ZBSERIAL, WARN, "Message 0x%04X was not handled\r\n", sMessage.u16Type);
        	}

        }		
	}
}


static void serialCallbackTask(void * pvParameters)
{
    tsCallbackTaskData *psCallbackData;
    tsSerialLink *psSerialLink = (tsSerialLink *)pvParameters;

    while (1) {
        if (pdPASS == xQueueReceive(psSerialLink->sCallbackQueue, &psCallbackData, portMAX_DELAY)) {
            psCallbackData->prCallback(psCallbackData->pvUser, psCallbackData->sMessage.u16Length, psCallbackData->sMessage.au8Message);
            vPortFree(psCallbackData);
        }
    }
}

