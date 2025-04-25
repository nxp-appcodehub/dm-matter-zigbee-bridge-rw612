/*
 * Copyright 2021-2023 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "board.h"
#include "fsl_usart.h"

#include "serial.h"
#include "fsl_debug_console.h" 

/*******************************************************************************
 * Definitions
 ******************************************************************************/

//#define ZBUART_DMA_MODE

#define ZBUART_TX_DMA_CHANNEL       3       /* Channel 0-2 are used by wifi driver */

#define ZBUART_TIMEOUT              pdMS_TO_TICKS(500)  /* 500ms timeout guard */
#define ZBUART_RXBUFF               1024

static QueueHandle_t        s_rxIrqDataQueue;

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*!
 * @brief eZb_Uart_Init function
 */
void eSerial_Init(void)
{
    usart_config_t config;
    status_t ret;
	/*
     * config.baudRate_Bps = 115200U;
     * config.parityMode = kLPUART_ParityDisabled;
     * config.stopBitCount = kLPUART_OneStopBit;
     * config.txFifoWatermark = 0;
     * config.rxFifoWatermark = 0;
     * config.enableTx = false;
     * config.enableRx = false;
     */
    USART_GetDefaultConfig(&config);
    config.baudRate_Bps = BOARD_ZB_UART_BAUDRATE;
    config.enableTx = true;
    config.enableRx = true;

  //  NVIC_SetPriority(BOARD_ZB_UART_IRQn, BOARD_ZB_UART_IRQLevel);

  //  ret=USART_Init(BOARD_ZB_UART_BASEADDR, &config,20000000/* BOARD_ZB_UART_CLK_FREQ*/); //comment as USART14 was init inside BOARD_InitDebugConsole

    /* Enable RX interrupt. */
    USART_EnableInterrupts(BOARD_ZB_UART_BASEADDR, kUSART_RxLevelInterruptEnable | kUSART_RxErrorInterruptEnable | kUSART_FramingErrorInterruptEnable);
    ret=EnableIRQ(BOARD_ZB_UART_IRQ);

    /* Initialise serial rx irq data queue */    
    s_rxIrqDataQueue = xQueueCreate(ZBUART_RXBUFF, 1);
    assert(s_rxIrqDataQueue != NULL);
}


void BOARD_ZB_UART_IRQ_HANDLER(void)
{
    uint8_t data;
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    /* If FrameError */
    if (USART_GetStatusFlags(BOARD_ZB_UART_BASEADDR) & kUSART_FramingErrorFlag)
    {
        USART_ClearStatusFlags(BOARD_ZB_UART_BASEADDR, kUSART_FramingErrorFlag);
        return;
    }
    
    /* If RX overrun. */
    if (USART_GetStatusFlags(BOARD_ZB_UART_BASEADDR) & kUSART_RxError)
    {
        USART_ClearStatusFlags(BOARD_ZB_UART_BASEADDR, kUSART_RxError);
        return;
    }

    /* If new data arrived. */
    if (USART_GetStatusFlags(BOARD_ZB_UART_BASEADDR) & kUSART_RxFifoNotEmptyFlag) // should also include kUSART_RxFifoFullFlag ???
    {
        BaseType_t status;
        
    	/* Obtain the new data  */
        data = USART_ReadByte(BOARD_ZB_UART_BASEADDR);

		/* Put the data into the queue */
        status = xQueueSendToBackFromISR(s_rxIrqDataQueue, &data, &xHigherPriorityTaskWoken);
        assert(pdTRUE == status);
    }

	/* Check if there is higher woken priority task */
	if (xHigherPriorityTaskWoken)
	{
		taskYIELD();
	}
	
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

teSerial_Status eSerial_Read(uint8_t *data)
{
    if (pdPASS == xQueueReceive(s_rxIrqDataQueue, data, portMAX_DELAY)) {
        return E_SERIAL_OK;
    }
    return E_SERIAL_NODATA;
}

void eSerial_WriteBuffer(uint8_t *data, uint8_t length)
{
    USART_WriteBlocking(BOARD_ZB_UART_BASEADDR, data, length);
    while (!(USART_GetStatusFlags(BOARD_ZB_UART_BASEADDR) & kUSART_TxFifoEmptyFlag));//use kUSART_TxFifoEmptyFlag to replace kUSART_TransmissionCompleteFlag
}


