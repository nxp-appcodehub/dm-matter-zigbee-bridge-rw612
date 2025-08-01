/*
 * Copyright 2021-2023, 2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
 
#pragma GCC diagnostic ignored "-Wshift-count-overflow"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "semphr.h"

#include "board.h"
#include "fsl_usart.h"

#include "fsl_device_registers.h"
#include "fsl_debug_console.h"
#include "fsl_os_abstraction.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "ZigbeeConstant.h"
#include "SerialLink.h"
#include "ZigbeeDevices.h"
#include "cmd.h"
#include "zcb.h"
#include "zcl.h"
#include "app_common.h"
#include "app_main.h"
#include "ram_storage.h"
#include "PDM.h"

#include "zigbee_cmd.h"

#include "ZcbMessage.h"

#include "CHIPProjectAppConfig.h"

#include "LevelControl.h"

#include "ColourControl.h"

#include "OnOff.h"

#define ZB_DEVICE_MESSAGE_TIMER_OUT_COUNT    5

extern OSA_MUTEX_HANDLE_DEFINE(zb_task_lock);

/* to calculate the time of device receiving last message */
typedef struct
{
    TimerHandle_t xTimers;
    uint16_t count;
} tsZbDeviceMsgTimer;

newdb_zcb_t sZcb;

static JoinedNodesSaved savedNodes;

// ---------------------------------------------------------------
// External Function Prototypes
// ---------------------------------------------------------------
extern tsZbNetworkInfo zbNetworkInfo;
extern tsZbDeviceInfo deviceTable[MAX_ZD_DEVICE_NUMBERS];
extern tsZbDeviceAttribute attributeTable[MAX_ZD_ATTRIBUTE_NUMBERS_TOTAL];
//extern lfs_t g_lfs; 
extern char g_OtaImagePath[ZB_DEVICE_OTA_IMAGE_PATH_MAX_LENGTH];

// ---------------------------------------------------------------
// Local Function Prototypes
// ---------------------------------------------------------------

void ZCB_HandleDeviceAnnounce            (uint16_t u16NwkAddr, uint64_t u64IeeeAddr, uint8_t u8Capability);
void ZCB_HandleDeviceLeave               (uint64_t u64ExtAddr, uint8_t u8Rejoin);
void ZCB_HandleSimpleDescriptorResponse  (uint16_t u16NwkAddrOfInterest, uint8_t u8Endpoint, uint16_t u16ApplicationProfileId,
    uint16_t u16DeviceId, uint8_t u8Value, uint8_t u8InClusterCount, uint8_t u8OutClusterCount,
    uint16_t au16Data[34]);
void ZCB_HandleAttributeReport           (uint16_t u16ShortAddress, uint8_t u8EndPoint, uint16_t u16ClusterId, 
                                          uint16_t u16AttributeId, uint8_t u8AttributeStatus, uint8_t u8AttributeType, 
                                          uint16_t u16SizeOfAttributesInBytes, uint64_t u64Data);
void ZCB_HandleReadAttrResp              (uint16_t u16ShortAddress, uint8_t u8EndPoint, uint16_t u16ClusterId, 
                                          uint16_t u16AttributeId, uint8_t u8AttributeStatus, uint8_t u8AttributeType, 
                                          uint16_t u16SizeOfAttributesInBytes, uint8_t     auAttributeValue[50]);
void ZCB_HandleActiveEndPointResp        (uint16_t u16NwkAddrOfInterest, uint8_t u8ActiveEpCount, uint8_t* pu8ActiveEpList);
void ZCB_HandleNetworkAddressReponse     (uint64_t u64IeeeAddrRemoteDev, uint16_t u16NwkAddrRemoteDev);
void ZCB_HandleIeeeAddressReponse        (uint64_t u64IeeeAddrRemoteDev, uint16_t u16NwkAddrRemoteDev);

NodeDB JoinedNodes[DEV_NUM];
uint8_t idx=0;

const char * myDB_filename = "ZBNodes";
uint16_t SIZE= DEV_NUM*sizeof(NodeDB);

// ---------------------------------------------------------------
// Exported Functions
// ---------------------------------------------------------------

void eZCB_Init(void) 
{
	uint8_t i;

    gpio_pin_config_t led_config = {kGPIO_DigitalOutput,0,};

	GPIO_PortInit(GPIO, 1);
	GPIO_PinInit(GPIO, 1, 55-32, &led_config);
	
    /* Hold reset pin until all init process done */
    GPIO_PinWrite(GPIO, 1, 55-32, 0);

    vZbDeviceTable_Init();
    
    /* Release reset pin to start Zigbee */
    GPIO_PinWrite(GPIO, 1, 55-32, 1);
	
	for (i=0;i<DEV_NUM;i++)
		JoinedNodes[i].type=JoinedNodes[i].ep=JoinedNodes[i].shortaddr=JoinedNodes[i].mac=0;
}

teZcbStatus eOnOff()
{
    APP_tsEvent sButtonEvent;

    sButtonEvent.eType = APP_E_EVENT_SERIAL_TOGGLE;

    ZQ_bQueueSend(&APP_msgAppEvents, &sButtonEvent);

    return E_ZCB_OK;
}

teZcbStatus eOn_Off( uint16_t u16ShortAddress, uint8_t u8Mode ) 
{
    uint8_t u8DstEp = 1;
    uint8_t u8SrcEp = 1;

    tsZCL_Address sAddress;
    sAddress.eAddressMode                      =  2;
    sAddress.uAddress.u16DestinationAddress    =  u16ShortAddress;

    uint8_t u8SeqNum;
    OSA_MutexLock((osa_mutex_handle_t) zb_task_lock, osaWaitForever_c);
    eCLD_OnOffCommandSend(u8SrcEp, u8DstEp, &sAddress, &u8SeqNum, u8Mode);
    OSA_MutexUnlock((osa_mutex_handle_t) zb_task_lock);
    return E_ZCB_OK;
}

teZcbStatus eLevelControlMove(uint8_t u8AddrMode, 
                              uint16_t u16Addr, 
                              uint8_t u8SrcEp, 
                              uint8_t u8DstEp,
                              uint8_t u8OnOff,
                              uint8_t u8Mode,
                              uint8_t u8Rate)
{
    tsZCL_Address sAddress;
    sAddress.eAddressMode                      =  u8AddrMode;
    sAddress.uAddress.u16DestinationAddress    =  u16Addr;

    tsCLD_LevelControl_MoveCommandPayload    sCommand;
    sCommand.u8MoveMode     =  u8Mode;
    sCommand.u8Rate         =  u8Rate;

    uint8_t u8SeqNum;
    OSA_MutexLock((osa_mutex_handle_t) zb_task_lock, osaWaitForever_c);
    eCLD_LevelControlCommandMoveCommandSend(u8SrcEp, u8DstEp, &sAddress, &u8SeqNum, u8OnOff, &sCommand);
    OSA_MutexUnlock((osa_mutex_handle_t) zb_task_lock);
    return E_ZCB_OK;
}


teZcbStatus eLevelControlMoveToLevel(uint16_t u16Addr, 
                                     //uint8_t u8OnOff,
                                     uint8_t u8Level,
                                     uint16_t u16Time)
{
    uint8_t u8DstEp = 1;
    uint8_t u8SrcEp = 1;

    tsZCL_Address sAddress;
    sAddress.eAddressMode                      =  2;
    sAddress.uAddress.u16DestinationAddress    =  u16Addr;

    tsCLD_LevelControl_MoveToLevelCommandPayload sCommand;
    sCommand.u8Level =  u8Level;
    sCommand.u16TransitionTime = u16Time;

    uint8_t u8SeqNum;
    OSA_MutexLock((osa_mutex_handle_t) zb_task_lock, osaWaitForever_c);
    eCLD_LevelControlCommandMoveToLevelCommandSend(u8SrcEp, u8DstEp, &sAddress, &u8SeqNum, 1, &sCommand);
    OSA_MutexUnlock((osa_mutex_handle_t) zb_task_lock);

    return E_ZCB_OK;
}



teZcbStatus eLevelControlMoveStep(uint8_t u8AddrMode, 
                                  uint16_t u16Addr, 
                                  uint8_t u8SrcEp, 
                                  uint8_t u8DstEp,
                                  uint8_t u8OnOff,
                                  uint8_t u8Mode,
                                  uint8_t u8Size,
                                  uint16_t u16Time)
{
    tsZCL_Address sAddress;
    sAddress.eAddressMode                      =  u8AddrMode;
    sAddress.uAddress.u16DestinationAddress    =  u16Addr;

    tsCLD_LevelControl_StepCommandPayload     sCommand;
    sCommand.u8StepMode           =  u8Mode;
    sCommand.u8StepSize           =  u8Size;
    sCommand.u16TransitionTime    =  u16Time;

    uint8_t u8SeqNum;

    OSA_MutexLock((osa_mutex_handle_t) zb_task_lock, osaWaitForever_c);
    eCLD_LevelControlCommandStepCommandSend(u8SrcEp, u8DstEp, &sAddress, &u8SeqNum, u8OnOff, &sCommand);
    OSA_MutexUnlock((osa_mutex_handle_t) zb_task_lock);

    return E_ZCB_OK;
}


teZcbStatus eColorControlMoveToColor(uint16_t u16Addr, 
									 uint16_t u16ColorX,
									 uint16_t u16ColorY,
                                     uint16_t u16Time)
{
    uint8_t u8DstEp = 1;
    uint8_t u8SrcEp = 1;
    tsZCL_Address sAddress;
    sAddress.eAddressMode                      =  2;
    sAddress.uAddress.u16DestinationAddress    =  u16Addr;

    tsCLD_ColourControl_MoveToColourCommandPayload    sPayload;
    sPayload.u16ColourX           =  u16ColorX;
    sPayload.u16ColourY           =  u16ColorY;
    sPayload.u16TransitionTime    =  u16Time;

    uint8_t u8SeqNum;

    OSA_MutexLock((osa_mutex_handle_t) zb_task_lock, osaWaitForever_c);
    eCLD_ColourControlCommandMoveToColourCommandSend(u8SrcEp, u8DstEp, &sAddress, &u8SeqNum, &sPayload);
    OSA_MutexUnlock((osa_mutex_handle_t) zb_task_lock);
    return E_ZCB_OK;
}


teZcbStatus eColorControlMoveToTemp(uint16_t u16Addr, 
                                    uint16_t u16ColorTemp,
                                    uint16_t u16Time)
{
	uint8_t u8DstEp = 1;
    uint8_t u8SrcEp = 1;
    
    tsZCL_Address sAddress;
    sAddress.eAddressMode                      =  2;
    sAddress.uAddress.u16DestinationAddress    =  u16Addr;

    tsCLD_ColourControl_MoveToColourTemperatureCommandPayload    sPayload;
    sPayload.u16ColourTemperatureMired    =  u16ColorTemp;
    sPayload.u16TransitionTime            =  u16Time;

    uint8_t u8SeqNum;
    OSA_MutexLock((osa_mutex_handle_t) zb_task_lock, osaWaitForever_c);
    eCLD_ColourControlCommandMoveToColourTemperatureCommandSend(u8SrcEp, u8DstEp, &sAddress, &u8SeqNum, &sPayload);
	OSA_MutexUnlock((osa_mutex_handle_t) zb_task_lock);

    return E_ZCB_OK;
}

teZcbStatus eColorControlMoveToHue(uint16_t u16Addr, 
                                   uint8_t u8Hue,
                                   uint8_t u8Dir,
                                   uint16_t u16Time)
{
    uint8_t u8DstEp = 1;
    uint8_t u8SrcEp = 1;

    tsZCL_Address sAddress;
    sAddress.eAddressMode                      =  2;
    sAddress.uAddress.u16DestinationAddress    =  u16Addr;

    tsCLD_ColourControl_MoveToHueCommandPayload    sPayload;
    sPayload.eDirection           = u8Dir;
    sPayload.u8Hue                =  u8Hue;
    sPayload.u16TransitionTime    =  u16Time;

    uint8_t u8SeqNum;

    OSA_MutexLock((osa_mutex_handle_t) zb_task_lock, osaWaitForever_c);
    eCLD_ColourControlCommandMoveToHueCommandSend(u8SrcEp, u8DstEp, &sAddress, &u8SeqNum, &sPayload);
    OSA_MutexUnlock((osa_mutex_handle_t) zb_task_lock);
    return E_ZCB_OK;
}
teZcbStatus eColorControlMoveToSaturation(uint16_t u16Addr, 
                                   uint8_t u8Sat,
                                   uint16_t u16Time)
{
    uint8_t u8DstEp = 1;
    uint8_t u8SrcEp = 1;

    tsZCL_Address sAddress;
    sAddress.eAddressMode                      =  2;
    sAddress.uAddress.u16DestinationAddress    =  u16Addr;
    
    tsCLD_ColourControl_MoveToSaturationCommandPayload    sPayload;
    sPayload.u8Saturation         = u8Sat;
    sPayload.u16TransitionTime    =  u8Sat;

    uint8_t u8SeqNum;

    OSA_MutexLock((osa_mutex_handle_t) zb_task_lock, osaWaitForever_c);
    eCLD_ColourControlCommandMoveToSaturationCommandSend(u8SrcEp, u8DstEp, &sAddress, &u8SeqNum, &sPayload);
    OSA_MutexUnlock((osa_mutex_handle_t) zb_task_lock);
    return E_ZCB_OK;
}

// ------------------------------------------------------------------
// Handlers
// ------------------------------------------------------------------

bool EnumJoinedNodes(void)
{
	uint8_t j;
	int ret=0;

#ifdef CONFIG_NVS
    uint16 u16ByteRead;
    ret = (PDM_eReadDataFromRecord(PDM_ID_APP_BRIDGE, (uint8_t *)&savedNodes, sizeof(savedNodes), &u16ByteRead) == 0);
#else
    ret = ramStorageReadFromFlash(myDB_filename,(uint8_t *)&savedNodes,sizeof(savedNodes));
#endif
	if (ret)
	{
		PRINTF("\n ### ramStorageReadFromFlash=%d,Joined Nodes=%d",ret,savedNodes.totalNodes);
		for (j=0;j<savedNodes.totalNodes;j++)
		{
			JoinedNodes[j].ep = savedNodes.joinedNodes[j].ep;
			JoinedNodes[j].mac = savedNodes.joinedNodes[j].mac;
			JoinedNodes[j].type = savedNodes.joinedNodes[j].type;
			JoinedNodes[j].shortaddr = savedNodes.joinedNodes[j].shortaddr;
			PRINTF("\n ### Idx=%d,Type=%d,short=0x%x,mac=0x%llx,ep=%d",j,savedNodes.joinedNodes[j].type,savedNodes.joinedNodes[j].shortaddr,savedNodes.joinedNodes[j].mac,savedNodes.joinedNodes[j].ep);
		}
		return 1;
	}
	else
		return 0;
}

void eZCB_SendMsg(int MsgType, newdb_zcb_t *Zcb, void* data)
{
	if (xSemaphoreTake(ZcbMsg.bridge_mutex, portMAX_DELAY) != pdTRUE)
		PRINTF("\n *** Failed to take semaphore *** ");

 //   ZcbMsg.HandleMask = false;

    if( Zcb != NULL ) {
        ZcbMsg.zcb = *Zcb;
        ZcbMsg.msg_type = MsgType;
        ZcbMsg.msg_data = data;
    } else {
        ZcbMsg.msg_type = MsgType;
    }
	
	xSemaphoreGive(ZcbMsg.bridge_mutex);
	return ;
}

void SaveJoinedNodes(void)
{
	uint8_t i,j,NewNodeCnt=0;
	uint8_t DupNodeCnt=0;
	int ret=0;
    uint8_t NewNode[DEV_NUM]={0,0,0,0,0};
	uint8_t DupNode[DEV_NUM]={0,0,0,0,0};
	bool EmptyFlash=0;
	
#ifdef CONFIG_NVS
    uint16 u16ByteRead;
    ret = (PDM_eReadDataFromRecord(PDM_ID_APP_BRIDGE, (uint8_t *)&savedNodes, sizeof(savedNodes), &u16ByteRead) == 0);
#else
    ret = ramStorageReadFromFlash(myDB_filename,(uint8_t *)&savedNodes,sizeof(savedNodes));
#endif
	if (ret)
	{
		PRINTF("\n ### ramStorageReadFromFlash=%d,Joined Nodes=%d",ret,savedNodes.totalNodes);
		for (j=0;j<savedNodes.totalNodes;j++)
			PRINTF("\n ### Idx=%d,Type=%d,short=0x%x,mac=0x%llx,ep=%d",j,savedNodes.joinedNodes[j].type,savedNodes.joinedNodes[j].shortaddr,savedNodes.joinedNodes[j].mac,savedNodes.joinedNodes[j].ep);
	}
	
	if (savedNodes.totalNodes)
	{
	  for (i=0;i<DEV_NUM;i++)
	  {
	   if (JoinedNodes[i].type)
	   {
	     for (j=0;j<savedNodes.totalNodes;j++)
	  	 if ((savedNodes.joinedNodes[j].shortaddr==JoinedNodes[i].shortaddr)&&(savedNodes.joinedNodes[j].mac==JoinedNodes[i].mac)&&(savedNodes.joinedNodes[j].ep==JoinedNodes[i].ep)&&(savedNodes.joinedNodes[j].type==JoinedNodes[i].type))
	  	 {
	  		DupNode[DupNodeCnt++]=i;
			PRINTF("\n ### Found Dup Node = %d(%d)\n",DupNodeCnt,i);
	  	 }
		 else
		 {	
		    NewNode[NewNodeCnt++]=i;
			PRINTF("\n ### Found New Node = %d(%d)\n",NewNodeCnt,i);
		 }
	   }
	  }
	}
	else
	{
		EmptyFlash=1;
	  for (i=0;i<DEV_NUM;i++)
		if (JoinedNodes[i].type)
		{
	  	   savedNodes.joinedNodes[savedNodes.totalNodes].type = JoinedNodes[i].type;	
		   savedNodes.joinedNodes[savedNodes.totalNodes].shortaddr = JoinedNodes[i].shortaddr;
		   savedNodes.joinedNodes[savedNodes.totalNodes].mac = JoinedNodes[i].mac;
		   savedNodes.joinedNodes[savedNodes.totalNodes].ep = JoinedNodes[i].ep;
		   savedNodes.totalNodes++;		
		}
	}

   if (!EmptyFlash)
   {
    for (i=0;i<DEV_NUM;i++)
	 for (j=0;j<NewNodeCnt;j++)
	  if ((JoinedNodes[i].type)&&(NewNode[j]==i))
	  {
	  	   savedNodes.joinedNodes[savedNodes.totalNodes].type = JoinedNodes[i].type;	
		   savedNodes.joinedNodes[savedNodes.totalNodes].shortaddr = JoinedNodes[i].shortaddr;
		   savedNodes.joinedNodes[savedNodes.totalNodes].mac = JoinedNodes[i].mac;
		   savedNodes.joinedNodes[savedNodes.totalNodes].ep = JoinedNodes[i].ep;
		   savedNodes.totalNodes++;
	  } 
   	}  
	  
	if (savedNodes.totalNodes)
	{
	  
#ifdef CONFIG_NVS
    ret = (PDM_eSaveRecordData(PDM_ID_APP_BRIDGE, (uint8_t *)&savedNodes, sizeof(savedNodes)) == 0);
#else
    ret = ramStorageSavetoFlash(myDB_filename,(uint8_t *)&savedNodes,sizeof(savedNodes));
#endif
	  if (ret)
		  PRINTF("\n ### ramStorageSavetoFlash=%d,Joined Nodes=%d\n",ret,savedNodes.totalNodes);
	  
	}
}

void RestoreJoinedNodes(void)
{
	uint8_t i;
	int ret=0;

#ifdef CONFIG_NVS
    uint16 u16ByteRead;
    ret = (PDM_eReadDataFromRecord(PDM_ID_APP_BRIDGE, (uint8_t *)&savedNodes, sizeof(savedNodes), &u16ByteRead) == 0);
#else
    ret = ramStorageReadFromFlash(myDB_filename,(uint8_t *)&savedNodes,sizeof(savedNodes));
#endif
	if (ret)
	{
		PRINTF("\n ### ramStorageReadFromFlash=%d,Joined Nodes=%d",ret,savedNodes.totalNodes);
		for (i=0;i<savedNodes.totalNodes;i++)
		{
			if (savedNodes.joinedNodes[i].type)
			{
			  JoinedNodes[i].ep = savedNodes.joinedNodes[i].ep;
			  JoinedNodes[i].mac = savedNodes.joinedNodes[i].mac; 
			  JoinedNodes[i].type = savedNodes.joinedNodes[i].type;
			  JoinedNodes[i].shortaddr = savedNodes.joinedNodes[i].shortaddr;
			  sZcb.type=JoinedNodes[i].type;
			  sZcb.DynamicEP=JoinedNodes[i].ep;
			  eZCB_SendMsg(BRIDGE_RESTORE_JOINED_NODE,&sZcb,NULL); 
			  PRINTF("\n ### Idx=%d,Type=%d,short=0x%x,mac=0x%llx,ep=%d",i,savedNodes.joinedNodes[i].type,savedNodes.joinedNodes[i].shortaddr,savedNodes.joinedNodes[i].mac,savedNodes.joinedNodes[i].ep);
			  vTaskDelay(500);			  
			}
		}
	}	
	
}

void StoreJoinedNode(NodeDB NewNode)
{
	uint8_t j;
	int ret=0;

	PRINTF("\n ### Node Type=%d,Short=0x%x,MAC=0x%llx,EP=%d\n",NewNode.type,NewNode.shortaddr,NewNode.mac,NewNode.ep);

	
#ifdef CONFIG_NVS
    uint16 u16ByteRead;
    ret = (PDM_eReadDataFromRecord(PDM_ID_APP_BRIDGE, (uint8_t *)&savedNodes, sizeof(savedNodes), &u16ByteRead) == 0);
#else
    ret = ramStorageReadFromFlash(myDB_filename,(uint8_t *)&savedNodes,sizeof(savedNodes));
#endif
	if (ret)
	{
		PRINTF("\n ### ramStorageReadFromFlash=%d,Joined Nodes=%d",ret,savedNodes.totalNodes);
		for (j=0;j<savedNodes.totalNodes;j++)
		{
			PRINTF("\n ### Idx=%d,Type=%d,short=0x%x,mac=0x%llx,ep=%d",j,savedNodes.joinedNodes[j].type,savedNodes.joinedNodes[j].shortaddr,savedNodes.joinedNodes[j].mac,savedNodes.joinedNodes[j].ep);
		
			if (savedNodes.joinedNodes[j].mac==NewNode.mac)
			{
				savedNodes.joinedNodes[j].type=NewNode.type;
				savedNodes.joinedNodes[j].shortaddr=NewNode.shortaddr;
				savedNodes.joinedNodes[j].ep=NewNode.ep;
				
#ifdef CONFIG_NVS
                ret = (PDM_eSaveRecordData(PDM_ID_APP_BRIDGE, (uint8_t *)&savedNodes, sizeof(savedNodes)) == 0);
#else
                ret = ramStorageSavetoFlash(myDB_filename,(uint8_t *)&savedNodes,sizeof(savedNodes));
#endif
				return;
			}	
		}
	}

  if (NewNode.type)
  {
	 savedNodes.joinedNodes[savedNodes.totalNodes].type = NewNode.type;	  
	 savedNodes.joinedNodes[savedNodes.totalNodes].shortaddr = NewNode.shortaddr;
	 savedNodes.joinedNodes[savedNodes.totalNodes].mac = NewNode.mac;
	 savedNodes.joinedNodes[savedNodes.totalNodes].ep = NewNode.ep;
	 savedNodes.totalNodes++;	
  }

  if (savedNodes.totalNodes)
  {
#ifdef CONFIG_NVS
    ret = (PDM_eSaveRecordData(PDM_ID_APP_BRIDGE, (uint8_t *)&savedNodes, sizeof(savedNodes)) == 0);
#else
    ret = ramStorageSavetoFlash(myDB_filename,(uint8_t *)&savedNodes,sizeof(savedNodes));
#endif
	if (ret)
		PRINTF("\n ### ramStorageSavetoFlash=%d,Joined Nodes=%d\n",ret,savedNodes.totalNodes);
  }
	return ;
}

void UpdateJoinedNodes(uint16_t EP)
{
  if ((JoinedNodes[idx].type)&&(JoinedNodes[idx].ep==0)&&(EP!=0))
  {
	JoinedNodes[idx].ep=EP;
	StoreJoinedNode(JoinedNodes[idx]);
  }	
  else
  	PRINTF("\n ### New Joined Nodes Update Failure !!!\n");
}

void ZCB_HandleDeviceAnnounce(uint16_t u16NwkAddr, uint64_t u64IeeeAddr, uint8_t u8Capability)

{
    PRINTF("ZCB_HandleDeviceAnnounce\r\n");
	for (int i = 0; i < DEV_NUM; i++)
	{
		if ((JoinedNodes[i].type==0)&&(JoinedNodes[i].ep==0))
		{
			JoinedNodes[i].shortaddr=u16NwkAddr;
			JoinedNodes[i].mac=u64IeeeAddr;
			JoinedNodes[i].type=1;

			idx=i;
			break;
		}
	}	

    tsZbDeviceInfo* sDevice = NULL;

    if (tZDM_FindDeviceByIeeeAddress(u64IeeeAddr) == NULL) {
        if ((sDevice = tZDM_AddNewDeviceToDeviceTable(u16NwkAddr, u64IeeeAddr)) != NULL) 
		{
            vZDM_NewDeviceQualifyProcess(sDevice);
        }
    }     
}

void ZCB_HandleDeviceLeave(uint64_t u64ExtAddr, uint8_t u8Rejoin) 
{
	uint8_t i, j, Idx;
	int ret = 0;
    PRINTF("ZCB_HandleDeviceLeave\r\n");
	for (i = 0; i < DEV_NUM; i++)
	{
		if (JoinedNodes[i].mac == u64ExtAddr)
		{
			PRINTF("\n ### %d: Remove Node : 0x%llx", i, u64ExtAddr);
#ifdef CONFIG_NVS
            uint16 u16ByteRead;
            ret = (PDM_eReadDataFromRecord(PDM_ID_APP_BRIDGE, (uint8_t *)&savedNodes, sizeof(savedNodes), &u16ByteRead) == 0);
#else
            ret = ramStorageReadFromFlash(myDB_filename,(uint8_t *)&savedNodes,sizeof(savedNodes));
#endif
			if (ret)
			{
			  for (j=0;j<savedNodes.totalNodes;j++)
				if (savedNodes.joinedNodes[j].mac==JoinedNodes[i].mac)
				{
					Idx=j;
					break;
				}	
			  if (Idx)
			  {
			    for (j=Idx;j<savedNodes.totalNodes;j++)	
			    {
					savedNodes.joinedNodes[j].type=savedNodes.joinedNodes[j+1].type;
					savedNodes.joinedNodes[j].ep=savedNodes.joinedNodes[j+1].ep;
					savedNodes.joinedNodes[j].mac=savedNodes.joinedNodes[j+1].mac;
					savedNodes.joinedNodes[j].shortaddr=savedNodes.joinedNodes[j+1].shortaddr;
			    }
				savedNodes.totalNodes-=1;
				if (savedNodes.totalNodes)
				{
#ifdef CONFIG_NVS
                    ret = (PDM_eSaveRecordData(PDM_ID_APP_BRIDGE, (uint8_t *)&savedNodes, sizeof(savedNodes)) == 0);
#else
                    ret = ramStorageSavetoFlash(myDB_filename,(uint8_t *)&savedNodes,sizeof(savedNodes));
#endif
				  if (ret)
					  PRINTF("\n ### ramStorageSavetoFlash=%d,Joined Nodes=%d\n",ret,savedNodes.totalNodes);
				}
			  } 	
			}			
			JoinedNodes[i].type=JoinedNodes[i].ep=JoinedNodes[i].shortaddr=JoinedNodes[i].mac=0;
            sZcb.matterIndex = i;
		}	
	}
	
    tsZbDeviceInfo *sDevice = tZDM_FindDeviceByIeeeAddress(u64ExtAddr);
    if (sDevice == NULL)
        return;
	
    sDevice->eDeviceState = E_ZB_DEVICE_STATE_LEFT;
    eZCB_SendMsg(BRIDGE_REMOVE_DEV, &sZcb, NULL);
    bZDM_EraseDeviceFromDeviceTable(u64ExtAddr);
}

void handleAttribute( uint16_t u16ShortAddress,
                      uint16_t u16ClusterID,
                      uint16_t u16AttributeID,
                      uint64_t u64Data,
                      uint8_t  u8Endpoint ) 
{

    uint64_t u64IEEEAddress = 0;
    newdb_zcb_t zcb;
    ZcbAttribute_t *msg_data = NULL;
	
	{
      switch ( u16ClusterID ) 
	  {
		case E_ZB_CLUSTERID_ONOFF:
			switch ( u16AttributeID ) 
			{
			case E_ZB_ATTRIBUTEID_ONOFF_ONOFF:
    		{
                    msg_data = (ZcbAttribute_t *)malloc(sizeof(ZcbAttribute_t));
					if (msg_data)
					{
                    msg_data->u16ClusterID = u16ClusterID;
                    msg_data->u16AttributeID = u16AttributeID;
                    msg_data->u64Data = u64Data;
                    eZCB_SendMsg(BRIDGE_WRITE_ATTRIBUTE, &sZcb, msg_data);	
					free(msg_data);
					msg_data=NULL;					
					}
					else
						PRINTF("\n ### Fail to malloc buf !!!");
			}
			break;
			default: 
				break;
			}
		break;
		
		case E_ZB_CLUSTERID_LEVEL_CONTROL:
			switch ( u16AttributeID ) 
			{
			case E_ZB_ATTRIBUTEID_LEVEL_CURRENTLEVEL:
			{
                    msg_data = (ZcbAttribute_t *)malloc(sizeof(ZcbAttribute_t));
					if (msg_data)
					{
                    msg_data->u16ClusterID = u16ClusterID;
                    msg_data->u16AttributeID = u16AttributeID;
                    msg_data->u64Data = u64Data;
                    eZCB_SendMsg(BRIDGE_WRITE_ATTRIBUTE, &sZcb, msg_data);		
					free(msg_data);
					msg_data=NULL;					
					}
					else
						PRINTF("\n ### Fail to malloc buf !!!");
			}
			break;
			default: 
				break;
			}
		break;
		
		case E_ZB_CLUSTERID_MEASUREMENTSENSING_ILLUM:
            switch ( u16AttributeID ) 
            {
	            case E_ZB_ATTRIBUTEID_MS_ILLUM_MEASURED:
                    msg_data = (ZcbAttribute_t *)malloc(sizeof(ZcbAttribute_t));
					if (msg_data)
					{
                    msg_data->u16ClusterID = u16ClusterID;
                    msg_data->u16AttributeID = u16AttributeID;
                    msg_data->u64Data = u64Data;
                    eZCB_SendMsg(BRIDGE_WRITE_ATTRIBUTE, &sZcb, msg_data);
					free(msg_data);
					msg_data=NULL;
					}
					else
						PRINTF("\n ### Fail to malloc buf !!!");
					break;
				default:
					break;
            }
		  break;	

		  	
		case E_ZB_CLUSTERID_OCCUPANCYSENSING:
			switch ( u16AttributeID ) 
			{
			   case E_ZB_ATTRIBUTEID_MS_OCC_OCCUPANCY:
                    msg_data = (ZcbAttribute_t *)malloc(sizeof(ZcbAttribute_t));
					if (msg_data)
					{
                    msg_data->u16ClusterID = u16ClusterID;
                    msg_data->u16AttributeID = u16AttributeID;
                    msg_data->u64Data = u64Data;
                    eZCB_SendMsg(BRIDGE_WRITE_ATTRIBUTE, &sZcb, msg_data);	
					free(msg_data);
					msg_data=NULL;
					}
					else
						PRINTF("\n ### Fail to malloc buf !!!");
					break;
				default:
					break;			   	
			}
			break;
			
	    case E_ZB_CLUSTERID_MEASUREMENTSENSING_TEMP:
            switch ( u16AttributeID ) 
			{
            case E_ZB_ATTRIBUTEID_MS_TEMP_MEASURED:
                {
                    msg_data = (ZcbAttribute_t *)malloc(sizeof(ZcbAttribute_t));
					if (msg_data)
					{
                    msg_data->u16ClusterID = u16ClusterID;
                    msg_data->u16AttributeID = u16AttributeID;
                    msg_data->u64Data = u64Data;
                    eZCB_SendMsg(BRIDGE_WRITE_ATTRIBUTE, &sZcb, msg_data);
					free(msg_data);
					msg_data=NULL;					
					}
					else
						PRINTF("\n ### Fail to malloc buf !!!");
                }
                break;

            default:
         //       printf( "Received attribute 0x%04x in MEASUREMENTSENSING_TEMP cluster\n", u16AttributeID );
                break;
            }
            break;

        default:
        //    printf( "Received attribute 0x%04x in cluster 0x%04x\n",u16AttributeID, u16ClusterID );
            break;
        }
    }
}

void ZCB_HandleAttributeReport(uint16_t u16ShortAddress, uint8_t u8Endpoint, uint16_t u16ClusterID, 
                            uint16_t u16AttributeID, uint8_t u8AttributeStatus, uint8_t u8Type, 
                            uint16_t u16SizeOfAttributesInBytes, uint64_t u64Data) 
{    
    tsZbDeviceInfo * sDevice = tZDM_FindDeviceByNodeId(u16ShortAddress);
    if (sDevice == NULL) {
        eIeeeAddressRequest(u16ShortAddress, u16ShortAddress, 0, 0);
        return;
    }

    if (sDevice->eDeviceState == E_ZB_DEVICE_STATE_OFF_LINE) {
        sDevice->eDeviceState = E_ZB_DEVICE_STATE_ACTIVE;
    }
    
    tsZbDeviceAttribute *sAttribute = tZDM_FindAttributeEntryByElement(u16ShortAddress,
                                                                       u8Endpoint,
                                                                       u16ClusterID,
                                                                       u16AttributeID);
    if (sAttribute == NULL)
        return; 

    handleAttribute(u16ShortAddress, u16ClusterID,
                    u16AttributeID, u64Data, u8Endpoint);
}

void ZCB_HandleActiveEndPointResp(uint16_t u16NwkAddrOfInterest, uint8_t u8ActiveEpCount, uint8_t* pu8ActiveEpList) 
{
    PRINTF("ZCB_HandleActiveEndPointResp\r\n");
    tsZbDeviceInfo* sDevice = tZDM_FindDeviceByNodeId(u16NwkAddrOfInterest);
    if (sDevice == NULL) {
        return; 
    }
    
    if (sDevice->eDeviceState != E_ZB_DEVICE_STATE_ACTIVE) {
        sDevice->u8EndpointCount = u8ActiveEpCount;
    //    LOG(ZCB, INFO, "ActiveEpRsp: -EpList: ");
        for (uint8_t i = 0; i < sDevice->u8EndpointCount; i++) {
            sDevice->sZDEndpoint[i].u8EndpointId = pu8ActiveEpList[i];
    //        LOG(ZCB, INFO, "%d,", sDevice->sZDEndpoint[i].u8EndpointId);
        }
  //      LOG(ZCB, INFO, "\r\n");
        sDevice->eDeviceState = E_ZB_DEVICE_STATE_GET_CLUSTER;
        vZDM_NewDeviceQualifyProcess(sDevice);
    }  
}

void ClearClusterBitmap(void)
{
	sZcb.uSupportedClusters.sClusterBitmap.hasOnOff=0;
	sZcb.uSupportedClusters.sClusterBitmap.hasDimmable=0;
	sZcb.uSupportedClusters.sClusterBitmap.hasColor=0;
	sZcb.uSupportedClusters.sClusterBitmap.hasIlluminanceSensing=0;
	sZcb.uSupportedClusters.sClusterBitmap.hasTemperatureSensing=0;
	sZcb.uSupportedClusters.sClusterBitmap.hasOccupancySensing=0;
	return ;
}
void ZCB_HandleSimpleDescriptorResponse(uint16_t u16NwkAddrOfInterest, 
                                          uint8_t u8EndPoint,
                                          uint16_t u16ApplicationProfileId,
                                          uint16_t u16DeviceId,
                                          uint8_t u8Value,
                                          uint8_t u8InClusterCount,
                                          uint8_t u8OutClusterCount,
                                          uint16_t au16Data[34])
{
    PRINTF( "ZCB_HandleSimpleDescriptorResponse\r\n" );
    tsZbDeviceEndPoint * devEp;
	uint8_t   u8Cluster;

 //   LOG(ZCB, INFO, "SimpleRsp: addr = 0x%04x, ep = %d, devId = 0x%04x\r\n", u16ShortAddress, u8EndPoint, u16DeviceId);
    
    tsZbDeviceInfo *sDevice = tZDM_FindDeviceByNodeId(u16NwkAddrOfInterest);
    if (sDevice->eDeviceState != E_ZB_DEVICE_STATE_ACTIVE) {
        devEp = tZDM_FindEndpointEntryInDeviceTable(sDevice->u16NodeId, u8EndPoint);
        devEp->u16DeviceType  = u16DeviceId;
        uint8_t actualClusCnt = 0;
        uint16_t tempClusterId = 0;
        for (uint8_t i = 0; i < u8InClusterCount; i++) {
            tempClusterId = au16Data[i];
            if ((tempClusterId != E_ZB_CLUSTERID_GROUPS)
                && (tempClusterId != E_ZB_CLUSTERID_SCENES)
                && (tempClusterId != E_ZB_CLUSTERID_IDENTIFY)
                && (tempClusterId != E_ZB_CLUSTERID_ZLL_COMMISIONING)
                && (tempClusterId != 0xFFFF)) {
                devEp->sZDCluster[actualClusCnt++].u16ClusterId = tempClusterId;
            }
        }
        devEp->u8ClusterCount = actualClusCnt;
		ClearClusterBitmap();
		for (u8Cluster = 0;	u8Cluster < actualClusCnt;u8Cluster++)
		{
		  if (devEp->sZDCluster[u8Cluster].u16ClusterId==E_ZB_CLUSTERID_ONOFF)
		  {
		//	  PRINTF("\n ### Cluster is OnOff ");
			  sZcb.uSupportedClusters.sClusterBitmap.hasOnOff = 1;
		  }	
		  else if (devEp->sZDCluster[u8Cluster].u16ClusterId==E_ZB_CLUSTERID_LEVEL_CONTROL)
		  {
		//	  PRINTF("\n ### Cluster is Level Control ");
			  sZcb.uSupportedClusters.sClusterBitmap.hasDimmable = 1;
			  JoinedNodes[idx].type=2;
		  }
		  else if (devEp->sZDCluster[u8Cluster].u16ClusterId==E_ZB_CLUSTERID_COLOR_CONTROL)
		  {
		//	  PRINTF("\n ### Cluster is Color Control ");
			  sZcb.uSupportedClusters.sClusterBitmap.hasColor = 1;
			  JoinedNodes[idx].type=3;
		  }
		  else if (devEp->sZDCluster[u8Cluster].u16ClusterId==E_ZB_CLUSTERID_MEASUREMENTSENSING_ILLUM)
		  {
   		//      PRINTF("\n ### Cluster is Illuminance Measurement ");
			  sZcb.uSupportedClusters.sClusterBitmap.hasIlluminanceSensing=1;
		  }
		  else if (devEp->sZDCluster[u8Cluster].u16ClusterId==E_ZB_CLUSTERID_MEASUREMENTSENSING_TEMP)
		  {
   		//      PRINTF("\n ### Cluster is Temperature Measurement ");
		      sZcb.uSupportedClusters.sClusterBitmap.hasTemperatureSensing=1;
		  }
		  else if (devEp->sZDCluster[u8Cluster].u16ClusterId==E_ZB_CLUSTERID_OCCUPANCYSENSING)
		  {
		//	  PRINTF("\n ### Cluster is Occupancy Sensing ");
			  sZcb.uSupportedClusters.sClusterBitmap.hasOccupancySensing=1;
		  }
		}
#if 0 //force to EP 1 in case EP 2 is GP (0xF2)
        if (u8EndPoint == sDevice->sZDEndpoint[sDevice->u8EndpointCount - 1].u8EndpointId) 
#else
		if (u8EndPoint == sDevice->sZDEndpoint[0].u8EndpointId) 
#endif
		{
           sDevice->eDeviceState = E_ZB_DEVICE_STATE_READ_ATTRIBUTE;            
        } else {
            sDevice->eDeviceState = E_ZB_DEVICE_STATE_GET_CLUSTER;
        }
        vZDM_NewDeviceQualifyProcess(sDevice);

	eZCB_SendMsg(BRIDGE_ADD_DEV,&sZcb,NULL);
	
    }
} 

void ZCB_HandleReadAttrResp(uint16_t u16ShortAddress, uint8_t u8EndPoint, uint16_t u16ClusterId, 
                            uint16_t u16AttributeId, uint8_t u8AttributeStatus, uint8_t u8AttributeType, 
                            uint16_t u16SizeOfAttributesInBytes, uint8_t auAttributeValue[50])
{
    PRINTF("ZCB_HandleReadAttrResp\r\n");

    tsZbDeviceAttribute *sAttribute = tZDM_FindAttributeEntryByElement(u16ShortAddress, u8EndPoint, u16ClusterId, u16AttributeId);

    if (sAttribute == NULL) {
        return;
    }
    
    sAttribute->u8DataType = u8AttributeType;;
    switch (sAttribute->u8DataType)
    {
        case(E_ZCL_GINT8):
        case(E_ZCL_UINT8):
        case(E_ZCL_INT8):
        case(E_ZCL_ENUM8):
        case(E_ZCL_BMAP8):
        case(E_ZCL_BOOL):
        {
            uint8_t u8Data;
            memcpy(&u8Data, auAttributeValue, sizeof(uint8_t));
            sAttribute->uData.u64Data = (uint64_t)u8Data;
        }
            break;
            
        case(E_ZCL_STRUCT):
        case(E_ZCL_INT16):
        case(E_ZCL_UINT16):
        case(E_ZCL_ENUM16):
        case(E_ZCL_CLUSTER_ID):
        case(E_ZCL_ATTRIBUTE_ID):
        {
            uint16_t u16Data;
            memcpy(&u16Data, auAttributeValue, sizeof(uint16_t));
            sAttribute->uData.u64Data = (uint64_t)pri_ntohs(u16Data);
        }
            break;
            
        case(E_ZCL_UINT24):
        case(E_ZCL_UINT32):
        case(E_ZCL_TOD):
        case(E_ZCL_DATE):
        case(E_ZCL_UTCT):
        case(E_ZCL_BACNET_OID):
        {
            uint32_t u32Data;
            memcpy(&u32Data, auAttributeValue, sizeof(uint32_t));
            sAttribute->uData.u64Data = (uint64_t)pri_ntohl(u32Data);
        }
            break;
            
        case(E_ZCL_UINT40):
        case(E_ZCL_UINT48):
        case(E_ZCL_UINT56):
        case(E_ZCL_UINT64):
        case(E_ZCL_IEEE_ADDR):
        {
            uint64_t u64Data;
            memcpy(&u64Data, auAttributeValue, sizeof(uint64_t));
            sAttribute->uData.u64Data = pri_ntohd(u64Data);
        }
            break;
                    
        case E_ZCL_OSTRING:
        case E_ZCL_CSTRING:
            sAttribute->uData.sData.u8Length = (uint8_t)u16SizeOfAttributesInBytes;
            if (sAttribute->uData.sData.pData == NULL) {
                sAttribute->uData.sData.pData = pvPortMalloc(sizeof(uint8_t) * (sAttribute->uData.sData.u8Length + 1));
            }      
            memcpy(sAttribute->uData.sData.pData, auAttributeValue, sizeof(uint8_t) * sAttribute->uData.sData.u8Length);
            sAttribute->uData.sData.pData[sAttribute->uData.sData.u8Length] = '\0';
            break;
            
        case E_ZCL_LCSTRING:
        case E_ZCL_LOSTRING:
            break;
            
        default:
            break;
    }
    
    if((sAttribute->u8DataType == E_ZCL_OSTRING) || (sAttribute->u8DataType == E_ZCL_CSTRING))        
          ;//        LOG(ZCB, INFO, "attr value = %s\r\n", sAttribute->uData.sData.pData);
    else
         ;//       LOG(ZCB, INFO, "attr value = %d\r\n", sAttribute->uData.u64Data);

    tsZbDeviceInfo *sDevice = tZDM_FindDeviceByNodeId(u16ShortAddress);
    if (sDevice->eDeviceState != E_ZB_DEVICE_STATE_ACTIVE) {
        if ((u16ClusterId == E_ZB_CLUSTERID_BASIC) && (u16AttributeId == E_ZB_ATTRIBUTEID_BASIC_MODEL_ID)) {
            sDevice->eDeviceState = E_ZB_DEVICE_STATE_BIND_CLUSTER;
            vZDM_NewDeviceQualifyProcess(sDevice);
        }
    }
}

void ZCB_HandleNetworkAddressReponse (uint64_t u64IeeeAddrRemoteDev, uint16_t u16NwkAddrRemoteDev)
{
    PRINTF("ZCB_HandleNetworkAddressRsp\r\n");
    tsZbDeviceInfo *sDevice = tZDM_FindDeviceByIeeeAddress(u64IeeeAddrRemoteDev);
    if (sDevice == NULL)
        return;
    
    sDevice->u64IeeeAddress = u16NwkAddrRemoteDev;   
}

void ZCB_HandleIeeeAddressReponse (uint64_t u64IeeeAddrRemoteDev, uint16_t u16NwkAddrRemoteDev)
{
    PRINTF("ZCB_HandleIeeeAddressRsp\r\n");

    tsZbDeviceInfo *sDevice = tZDM_FindDeviceByIeeeAddress(u64IeeeAddrRemoteDev);
    if (sDevice == NULL)
        return;
    sDevice->u16NodeId = u16NwkAddrRemoteDev;
}

uint8_t FindMatchedNodeByEP(uint16_t ep)
{
	uint8_t i;

	for (i=0;i<DEV_NUM;i++)
	{
		  if ((JoinedNodes[i].type)&&(JoinedNodes[i].ep==ep))
			break;
	}
	if (i==DEV_NUM)
	{
		PRINTF("\n ### Fail to find the Node with its EP Matches the required EP\n");
		return 0xff;
	}	
	else
		return i;
}

void BridgedOnOff(uint16_t ep,uint8_t mode)
{
	uint8_t i;
	if ((i=FindMatchedNodeByEP(ep))!=0xff)
	{
		PRINTF("\n ### Send On/Off/Toggle to 0x%x with Mode:%d at EP=%d\n",JoinedNodes[i].shortaddr,mode,ep);		
		eOn_Off(JoinedNodes[i].shortaddr, mode); 
	}	
}

void BridgedLevelControl(uint16_t ep,uint8_t level,uint16_t time)
{
	uint8_t i;
	if ((i=FindMatchedNodeByEP(ep))!=0xff)
	{
		PRINTF("\n ### Send MoveToLevel to 0x%x with Level:%d,TransTime:%d at EP=%d\n",JoinedNodes[i].shortaddr,level,time,ep);		
		eLevelControlMoveToLevel(JoinedNodes[i].shortaddr,level,time); 
	}	
}

void BridgedMoveToHue(uint16_t ep,uint8_t hue,uint8_t dir,uint16_t time)
{
	uint8_t i;
	if ((i=FindMatchedNodeByEP(ep))!=0xff)
	{
		PRINTF("\n ### Send MoveToHue to 0x%x with Hue:%d,Dir:%d,TransTime:%d at EP=%d\n",JoinedNodes[i].shortaddr,hue,dir,time,ep);		
		eColorControlMoveToHue(JoinedNodes[i].shortaddr,hue,dir,time); 
	}	
}

void BridgedMoveToSaturation(uint16_t ep,uint8_t sat,uint16_t time)
{
	uint8_t i;
	if ((i=FindMatchedNodeByEP(ep))!=0xff)
	{
		PRINTF("\n ### Send MoveToSaturation to 0x%x with Sat:%d,TransTime:%d at EP=%d\n",JoinedNodes[i].shortaddr,sat,time,ep);		
		eColorControlMoveToSaturation(JoinedNodes[i].shortaddr,sat,time); 
	}
}

void BridgedMoveToColorTemperature(uint16_t ep,uint16_t temp,uint16_t time)
{
	uint8_t i;

	if ((i=FindMatchedNodeByEP(ep))!=0xff)
	{
		PRINTF("\n ### Send MoveToTemperature to 0x%x with Temp:%d,TransTime:%d at EP=%d\n",JoinedNodes[i].shortaddr,temp,time,ep);		
		eColorControlMoveToTemp(JoinedNodes[i].shortaddr,temp,time); 
	}	
}

void BridgedMoveToColor(uint16_t ep,uint16_t x,uint16_t y,uint16_t time)
{
	uint8_t i;
	if ((i=FindMatchedNodeByEP(ep))!=0xff)
	{
		PRINTF("\n ### Send MoveToColor to 0x%x with ColorX:%d,ColorY:%d,TransTime:%d at EP=%d\n",JoinedNodes[i].shortaddr,x,y,time,ep);		
		eColorControlMoveToColor(JoinedNodes[i].shortaddr,x,y,time); 
	}	
}
// ------------------------------------------------------------------
// END OF FILE
// ------------------------------------------------------------------
