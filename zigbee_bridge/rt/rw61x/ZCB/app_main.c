/*
* Copyright 2019, 2023-2025 NXP
* All rights reserved.
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include "zb_platform.h"
#include "EmbeddedTypes.h"

/* FreeRTOS kernel includes. */

#include "fsl_os_abstraction.h"
#ifdef FSL_RTOS_FREE_RTOS
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "portmacro.h"
#endif

#include "ZQueue.h"
#include "ZTimer.h"
#include "zigbee_config.h"
#include "app_crypto.h"
#include "fsl_gpio.h"
#include "fwk_platform.h"
#include "fsl_component_mem_manager.h"
#include "pwrm.h"

#include "app_coordinator.h"
#include "app_signals.h"

#include "app_uart.h"
#include "app_buttons.h"
#include "app_main.h"
#include "app_leds.h"
#include "zps_apl_zdo.h"
#include "dbg.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#define APP_NUM_STD_TMRS             (2)
#ifdef NCP_HOST
#define APP_QUEUE_SIZE               (30)

#ifndef ZCL_QUEUE_SIZE
#define ZCL_QUEUE_SIZE              35U
#endif

#else
#define APP_QUEUE_SIZE               (1)
#endif

#define APP_ZTIMER_STORAGE           ( APP_NUM_STD_TMRS )


/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
void ZPS_task();
void APP_task();
void vWakeCallBackZtimer();
/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
/* timers */
uint8_t u8TimerId;
uint8_t  u8LedTimer;
/* queue handles */
tszQueue APP_msgAppEvents;
uint32_t u32Togglems;
bool_t initialized = FALSE;
/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
static ZTIMER_tsTimer asTimers[APP_ZTIMER_STORAGE + ZIGBEE_TIMER_STORAGE];
extern const uint8_t gUseRtos_c;

static OSA_TASK_DEFINE(ZPS_task, gMainThreadPriority_c, 1, gMainThreadStackSize_c, 0);

OSA_TASK_HANDLE_DEFINE(ZPS_taskHandle);

OSA_SEMAPHORE_HANDLE_DEFINE(ZPS_semaphoreHandle);
OSA_SEMAPHORE_HANDLE_DEFINE(APP_semaphoreHandle);
OSA_MUTEX_HANDLE_DEFINE(zb_task_lock);

PWR_tsWakeTimerEvent sZTimer;
#define WAKEUP_TIMER_VALUE_MS 10

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

extern void vAppMain(void);

/****************************************************************************
 *
 * NAME: main_task
 *
 * DESCRIPTION:
 * Main  execution loop
 *
 * RETURNS:
 * Never
 *
 ****************************************************************************/
void zigbee_main(uint32_t parameter)
{
    /* e.g. osaEventFlags_t ev; */

    /* Wait NBU fw to load and wait Matter to start */
    vTaskDelay(10000);
    if(!initialized)
    {
        /* place initialization code here... */
        PLATFORM_InitTimerManager();
        CRYPTO_Init();
        CRYPTO_u8RandomInit();
        MEM_Init();

        (void)OSA_SemaphoreCreate((osa_semaphore_handle_t)ZPS_semaphoreHandle, 1);
        (void)OSA_SemaphoreCreate((osa_semaphore_handle_t)APP_semaphoreHandle, 1);
        (void)OSA_MutexCreate((osa_mutex_handle_t)zb_task_lock);

        vAppMain();

        (void)OSA_TaskCreate(ZPS_taskHandle, OSA_TASK(ZPS_task), NULL);

        memset( &sZTimer, 0x0, sizeof(PWR_tsWakeTimerEvent));
        PWR_eScheduleActivity(&sZTimer, WAKEUP_TIMER_VALUE_MS, vWakeCallBackZtimer);
        initialized = TRUE;
    }
    APP_task();
}

void vWakeCallBackZtimer(void)
{
    zbTaskletsSignalApp();
    memset( &sZTimer, 0x0, sizeof(PWR_tsWakeTimerEvent));
    PWR_eScheduleActivity(&sZTimer, WAKEUP_TIMER_VALUE_MS, vWakeCallBackZtimer);
}

void APP_task()
{
    while(1)
    {
        if (OSA_SemaphoreWait((osa_semaphore_handle_t)APP_semaphoreHandle, osaWaitForever_c) == KOSA_StatusSuccess)
        {
            OSA_MutexLock((osa_mutex_handle_t) zb_task_lock, osaWaitForever_c);
            ZTIMER_vTask();
            APP_taskCoordinator();
            bdb_taskBDB();
            OSA_MutexUnlock((osa_mutex_handle_t) zb_task_lock);
        }
        if(!gUseRtos_c)
        {
            break;
        }
    }
}

void ZPS_task()
{
    while(1)
    {
        if (OSA_SemaphoreWait((osa_semaphore_handle_t)ZPS_semaphoreHandle, osaWaitForever_c) == KOSA_StatusSuccess)
        {
            OSA_MutexLock((osa_mutex_handle_t) zb_task_lock, osaWaitForever_c);
            zps_taskZPS();
            OSA_MutexUnlock((osa_mutex_handle_t) zb_task_lock);
        }
        if(!gUseRtos_c)
        {
            break;
        }
    }
}

void zbTaskletsSignalZps()
{
    if (initialized)
    {
        OSA_SemaphorePost((osa_semaphore_handle_t)ZPS_semaphoreHandle);
    }
}

void zbTaskletsSignalApp()
{
    if (initialized)
    {
        OSA_SemaphorePost((osa_semaphore_handle_t)APP_semaphoreHandle);
    }
}
/****************************************************************************
 *
 * NAME: APP_vInitResources
 *
 * DESCRIPTION:
 * Initialise resources (timers, queue's etc)
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
void APP_vInitResources(void)
{
    /* Initialise the Z timer module */
    ZTIMER_eInit(asTimers, sizeof(asTimers) / sizeof(ZTIMER_tsTimer));

    /* Create Z timers */
    ZTIMER_eOpen(&u8TimerId,            APP_cbTimerId,          NULL, ZTIMER_FLAG_PREVENT_SLEEP);
    ZTIMER_eOpen(&u8LedTimer,           APP_cbTimerLed,  NULL, ZTIMER_FLAG_PREVENT_SLEEP);
    ZQ_vQueueCreate(&APP_msgAppEvents,        APP_QUEUE_SIZE,          sizeof(APP_tsEvent),         NULL);
    ZQ_vRegisterCallback(&APP_msgAppEvents, zbTaskletsSignalApp);
}

/****************************************************************************
*
* NAME: APP_cbTimerLed
*
* DESCRIPTION:
* Timer callback to to toggle LEDs
*
* PARAMETER:
*
* RETURNS:
*
****************************************************************************/
void APP_cbTimerLed(void *pvParam)
{
    static bool_t bCurrentState = TRUE;
    APP_vSetLed(APP_E_LEDS_LED_2, bCurrentState);
    if( ZPS_bGetPermitJoiningStatus()|| u32Togglems != 500)
    {
        ZTIMER_eStart(u8LedTimer, ZTIMER_TIME_MSEC(u32Togglems));
        bCurrentState = !bCurrentState;
    }
    else
    {
        APP_vSetLed(APP_E_LEDS_LED_2, APP_E_LED_ON);
    }

}
/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/