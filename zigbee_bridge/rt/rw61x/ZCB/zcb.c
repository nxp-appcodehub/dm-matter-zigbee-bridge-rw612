/*
 * Copyright 2021-2023 NXP
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "ZigbeeConstant.h"
#include "SerialLink.h"
#include "ZigbeeDevices.h"
#include "cmd.h"
#include "zcb.h"

#include "ram_storage.h" 

#include "zigbee_cmd.h"

#include "ZcbMessage.h"

#include "CHIPProjectAppConfig.h"

#define ZB_DEVICE_MESSAGE_TIMER_OUT_COUNT    5

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
static void ZCB_HandleVersionResponse           (void *pvUser, uint16_t u16Length, void *pvMessage);
static void ZCB_HandleNodeClusterList           (void *pvUser, uint16_t u16Length, void *pvMessage);
static void ZCB_HandleNodeClusterAttributeList  (void *pvUser, uint16_t u16Length, void *pvMessage);
static void ZCB_HandleNodeCommandIDList         (void *pvUser, uint16_t u16Length, void *pvMessage);
static void ZCB_HandleNetworkJoined             (void *pvUser, uint16_t u16Length, void *pvMessage);
static void ZCB_HandleDeviceAnnounce            (void *pvUser, uint16_t u16Length, void *pvMessage);
static void ZCB_HandleDeviceLeave               (void *pvUser, uint16_t u16Length, void *pvMessage);
static void ZCB_HandleMatchDescriptorResponse   (void *pvUser, uint16_t u16Length, void *pvMessage);
static void ZCB_HandleAttributeReport           (void *pvUser, uint16_t u16Length, void *pvMessage);
static void ZCB_HandleSimpleDescriptorResponse  (void *pvUser, uint16_t u16Length, void *pvMessage);
static void ZCB_HandleDefaultResponse           (void *pvUser, uint16_t u16Length, void *pvMessage);
static void ZCB_HandleReadAttrResp              (void *pvUser, uint16_t u16Length, void *pvMessage);
static void ZCB_HandleActiveEndPointResp        (void *pvUser, uint16_t u16Length, void *pvMessage);
static void ZCB_HandleLog                       (void *pvUser, uint16_t u16Length, void *pvMessage);
static void ZCB_HandleIASZoneStatusChangeNotify (void *pvUser, uint16_t u16Length, void *pvMessage); 
static void ZCB_HandleNetworkAddressReponse     (void *pvUser, uint16_t u16Length, void *pvMessage);
static void ZCB_HandleIeeeAddressReponse        (void *pvUser, uint16_t u16Length, void *pvMessage);
static void ZCB_HandleOtaBlockRequest           (void *pvUser, uint16_t u16Length, void *pvMessage);
static void ZCB_HandleOtaUpgradeEndRequest      (void *pvUser, uint16_t u16Length, void *pvMessage);
static void ZCB_HandleGetPermitResponse         (void *pvUser, uint16_t u16Length, void *pvMessage);
static void ZCB_HandleRestartProvisioned        (void *pvUser, uint16_t u16Length, void *pvMessage);
static void ZCB_HandleRestartFactoryNew         (void *pvUser, uint16_t u16Length, void *pvMessage);

static void eDeviceTimer_Init();
static void vDevTimerCallback(TimerHandle_t xTimers);
tsZbDeviceMsgTimer deviceTimer[MAX_ZD_DEVICE_NUMBERS];

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

    if (E_SL_OK != eSL_Init())
    {
        PRINTF("\n ZCB Initial Failed\n");
     //   LOG(ZCB, ERR, "ZCB Initial Failed!\r\n");
    }

    vZbDeviceTable_Init();
    
    /* Register listeners */
    eSL_AddListener(E_SL_MSG_VERSION_LIST,               ZCB_HandleVersionResponse,          NULL);
    eSL_AddListener(E_SL_MSG_NODE_CLUSTER_LIST,          ZCB_HandleNodeClusterList,          NULL);
    eSL_AddListener(E_SL_MSG_NODE_ATTRIBUTE_LIST,        ZCB_HandleNodeClusterAttributeList, NULL);
    eSL_AddListener(E_SL_MSG_NODE_COMMAND_ID_LIST,       ZCB_HandleNodeCommandIDList,        NULL);
    eSL_AddListener(E_SL_MSG_NETWORK_JOINED_FORMED,      ZCB_HandleNetworkJoined,            NULL);
    eSL_AddListener(E_SL_MSG_DEVICE_ANNOUNCE,            ZCB_HandleDeviceAnnounce,           NULL);
    eSL_AddListener(E_SL_MSG_LEAVE_INDICATION,           ZCB_HandleDeviceLeave,              NULL);
    eSL_AddListener(E_SL_MSG_MATCH_DESCRIPTOR_RESPONSE,  ZCB_HandleMatchDescriptorResponse,  NULL);
    eSL_AddListener(E_SL_MSG_ATTRIBUTE_REPORT,           ZCB_HandleAttributeReport,          NULL);
    eSL_AddListener(E_SL_MSG_SIMPLE_DESCRIPTOR_RESPONSE, ZCB_HandleSimpleDescriptorResponse, NULL);
    eSL_AddListener(E_SL_MSG_DEFAULT_RESPONSE,           ZCB_HandleDefaultResponse,          NULL);
    eSL_AddListener(E_SL_MSG_READ_ATTRIBUTE_RESPONSE,    ZCB_HandleReadAttrResp,             NULL);
    eSL_AddListener(E_SL_MSG_ACTIVE_ENDPOINT_RESPONSE,   ZCB_HandleActiveEndPointResp,       NULL);
    eSL_AddListener(E_SL_MSG_LOG,                        ZCB_HandleLog,                      NULL);
    eSL_AddListener(E_SL_MSG_IAS_ZONE_STATUS_CHANGE_NOTIFY, ZCB_HandleIASZoneStatusChangeNotify, NULL);
    eSL_AddListener(E_SL_MSG_NETWORK_ADDRESS_RESPONSE,   ZCB_HandleNetworkAddressReponse,    NULL);
    eSL_AddListener(E_SL_MSG_IEEE_ADDRESS_RESPONSE,      ZCB_HandleIeeeAddressReponse,       NULL);
 //   eSL_AddListener(E_SL_MSG_BLOCK_REQUEST,              ZCB_HandleOtaBlockRequest,          NULL); 
    eSL_AddListener(E_SL_MSG_UPGRADE_END_REQUEST,        ZCB_HandleOtaUpgradeEndRequest,     NULL);
    eSL_AddListener(E_SL_MSG_GET_PERMIT_JOIN_RESPONSE,   ZCB_HandleGetPermitResponse,        NULL);
    eSL_AddListener(E_SL_MSG_RESTART_PROVISIONED,        ZCB_HandleRestartProvisioned,       NULL);
    eSL_AddListener(E_SL_MSG_RESTART_FACTORY_NEW,        ZCB_HandleRestartFactoryNew,        NULL);
	
    /* Release reset pin to start Zigbee */
    GPIO_PinWrite(GPIO, 1, 55-32, 1);
	
	for (i=0;i<DEV_NUM;i++)
		JoinedNodes[i].type=JoinedNodes[i].ep=JoinedNodes[i].shortaddr=JoinedNodes[i].mac=0;
	
    /* only register Zigbee Cmd after */
   // ZigBeeCmdRegister();

    /* Create the device timers */
    eDeviceTimer_Init();
}

static void eDeviceTimer_Init()
{
    for (uint8_t i = 0; i < MAX_ZD_DEVICE_NUMBERS; i++) {
        deviceTimer[i].xTimers = xTimerCreate( "Timer",
                                               pdMS_TO_TICKS(65000), //65s
                                               pdTRUE,&i,vDevTimerCallback);
        if (deviceTimer[i].xTimers == NULL) {
            PRINTF("\n deviceTimer[%d] create fail",i);
        }
    }
}

teZcbStatus eZCB_GetCoordinatorVersion(void) 
{    
 //   LOG(ZCB, INFO, "eZCB_GetCoordinatorVersion\r\n"); 
    
    if (eSL_SendMessage(E_SL_MSG_GET_VERSION, 0, NULL, NULL) == E_SL_OK)
    {        
        /* Wait 300ms for the version message to arrive */
        if (eSL_MessageWait(E_SL_MSG_VERSION_LIST, 300, NULL, NULL) == E_SL_OK) {
            return E_ZCB_OK;
        }
    }   
    return E_ZCB_COMMS_FAILED;
}

teZcbStatus eOnOff(uint8_t u8AddrMode,
                   uint16_t u16Addr,
                   uint8_t u8SrcEp,
                   uint8_t u8DstEp,
                   uint8_t u8Mode)
{
    uint8_t             u8SequenceNo;
    teSL_Status         eStatus;

    struct {
        uint8_t     u8TargetAddressMode;
        uint16_t    u16TargetAddress;
        uint8_t     u8SourceEndpoint;
        uint8_t     u8DestinationEndpoint;
        uint8_t     u8Mode;
    } PACKED sOnOffMessage;

//    LOG(ZCB, INFO, "On/Off (Set Mode=%d)\r\n", u8Mode);

    if (u8Mode > 2) {
        /* Illegal value */
        return E_ZCB_ERROR;
    }

    sOnOffMessage.u8TargetAddressMode   = u8AddrMode;
    sOnOffMessage.u16TargetAddress      = pri_ntohs(u16Addr);
    sOnOffMessage.u8SourceEndpoint      = 1;
    sOnOffMessage.u8DestinationEndpoint = 1;
    sOnOffMessage.u8Mode                = 2/*u8Mode*/; //toggle=2
    eStatus = eSL_SendMessage(E_SL_MSG_ONOFF, sizeof(sOnOffMessage),
        &sOnOffMessage, &u8SequenceNo);

    if (eStatus != E_SL_OK)
    {
  //      LOG(ZCB, ERR, "Sending of Command '%s' failed (0x%02x)\r\n",
    //        (u8Mode? "On" : "Off"),
     //       eStatus);
        return E_ZCB_COMMS_FAILED;
    }

    return E_ZCB_OK;
}

teZcbStatus eOn_Off( uint16_t u16ShortAddress, uint8_t u8Mode ) 
{
    uint8_t             u8SequenceNo;
    teSL_Status         eStatus;

    struct {
        uint8_t     u8TargetAddressMode;
        uint16_t    u16TargetAddress;
        uint8_t     u8SourceEndpoint;
        uint8_t     u8DestinationEndpoint;
        uint8_t     u8Mode;
    } __attribute__((__packed__)) sOnOffMessage;

    if (u8Mode > 2) {
        /* Illegal value */
        return E_ZCB_ERROR;
    }

    sOnOffMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT/* E_ZB_ADDRESS_MODE_SHORT_NO_ACK*/;
    sOnOffMessage.u16TargetAddress      = pri_ntohs(u16ShortAddress);
    sOnOffMessage.u8SourceEndpoint      = 1;
    sOnOffMessage.u8DestinationEndpoint = 1;

    sOnOffMessage.u8Mode = u8Mode;
    eStatus = eSL_SendMessage(E_SL_MSG_ONOFF, sizeof(sOnOffMessage),
        &sOnOffMessage, &u8SequenceNo);

    if (eStatus != E_SL_OK)
    {
      PRINTF( "\n ### Sending of Command '%s' failed (0x%02x)\n",(u8Mode? "On" : "Off"),eStatus);
      return E_ZCB_COMMS_FAILED;
    }

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
    uint8_t             u8SequenceNo;
    teSL_Status         eStatus;

    struct {
        uint8_t     u8TargetAddressMode;
        uint16_t    u16TargetAddress;
        uint8_t     u8SourceEndpoint;
        uint8_t     u8DestinationEndpoint;
        uint8_t     u8OnOff;
        uint8_t     u8Mode;
        uint8_t     u8Rate;
    } PACKED sLevelControlMoveMessage;

    sLevelControlMoveMessage.u8TargetAddressMode   = u8AddrMode;
    sLevelControlMoveMessage.u16TargetAddress      = pri_ntohs(u16Addr);
    sLevelControlMoveMessage.u8SourceEndpoint      = u8SrcEp;
    sLevelControlMoveMessage.u8DestinationEndpoint = u8DstEp;
    sLevelControlMoveMessage.u8OnOff               = u8OnOff;
    sLevelControlMoveMessage.u8Mode                = u8Mode;
    sLevelControlMoveMessage.u8Rate                = u8Rate;
    
    eStatus = eSL_SendMessage(E_SL_MSG_MOVE_TO_LEVEL, sizeof(sLevelControlMoveMessage),
        &sLevelControlMoveMessage, &u8SequenceNo);

    if (eStatus != E_SL_OK)
    {
    //    LOG(ZCB, ERR, "Sending of Command '%s' failed (0x%02x)\r\n",
      //      "LevelControlMove",
       //     eStatus);
        return E_ZCB_COMMS_FAILED;
    }

    return E_ZCB_OK;
}


teZcbStatus eLevelControlMoveToLevel(uint16_t u16Addr, 
                                     //uint8_t u8OnOff,
                                     uint8_t u8Level,
                                     uint16_t u16Time)
{
    uint8_t             u8SequenceNo;
    teSL_Status         eStatus;

    struct {
        uint8_t     u8TargetAddressMode;
        uint16_t    u16TargetAddress;
        uint8_t     u8SourceEndpoint;
        uint8_t     u8DestinationEndpoint;
        uint8_t     u8WithOnOffOrNot;  //0: Without OnOff
        uint8_t     u8MoveToLevel;
        uint16_t    u16TransitionTime;
    } PACKED sLevelControlMoveToLevelMessage;

 //   LOG(ZCB, INFO, "LevelControl (Move to Level=%d)\r\n", u8Level);

    sLevelControlMoveToLevelMessage.u8TargetAddressMode   = 2;
    sLevelControlMoveToLevelMessage.u16TargetAddress      = pri_ntohs(u16Addr);
    sLevelControlMoveToLevelMessage.u8SourceEndpoint      = 1;
    sLevelControlMoveToLevelMessage.u8DestinationEndpoint = 1;
    sLevelControlMoveToLevelMessage.u8WithOnOffOrNot      = 1;//u8OnOff;  
    sLevelControlMoveToLevelMessage.u8MoveToLevel         = u8Level;
    sLevelControlMoveToLevelMessage.u16TransitionTime     = pri_ntohs(u16Time);
    
    eStatus = eSL_SendMessage(E_SL_MSG_MOVE_TO_LEVEL_ONOFF, sizeof(sLevelControlMoveToLevelMessage),
        &sLevelControlMoveToLevelMessage, &u8SequenceNo);

    if (eStatus != E_SL_OK)
    {
//        LOG(ZCB, ERR, "Sending of Command '%s' failed (0x%02x)\r\n",
  //          "LevelControlMoveToLevel",
    //        eStatus);
        return E_ZCB_COMMS_FAILED;
    }

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
    uint8_t             u8SequenceNo;
    teSL_Status         eStatus;

    struct {
        uint8_t     u8TargetAddressMode;
        uint16_t    u16TargetAddress;
        uint8_t     u8SourceEndpoint;
        uint8_t     u8DestinationEndpoint;
        uint8_t     u8WithOnOffOrNot;  //0: Without OnOff
        uint8_t     u8StepMode;
        uint8_t     u8StepSize;
        uint16_t    u16TransitionTime;
    } PACKED sLevelControlMoveStepMessage;

  //  LOG(ZCB, INFO, "LevelControl (Move Step=%d, size=%d)\r\n", u8Mode, u8Size);

    sLevelControlMoveStepMessage.u8TargetAddressMode   = u8AddrMode;
    sLevelControlMoveStepMessage.u16TargetAddress      = pri_ntohs(u16Addr);
    sLevelControlMoveStepMessage.u8SourceEndpoint      = u8SrcEp;
    sLevelControlMoveStepMessage.u8DestinationEndpoint = u8DstEp;
    sLevelControlMoveStepMessage.u8WithOnOffOrNot      = u8OnOff;  
    sLevelControlMoveStepMessage.u8StepMode            = u8Mode;
    sLevelControlMoveStepMessage.u8StepSize            = u8Size;
    sLevelControlMoveStepMessage.u16TransitionTime     = pri_ntohs(u16Time);
    
    eStatus = eSL_SendMessage(E_SL_MSG_MOVE_STEP, sizeof(sLevelControlMoveStepMessage),
        &sLevelControlMoveStepMessage, &u8SequenceNo);

    if (eStatus != E_SL_OK)
    {
   //     LOG(ZCB, ERR, "Sending of Command '%s' failed (0x%02x)\r\n",
    //        "LevelControlMoveStep",
    //        eStatus);
        return E_ZCB_COMMS_FAILED;
    }

    return E_ZCB_OK;

}


teZcbStatus eColorControlMoveToColor(uint16_t u16Addr, 
									 uint16_t u16ColorX,
									 uint16_t u16ColorY,
                                     uint16_t u16Time)
{
    uint8_t             u8SequenceNo;
    teSL_Status         eStatus;

    struct {
        uint8_t     u8TargetAddressMode;
        uint16_t    u16TargetAddress;
        uint8_t     u8SourceEndpoint;
        uint8_t     u8DestinationEndpoint;
        uint16_t    u16MoveToColorX;
        uint16_t    u16MoveToColorY;
        uint16_t    u16TransitionTime;
    } PACKED sColorControlMoveToColorMessage;

//    LOG(ZCB, INFO, "ColorControl (Move to ColorX=%d, ColorY=%d)\r\n", u8ColorX, u8ColorY);

    sColorControlMoveToColorMessage.u8TargetAddressMode   = 2;
    sColorControlMoveToColorMessage.u16TargetAddress      = pri_ntohs(u16Addr);
    sColorControlMoveToColorMessage.u8SourceEndpoint      = 1;
    sColorControlMoveToColorMessage.u8DestinationEndpoint = 1;
    sColorControlMoveToColorMessage.u16MoveToColorX       = pri_ntohs(u16ColorX);
    sColorControlMoveToColorMessage.u16MoveToColorY       = pri_ntohs(u16ColorY);
    sColorControlMoveToColorMessage.u16TransitionTime     = pri_ntohs(u16Time);
    
    eStatus = eSL_SendMessage(E_SL_MSG_MOVE_TO_COLOUR, sizeof(sColorControlMoveToColorMessage),
        &sColorControlMoveToColorMessage, &u8SequenceNo);

    if (eStatus != E_SL_OK)
    {
  //      LOG(ZCB, ERR, "Sending of Command '%s' failed (0x%02x)\r\n",
    //        "ColorControlMoveToColor",
    //        eStatus);
        return E_ZCB_COMMS_FAILED;
    }
    return E_ZCB_OK;
}


teZcbStatus eColorControlMoveToTemp(uint16_t u16Addr, 
                                    uint16_t u16ColorTemp,
                                    uint16_t u16Time)
{
		uint8_t 			u8SequenceNo;
		teSL_Status 		eStatus;
	
		struct {
			uint8_t 	u8TargetAddressMode;
			uint16_t	u16TargetAddress;
			uint8_t 	u8SourceEndpoint;
			uint8_t 	u8DestinationEndpoint;
			uint16_t	u16MoveToColorTemp;
			uint16_t	u16TransitionTime;
		} PACKED sColorControlMoveToTempMessage;
	
	 //   LOG(ZCB, INFO, "ColorControl (Move to ColorTemp=%d)\r\n", u8ColorTemp);
	
		sColorControlMoveToTempMessage.u8TargetAddressMode	 = 2;
		sColorControlMoveToTempMessage.u16TargetAddress 	 = pri_ntohs(u16Addr);
		sColorControlMoveToTempMessage.u8SourceEndpoint 	 = 1;
		sColorControlMoveToTempMessage.u8DestinationEndpoint = 1;
		sColorControlMoveToTempMessage.u16MoveToColorTemp	 = pri_ntohs(u16ColorTemp); //must revert 2 bytes !!!
		sColorControlMoveToTempMessage.u16TransitionTime	 = pri_ntohs(u16Time);
		
		eStatus = eSL_SendMessage(E_SL_MSG_MOVE_TO_COLOUR_TEMPERATURE, sizeof(sColorControlMoveToTempMessage),
			&sColorControlMoveToTempMessage, &u8SequenceNo);
	
		if (eStatus != E_SL_OK)
		{
	  //	  LOG(ZCB, ERR, "Sending of Command '%s' failed (0x%02x)\r\n",
	  //		  "ColorControlMoveToTemp",
	  //		  eStatus);
			return E_ZCB_COMMS_FAILED;
		}
	
		return E_ZCB_OK;
}

teZcbStatus eColorControlMoveToHue(uint16_t u16Addr, 
                                   uint8_t u8Hue,
                                   uint8_t u8Dir,
                                   uint16_t u16Time)
{
    uint8_t             u8SequenceNo;
    teSL_Status         eStatus;

    struct {
        uint8_t     u8TargetAddressMode;
        uint16_t    u16TargetAddress;
        uint8_t     u8SourceEndpoint;
        uint8_t     u8DestinationEndpoint;
        uint8_t     u8MoveToHue;
        uint8_t     u8Direction;
        uint16_t    u16TransitionTime;
    } PACKED sColorControlMoveToHueMessage;

 //   LOG(ZCB, INFO, "ColorControl (Move to ColorHue=%d, Direction=%d)\r\n", u8Hue, u8Dir);

    sColorControlMoveToHueMessage.u8TargetAddressMode   = 2;
    sColorControlMoveToHueMessage.u16TargetAddress      = pri_ntohs(u16Addr);
    sColorControlMoveToHueMessage.u8SourceEndpoint      = 1;
    sColorControlMoveToHueMessage.u8DestinationEndpoint = 1;
    sColorControlMoveToHueMessage.u8MoveToHue           = u8Hue;
    sColorControlMoveToHueMessage.u8Direction           = u8Dir;
    sColorControlMoveToHueMessage.u16TransitionTime     = pri_ntohs(u16Time);
    
    eStatus = eSL_SendMessage(E_SL_MSG_MOVE_TO_HUE, sizeof(sColorControlMoveToHueMessage),
        &sColorControlMoveToHueMessage, &u8SequenceNo);

    if (eStatus != E_SL_OK)
    {
  //      LOG(ZCB, ERR, "Sending of Command '%s' failed (0x%02x)\r\n",
   //         "ColorControlMoveToHue",
   //         eStatus);
        return E_ZCB_COMMS_FAILED;
    }

    return E_ZCB_OK;
}
teZcbStatus eColorControlMoveToSaturation(uint16_t u16Addr, 
                                   uint8_t u8Sat,
                                   uint16_t u16Time)
{
	   uint8_t			   u8SequenceNo;
	   teSL_Status		   eStatus;
	
	   struct {
		   uint8_t	   u8TargetAddressMode;
		   uint16_t    u16TargetAddress;
		   uint8_t	   u8SourceEndpoint;
		   uint8_t	   u8DestinationEndpoint;
		   uint8_t	   u8MoveToSat;
		   uint16_t    u16TransitionTime;
	   } PACKED sColorControlMoveToSatMessage;
	
	//	 LOG(ZCB, INFO, "ColorControl (Move to ColorHue=%d, Direction=%d)\r\n", u8Hue, u8Dir);
	
	   sColorControlMoveToSatMessage.u8TargetAddressMode   = 2;//ShortAddr
	   sColorControlMoveToSatMessage.u16TargetAddress	   = pri_ntohs(u16Addr);
	   sColorControlMoveToSatMessage.u8SourceEndpoint	   = 1;
	   sColorControlMoveToSatMessage.u8DestinationEndpoint = 1;
	   sColorControlMoveToSatMessage.u8MoveToSat		   = u8Sat;
	   sColorControlMoveToSatMessage.u16TransitionTime	   = pri_ntohs(u16Time);
	   
	   eStatus = eSL_SendMessage(E_SL_MSG_MOVE_TO_SATURATION, sizeof(sColorControlMoveToSatMessage),
		   &sColorControlMoveToSatMessage, &u8SequenceNo);
	
	   if (eStatus != E_SL_OK)
	   {
	 // 	 LOG(ZCB, ERR, "Sending of Command '%s' failed (0x%02x)\r\n",
	  //		 "ColorControlMoveToHue",
	  //		 eStatus);
		   return E_ZCB_COMMS_FAILED;
	   }
	   return E_ZCB_OK;
}

teZcbStatus eIASZoneEnrollResponse(uint8_t u8AddrMode, 
                                   uint16_t u16Addr, 
                                   uint8_t u8SrcEp, 
                                   uint8_t u8DstEp, 
                                   uint8_t u8EnrollRspCode,
                                   uint8_t u8ZoneId)
{
    uint8_t             u8SequenceNo;
    teSL_Status         eStatus;

    struct {
        uint8_t     u8TargetAddressMode;
        uint16_t    u16TargetAddress;
        uint8_t     u8SourceEndpoint;
        uint8_t     u8DestinationEndpoint;
        uint8_t     u8EnrollRspCode;
        uint16_t    u8IASZoneId;
    } PACKED sIASZoneEnrollRspMessage;

 //   LOG(ZCB, INFO, "IAS Zone Enroll Response (RspCode = %d, ZoneId=%d)\r\n", u8EnrollRspCode, u8ZoneId);

    sIASZoneEnrollRspMessage.u8TargetAddressMode   = u8AddrMode;
    sIASZoneEnrollRspMessage.u16TargetAddress      = pri_ntohs(u16Addr);
    sIASZoneEnrollRspMessage.u8SourceEndpoint      = u8SrcEp;
    sIASZoneEnrollRspMessage.u8DestinationEndpoint = u8DstEp;
    sIASZoneEnrollRspMessage.u8EnrollRspCode       = u8EnrollRspCode;
    sIASZoneEnrollRspMessage.u8IASZoneId           = u8ZoneId;
    
    eStatus = eSL_SendMessage(E_SL_MSG_SEND_IAS_ZONE_ENROLL_RSP, sizeof(sIASZoneEnrollRspMessage),
        &sIASZoneEnrollRspMessage, &u8SequenceNo);

    if (eStatus != E_SL_OK)
    {
  //      LOG(ZCB, ERR, "Sending of Command '%s' failed (0x%02x)\r\n",
   //         "IASZoneEnrollResponse",
    //        eStatus);
        return E_ZCB_COMMS_FAILED;
    }

    return E_ZCB_OK;    
}

// ------------------------------------------------------------------
// Handlers
// ------------------------------------------------------------------

static void ZCB_HandleVersionResponse(void *pvUser, uint16_t u16Length, void *pvMessage) 
{   
  //  LOG(ZCB, INFO, "ZCB_HandleVersionResponse\r\n" );

    struct _tsVersion {
        uint32_t    u32Version;
    } PACKED *psVersion = (struct _tsVersion *)pvMessage;
    
    psVersion->u32Version = psVersion->u32Version;

 //   LOG(ZCB, INFO, "Coordinator Version 0x%08X\r\n", psVersion->u32Version);
}

static void ZCB_HandleNodeClusterList(void *pvUser, uint16_t u16Length, void *pvMessage) 
{
  //  LOG(ZCB, INFO, "ZCB_HandleNodeClusterList\r\n" );

    struct _tsClusterList {
        uint8_t     u8Endpoint;
        uint16_t    u16ProfileID;
        uint16_t    au16ClusterList[255];
    } PACKED *psClusterList = (struct _tsClusterList *)pvMessage;
    
    psClusterList->u16ProfileID = pri_ntohs(psClusterList->u16ProfileID);
    
//    LOG(ZCB, INFO, "Cluster list for endpoint %d, profile ID 0x%04X\r\n",
 //               psClusterList->u8Endpoint, 
  //              psClusterList->u16ProfileID);
    
    int nClusters = ( u16Length - 3 ) / 2;
    int i;
    for ( i=0; i<nClusters; i++ ) {
     //   LOG(ZCB, INFO, "- ID 0x%04X\r\n", pri_ntohs( psClusterList->au16ClusterList[i] ) );
    }
}

static void ZCB_HandleNodeClusterAttributeList(void *pvUser, uint16_t u16Length, void *pvMessage) 
{
  //  LOG(ZCB, INFO, "ZCB_HandleNodeClusterAttributeList\r\n" );

    struct _tsClusterAttributeList {
        uint8_t     u8Endpoint;
        uint16_t    u16ProfileID;
        uint16_t    u16ClusterID;
        uint16_t    au16AttributeList[255];
    } PACKED *psClusterAttributeList = (struct _tsClusterAttributeList *)pvMessage;
    
    psClusterAttributeList->u16ProfileID = pri_ntohs(psClusterAttributeList->u16ProfileID);
    psClusterAttributeList->u16ClusterID = pri_ntohs(psClusterAttributeList->u16ClusterID);
    
  //  LOG(ZCB, INFO, "Cluster attribute list for endpoint %d, cluster 0x%04X, profile ID 0x%04X\r\n",
  //              psClusterAttributeList->u8Endpoint, 
  //              psClusterAttributeList->u16ClusterID,
  //              psClusterAttributeList->u16ProfileID);

}

static void ZCB_HandleNodeCommandIDList(void *pvUser, uint16_t u16Length, void *pvMessage)
{
 //   LOG(ZCB, INFO, "ZCB_HandleNodeCommandIDList\r\n" );

    struct _tsCommandIDList {
        uint8_t     u8Endpoint;
        uint16_t    u16ProfileID;
        uint16_t    u16ClusterID;
        uint8_t     au8CommandList[255];
    } PACKED *psCommandIDList = (struct _tsCommandIDList *)pvMessage;
    
    psCommandIDList->u16ProfileID = pri_ntohs(psCommandIDList->u16ProfileID);
    psCommandIDList->u16ClusterID = pri_ntohs(psCommandIDList->u16ClusterID);
    
  //  LOG(ZCB, INFO, "Command ID list for endpoint %d, cluster 0x%04X, profile ID 0x%04X\r\n",
  //              psCommandIDList->u8Endpoint, 
  //              psCommandIDList->u16ClusterID,
  //              psCommandIDList->u16ProfileID);
}

static void ZCB_HandleNetworkJoined(void *pvUser, uint16_t u16Length, void *pvMessage) 
{
 //   LOG(ZCB, INFO, "ZCB_HandleNetworkJoined\r\n" );

    struct _tsNetworkJoinedFormedShort {
        uint8_t     u8Status;
        uint16_t    u16ShortAddress;
        uint64_t    u64IEEEAddress;
        uint8_t     u8Channel;
    } PACKED *psMessageShort = (struct _tsNetworkJoinedFormedShort *)pvMessage;
    
    psMessageShort->u16ShortAddress = pri_ntohs(psMessageShort->u16ShortAddress);
    psMessageShort->u64IEEEAddress  = pri_ntohd(psMessageShort->u64IEEEAddress);

    if ((psMessageShort->u8Status == 1) && (psMessageShort->u16ShortAddress == 0x0000))
    {
        zbNetworkInfo.u64IeeeAddress = psMessageShort->u64IEEEAddress;
        zbNetworkInfo.u8Channel = psMessageShort->u8Channel;
        zbNetworkInfo.eNetworkState = E_ZB_NETWORK_STATE_NWK_FORMED;
  //      LOG(ZCB, INFO, "Zigbee Network formed on channel %d\r\n", psMessageShort->u8Channel);
  //      LOG(ZCB, INFO, "Control bridge address 0x%04X (0x%016llX)\r\n", 
  //                     psMessageShort->u16ShortAddress,
  //                     (unsigned long long int)psMessageShort->u64IEEEAddress);
    }
}

bool EnumJoinedNodes(void)
{
	uint8_t j;
	int ret=0;

	ret = ramStorageReadFromFlash(myDB_filename,(uint8_t *)&savedNodes,sizeof(savedNodes));
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
	
	ret = ramStorageReadFromFlash(myDB_filename,(uint8_t *)&savedNodes,sizeof(savedNodes));
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
	  ret = ramStorageSavetoFlash(myDB_filename,(uint8_t *)&savedNodes,sizeof(savedNodes));
	  if (ret)
		  PRINTF("\n ### ramStorageSavetoFlash=%d,Joined Nodes=%d\n",ret,savedNodes.totalNodes);
	  
	}
}

void RestoreJoinedNodes(void)
{
	uint8_t i;
	int ret=0;

	ret = ramStorageReadFromFlash(myDB_filename,(uint8_t *)&savedNodes,sizeof(savedNodes));
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

	ret=ramStorageReadFromFlash(myDB_filename,(uint8_t *)&savedNodes,sizeof(savedNodes));
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
				ret = ramStorageSavetoFlash(myDB_filename,(uint8_t *)&savedNodes,sizeof(savedNodes));
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
	ret = ramStorageSavetoFlash(myDB_filename,(uint8_t *)&savedNodes,sizeof(savedNodes));
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

static void ZCB_HandleDeviceAnnounce(void *pvUser, uint16_t u16Length, void *pvMessage) 
{
//    LOG(ZCB, INFO, "ZCB_HandleDeviceAnnounce\r\n" );
	uint8_t i;

    struct _tsDeviceAnnounce {
        uint16_t    u16ShortAddress;
        uint64_t    u64IEEEAddress;
        uint8_t     u8MacCapability;
    } PACKED *psMessage = (struct _tsDeviceAnnounce *)pvMessage;
    
    psMessage->u16ShortAddress  = pri_ntohs(psMessage->u16ShortAddress);
    psMessage->u64IEEEAddress   = pri_ntohd(psMessage->u64IEEEAddress);
    
	for (i=0;i<DEV_NUM;i++)
	{
		if ((JoinedNodes[i].type==0)&&(JoinedNodes[i].ep==0))
		{
			JoinedNodes[i].shortaddr=psMessage->u16ShortAddress;
			JoinedNodes[i].mac=psMessage->u64IEEEAddress;
			JoinedNodes[i].type=1;
			idx=i;
			break;
		}
	}	

    tsZbDeviceInfo* sDevice = NULL;
    if (tZDM_FindDeviceByIeeeAddress(psMessage->u64IEEEAddress) == NULL) {
        if ((sDevice = tZDM_AddNewDeviceToDeviceTable(psMessage->u16ShortAddress, psMessage->u64IEEEAddress)) != NULL) 
		{
            vZDM_NewDeviceQualifyProcess(sDevice);
        }
    }     
}

static void ZCB_HandleDeviceLeave(void *pvUser, uint16_t u16Length, void *pvMessage) 
{
	uint8_t i,j,Idx;
	int ret=0;

//    LOG(ZCB, INFO, "ZCB_HandleDeviceLeave\r\n" );

    struct _tDeviceLeaveIndication {
        uint64_t    u64IeeeAddr;
        uint8_t     u8RejoinStatus;
    } PACKED *psMessage = (struct _tDeviceLeaveIndication *)pvMessage;

    psMessage->u64IeeeAddr = pri_ntohd(psMessage->u64IeeeAddr);
	
	for (i=0;i<DEV_NUM;i++)
	{
		if (JoinedNodes[i].mac==psMessage->u64IeeeAddr)
		{
			PRINTF("\n ### %d: Remove Node : 0x%llx",i,psMessage->u64IeeeAddr);
			ret=ramStorageReadFromFlash(myDB_filename,(uint8_t *)&savedNodes,sizeof(savedNodes));
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
				  ret = ramStorageSavetoFlash(myDB_filename,(uint8_t *)&savedNodes,sizeof(savedNodes));
				  if (ret)
					  PRINTF("\n ### ramStorageSavetoFlash=%d,Joined Nodes=%d\n",ret,savedNodes.totalNodes);
				}
			  } 	
			}			
			JoinedNodes[i].type=JoinedNodes[i].ep=JoinedNodes[i].shortaddr=JoinedNodes[i].mac=0;
		}	
	}
	
    tsZbDeviceInfo *sDevice = tZDM_FindDeviceByIeeeAddress(psMessage->u64IeeeAddr);
    if (sDevice == NULL)
        return;
	
    sDevice->eDeviceState = E_ZB_DEVICE_STATE_LEFT;
    bZDM_EraseDeviceFromDeviceTable(psMessage->u64IeeeAddr);
}



static void ZCB_HandleMatchDescriptorResponse(void *pvUser, uint16_t u16Length, void *pvMessage) 
{
 //  LOG(ZCB, INFO, "ZCB_HandleMatchDescriptorResponse\r\n" );

    struct _tMatchDescriptorResponse {
        uint8_t     u8SequenceNo;
        uint8_t     u8Status;
        uint16_t    u16ShortAddress;
        uint8_t     u8NumEndpoints;
        uint8_t     au8Endpoints[255];
    } PACKED *psMatchDescriptorResponse = (struct _tMatchDescriptorResponse *)pvMessage;
    psMatchDescriptorResponse->u16ShortAddress = pri_ntohs(psMatchDescriptorResponse->u16ShortAddress);
    
//    LOG(ZCB, INFO, "Match descriptor request response from node 0x%04X - %d matching endpoints.\r\n",
//                psMatchDescriptorResponse->u16ShortAddress,
//                psMatchDescriptorResponse->u8NumEndpoints);
}

int nibble2num( char c ) {
    // Warning: input must be a hex-nibble character, else 0 is returned
    int val = 0;
    if      ( c >= '0' && c <= '9' ) val = c - '0';
    else if ( c >= 'A' && c <= 'F' ) val = c - 'A' + 10;
    else if ( c >= 'a' && c <= 'f' ) val = c - 'a' + 10;
    return( val );
}

uint64_t nibblestr2u64( char * nibblestr ) {
    uint64_t u64 = 0;
    int i = 0;
    while ( i < 16 && nibblestr[i] != '\0' ) {
        u64 = ( u64 << 4 ) + (uint64_t)nibble2num( nibblestr[i] );
        i++;
    }
    return( u64 );
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

#if 0
    if ( newDbGetZcbSaddr( u16ShortAddress, &zcb ) ) 
        u64IEEEAddress = nibblestr2u64( zcb.mac );

    if ( u64IEEEAddress ) 
#endif		
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

static void ZCB_HandleAttributeReport(void *pvUser, uint16_t u16Length, void *pvMessage) 
{    
    struct _tsAttributeReport {
        uint8_t     u8SequenceNo;
        uint16_t    u16ShortAddress;
        uint8_t     u8Endpoint;
        uint16_t    u16ClusterID;
        uint16_t    u16AttributeID;
        uint8_t     u8AttributeStatus;
        uint8_t     u8Type;
        uint16_t    u16SizeOfAttributesInBytes;
	union {
		uint8_t 	u8Data;
		uint16_t	u16Data;
		uint32_t	u32Data;
		uint64_t	u64Data;
	} uData;
    } PACKED *psMessage = (struct _tsAttributeReport *)pvMessage;

    uint64_t u64Data = 0;
	
    psMessage->u16ShortAddress  = pri_ntohs(psMessage->u16ShortAddress);
    psMessage->u16ClusterID     = pri_ntohs(psMessage->u16ClusterID);
    psMessage->u16AttributeID   = pri_ntohs(psMessage->u16AttributeID);
    
    tsZbDeviceInfo * sDevice = tZDM_FindDeviceByNodeId(psMessage->u16ShortAddress);
    if (sDevice == NULL) {
        eIeeeAddressRequest(psMessage->u16ShortAddress, psMessage->u16ShortAddress, 0, 0);
        return;
    }

    uint8_t index = uZDM_FindDevTableIndexByNodeId(sDevice->u16NodeId);
    xTimerReset(deviceTimer[index].xTimers, 0);
    deviceTimer[index].count = 0;

    if (sDevice->eDeviceState == E_ZB_DEVICE_STATE_OFF_LINE) {
        sDevice->eDeviceState = E_ZB_DEVICE_STATE_ACTIVE;
    }
    
    tsZbDeviceAttribute *sAttribute = tZDM_FindAttributeEntryByElement(psMessage->u16ShortAddress,
                                                                       psMessage->u8Endpoint,
                                                                       psMessage->u16ClusterID,
                                                                       psMessage->u16AttributeID);
    if (sAttribute == NULL)
        return; 

    switch(psMessage->u8Type) 
    {
        case(E_ZCL_GINT8):
        case(E_ZCL_UINT8):
        case(E_ZCL_INT8):
        case(E_ZCL_ENUM8):
        case(E_ZCL_BMAP8):
        case(E_ZCL_BOOL):
        {
			u64Data = (uint64_t )psMessage->uData.u8Data;
        }
            break;
            
        case(E_ZCL_STRUCT):
        case(E_ZCL_INT16):
        case(E_ZCL_UINT16):
        case(E_ZCL_ENUM16):
        case(E_ZCL_CLUSTER_ID):
        case(E_ZCL_ATTRIBUTE_ID):
        {
 		    u64Data = (uint64_t )pri_ntohs(psMessage->uData.u16Data);
        }
            break;
            
        case(E_ZCL_UINT24):
        case(E_ZCL_UINT32):
        case(E_ZCL_TOD):
        case(E_ZCL_DATE):
        case(E_ZCL_UTCT):
        case(E_ZCL_BACNET_OID):
        {
			u64Data = (uint64_t )pri_ntohl(psMessage->uData.u32Data);
        }
            break;
            
        case(E_ZCL_UINT40):
        case(E_ZCL_UINT48):
        case(E_ZCL_UINT56):
        case(E_ZCL_UINT64):
        case(E_ZCL_IEEE_ADDR):
            break;
                    
        case E_ZCL_OSTRING:
        case E_ZCL_CSTRING:
            break;
            
        case E_ZCL_LOSTRING:
        case E_ZCL_LCSTRING:
            break;
            
        default:
            break;
    }

	handleAttribute( psMessage->u16ShortAddress,psMessage->u16ClusterID,
				psMessage->u16AttributeID,u64Data,psMessage->u8Endpoint );

#if 0
    if((psMessage->u8Type == E_ZCL_OSTRING) || (psMessage->u8Type == E_ZCL_CSTRING))        
         ;//      LOG(ZCB, INFO, "attr value = %s\r\n", sAttribute->uData.sData.pData);
    else
          ;//    LOG(ZCB, INFO, "attr value = %d\r\n", sAttribute->uData.u64Data);

    if (sDevice->eDeviceState == E_ZB_DEVICE_STATE_ACTIVE)
        ;//vZDM_cJSON_AttrUpdate(sAttribute);

    if ((sDevice->sZDEndpoint[0].u16DeviceType == 2) //Alarm Button
        && (psMessage->u16ClusterID == E_ZB_CLUSTERID_ONOFF)
        && (psMessage->u16AttributeID == E_ZB_ATTRIBUTEID_ONOFF_ONOFF)
        && (sAttribute->uData.u64Data == 1))  //On
    {
 //       LOG(ZCB, INFO, "Rx On Report from Button, ready to control a light\r\n");
        uint8_t i;
        for (i = 0; i < 5; i++) {
            if (deviceTable[i].sZDEndpoint[0].u16DeviceType == 257) { 
                eOnOff(E_ZB_ADDRESS_MODE_SHORT,
                       deviceTable[i].u16NodeId,
                       1,
                       1,
                       E_CLD_ONOFF_CMD_TOGGLE);
   //             LOG(ZCB, INFO, "Controlled the dimmer light 0x%04X\r\n", deviceTable[i].u16NodeId);
                break;
            } else if (deviceTable[i].sZDEndpoint[0].u16DeviceType == 0) {
                break;
            }
        }
    }
#endif
}

static void ZCB_HandleActiveEndPointResp(void *pvUser, uint16_t u16Length, void *pvMessage) 
{
    //ZCB_DEBUG("ZCB_HandleActiveEndPointResp\r\n" );
    tsZDActiveEndPointResp *psMessage = (tsZDActiveEndPointResp *) pvMessage;
    uint16_t u16ShortAddress  = pri_ntohs(psMessage->u16ShortAddress);
    tsZbDeviceInfo* sDevice = tZDM_FindDeviceByNodeId(u16ShortAddress);
    if (sDevice == NULL) {
        return; 
    }
    
    if (sDevice->eDeviceState != E_ZB_DEVICE_STATE_ACTIVE) {
        sDevice->u8EndpointCount = psMessage->u8EndPointCount;
    //    LOG(ZCB, INFO, "ActiveEpRsp: -EpList: ");
        for (uint8_t i = 0; i < sDevice->u8EndpointCount; i++) {
            sDevice->sZDEndpoint[i].u8EndpointId = psMessage->au8EndPointList[i];
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
static void ZCB_HandleSimpleDescriptorResponse(void *pvUser, uint16_t u16Length, void *pvMessage)
{
    //ZCB_DEBUG( "ZCB_HandleSimpleDescriptorResponse\r\n" );
    tsZDSimpleDescRsp *psMessage = (tsZDSimpleDescRsp *) pvMessage;
    uint16_t  u16ShortAddress    = pri_ntohs(psMessage->u16ShortAddress);
    uint8_t u8EndPoint           = psMessage->u8Endpoint;
    uint16_t  u16DeviceId        = pri_ntohs(psMessage->u16DeviceID);
    uint8_t u8InClusterCnt       = psMessage->u8ClusterCount;

    tsZbDeviceEndPoint * devEp;
	uint8_t   u8Cluster;

 //   LOG(ZCB, INFO, "SimpleRsp: addr = 0x%04x, ep = %d, devId = 0x%04x\r\n", u16ShortAddress, u8EndPoint, u16DeviceId);
    
    tsZbDeviceInfo *sDevice = tZDM_FindDeviceByNodeId(u16ShortAddress);
    if (sDevice->eDeviceState != E_ZB_DEVICE_STATE_ACTIVE) {
        devEp = tZDM_FindEndpointEntryInDeviceTable(sDevice->u16NodeId, u8EndPoint);
        devEp->u16DeviceType  = u16DeviceId;
        uint8_t actualClusCnt = 0;
        uint16_t tempClusterId = 0;
        for (uint8_t i = 0; i < u8InClusterCnt; i++) {
            tempClusterId = pri_ntohs(psMessage->au16Clusters[i]);
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

static void ZCB_HandleReadAttrResp(void *pvUser, uint16_t u16Length, void *pvMessage)
{
 //   LOG(ZCB, INFO, "ZCB_HandleReadAttrResp\r\n" );

    struct _sReadAttributeResponse {
        uint8_t     u8SequenceNo;
        uint16_t    u16ShortAddress;
        uint8_t     u8EndPoint;
        uint16_t    u16ClusterId;
        uint16_t    u16AttributeId;
        uint8_t     u8AttributeStatus;
        uint8_t     u8AttributeType;
        uint16_t    u16SizeOfAttributesInBytes;
        uint8_t     auAttributeValue[50];
    } PACKED *psMessage = (struct _sReadAttributeResponse *)pvMessage;

    psMessage->u16ShortAddress = pri_ntohs(psMessage->u16ShortAddress);
    psMessage->u16ClusterId    = pri_ntohs(psMessage->u16ClusterId);
    psMessage->u16AttributeId  = pri_ntohs(psMessage->u16AttributeId);
    tsZbDeviceAttribute *sAttribute = tZDM_FindAttributeEntryByElement(psMessage->u16ShortAddress,
                                                                       psMessage->u8EndPoint,
                                                                       psMessage->u16ClusterId,
                                                                       psMessage->u16AttributeId);
    if (sAttribute == NULL) {
        return;
    }
    
    sAttribute->u8DataType = psMessage->u8AttributeType;
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
            memcpy(&u8Data, psMessage->auAttributeValue, sizeof(uint8_t));
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
            memcpy(&u16Data, psMessage->auAttributeValue, sizeof(uint16_t));
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
            memcpy(&u32Data, psMessage->auAttributeValue, sizeof(uint32_t));
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
            memcpy(&u64Data, psMessage->auAttributeValue, sizeof(uint64_t));
            sAttribute->uData.u64Data = pri_ntohd(u64Data);
        }
            break;
                    
        case E_ZCL_OSTRING:
        case E_ZCL_CSTRING:
            sAttribute->uData.sData.u8Length = (uint8_t)pri_ntohs(psMessage->u16SizeOfAttributesInBytes);
            if (sAttribute->uData.sData.pData == NULL) {
                sAttribute->uData.sData.pData = pvPortMalloc(sizeof(uint8_t) * (sAttribute->uData.sData.u8Length + 1));
            }      
            memcpy(sAttribute->uData.sData.pData, psMessage->auAttributeValue, sizeof(uint8_t) * sAttribute->uData.sData.u8Length);
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

    tsZbDeviceInfo *sDevice = tZDM_FindDeviceByNodeId(psMessage->u16ShortAddress);
    if (sDevice->eDeviceState != E_ZB_DEVICE_STATE_ACTIVE) {
        if ((psMessage->u16ClusterId == E_ZB_CLUSTERID_BASIC) && (psMessage->u16AttributeId == E_ZB_ATTRIBUTEID_BASIC_MODEL_ID)) {
            sDevice->eDeviceState = E_ZB_DEVICE_STATE_BIND_CLUSTER;
            vZDM_NewDeviceQualifyProcess(sDevice);
        }
    }
}

static void ZCB_HandleDefaultResponse(void *pvUser, uint16_t u16Length, void *pvMessage) 
{
    struct _sDefaultResponse {
        uint8_t             u8SequenceNo;           /**< Sequence number of outgoing message */
        uint8_t             u8Endpoint;             /**< Source endpoint */
        uint16_t            u16ClusterID;           /**< Source cluster ID */
        uint8_t             u8CommandID;            /**< Source command ID */
        uint8_t             u8Status;               /**< Command status */
    } PACKED *psMessage = (struct _sDefaultResponse *)pvMessage;

    psMessage->u16ClusterID  = pri_ntohs(psMessage->u16ClusterID);

//    LOG(ZCB, INFO, "Default Rsp : cluster 0x%04X Cmd 0x%02x status: %02x\r\n",
   //     psMessage->u16ClusterID, psMessage->u8CommandID, psMessage->u8Status);
}

static void ZCB_HandleLog(void *pvUser, uint16_t u16Length, void *pvMessage) 
{
    struct sLog
    {
        uint8_t  u8Level;
        uint8_t  au8Message[255];
    } PACKED * psMessage = (struct sLog *) pvMessage;
    
    psMessage->au8Message[u16Length] = '\0';
    
    //LOG(ZCB, INFO, "Log: %s (%d)\r\n",psMessage->au8Message,psMessage->u8Level);
}

static void ZCB_HandleIASZoneStatusChangeNotify (void *pvUser, uint16_t u16Length, void *pvMessage)
{
  //  LOG(ZCB, INFO, "ZCB_HandleIASZoneStatusChangeNotify\r\n" );
}

static void ZCB_HandleNetworkAddressReponse (void *pvUser, uint16_t u16Length, void *pvMessage)
{
 //   LOG(ZCB, INFO, "ZCB_HandleNetworkAddressRsp\r\n" );

    struct _sNetworkAddressResponse {
        uint8_t     u8SequenceNo;
        uint8_t     u8Status;
        uint64_t    u64IeeeAddress;
        uint16_t    u16ShortAddress;
        uint8_t     u8DeviceNumber;
        uint8_t     u8Index;
        uint16_t    auDeviceList[];
    } PACKED *psMessage = (struct _sNetworkAddressResponse *)pvMessage;

    psMessage->u64IeeeAddress  = pri_ntohd(psMessage->u64IeeeAddress);
    psMessage->u16ShortAddress = pri_ntohs(psMessage->u16ShortAddress);
    
    tsZbDeviceInfo *sDevice = tZDM_FindDeviceByIeeeAddress(psMessage->u64IeeeAddress);
    if (sDevice == NULL)
        return;
    
    sDevice->u64IeeeAddress = psMessage->u16ShortAddress;    
}

static void ZCB_HandleIeeeAddressReponse (void *pvUser, uint16_t u16Length, void *pvMessage)
{
 //   LOG(ZCB, INFO, "ZCB_HandleIeeeAddressRsp\r\n" );

    struct _sIeeeAddressResponse {
        uint8_t     u8SequenceNo;
        uint8_t     u8Status;
        uint64_t    u64IeeeAddress;
        uint16_t    u16ShortAddress;
        uint8_t     u8DeviceNumber;
        uint8_t     u8Index;
        uint16_t    auDeviceList[];
    } PACKED *psMessage = (struct _sIeeeAddressResponse *)pvMessage;

    psMessage->u64IeeeAddress  = pri_ntohd(psMessage->u64IeeeAddress);
    psMessage->u16ShortAddress = pri_ntohs(psMessage->u16ShortAddress);
    
    tsZbDeviceInfo *sDevice = tZDM_FindDeviceByIeeeAddress(psMessage->u64IeeeAddress);
    if (sDevice == NULL)
        return;
    
    sDevice->u16NodeId = psMessage->u16ShortAddress;
}

static void ZCB_HandleOtaUpgradeEndRequest(void *pvUser, uint16_t u16Length, void *pvMessage)
{
 //   LOG(ZCB, INFO, "ZCB_HandleOtaUpgradeEndRequest\r\n" );

    struct _sOtaUpgradeEndRequest {
        uint8_t     u8SequenceNumber;
        uint8_t     u8SrcEndpoint;  //0x01
        uint16_t    u16ClusterId;   //0x0019, ota clusterId 
        uint8_t     u8SrcAddrMode;
        uint16_t    u16SrcAddress;
        uint32_t    u32FileVersion;
        uint16_t    u16ImageType;
        uint16_t    u16ManufactureCode;
        uint8_t     u8Status;        
    } PACKED *psMessage = (struct _sOtaUpgradeEndRequest *)pvMessage;

    psMessage->u16SrcAddress        = pri_ntohs(psMessage->u16SrcAddress);
    psMessage->u32FileVersion       = pri_ntohl(psMessage->u32FileVersion);
    psMessage->u16ImageType         = pri_ntohs(psMessage->u16ImageType);
    psMessage->u16ManufactureCode   = pri_ntohs(psMessage->u16ManufactureCode);
    
    if (psMessage->u8Status == SUCCESS) {
   //     LOG(ZCB, INFO, "Device 0x%04X OTA Ends, file Version = %d\r\n",
   //         psMessage->u16SrcAddress, psMessage->u32FileVersion);
        
        eOtaUpgradeEndResponse(E_ZB_ADDRESS_MODE_SHORT,
                               psMessage->u16SrcAddress,
                               1,
                               1,
                               psMessage->u8SequenceNumber,
                               0x00000005,
                               0x0000000A,
                               psMessage->u32FileVersion,
                               psMessage->u16ImageType,
                               psMessage->u16ManufactureCode);
    }
}

static void ZCB_HandleGetPermitResponse(void *pvUser, uint16_t u16Length, void *pvMessage) 
{    
    struct _GetPermitResponse {
        uint8_t    u8Status;
    } PACKED *psGetPermitResponse = (struct _GetPermitResponse *)pvMessage;
 //   LOG(ZCB, INFO, "ZCB Permit Response Status: %d\r\n", psGetPermitResponse->u8Status);
}

static void HandleRestart(void *pvUser, uint16_t u16Length, void *pvMessage, int factoryNew) 
{
    const char *pcStatus = NULL;
    struct _tsRestart {
        uint8_t     u8Status;
    } PACKED *psRestart = (struct _tsRestart *)pvMessage;

    switch (psRestart->u8Status)
    {
#define STATUS(a, b) case(a): pcStatus = b; break
        STATUS(0, "STARTUP");
        STATUS(1, "WAIT_START");
        STATUS(2, "NFN_START");
        STATUS(3, "DISCOVERY");
        STATUS(4, "NETWORK_INIT");
        STATUS(5, "RESCAN");
        STATUS(6, "RUNNING");
#undef STATUS
        default: pcStatus = "Unknown";
    }
//    LOG(ZCB, INFO,  "ZCB Restart, FactoryNew = %d, status = %d (%s)\r\n", factoryNew, psRestart->u8Status, pcStatus);

    if ( factoryNew ) {
        teZcbStatus rt = eSetDeviceType(E_MODE_COORDINATOR);
        assert(rt == E_ZCB_OK);
        //should form the new network here
    } else {
        //control bridge has formed network already
    }
    return;
}

static void ZCB_HandleRestartProvisioned(void *pvUser, uint16_t u16Length, void *pvMessage) 
{
    HandleRestart( pvUser, u16Length, pvMessage, 0 );
}

static void ZCB_HandleRestartFactoryNew(void *pvUser, uint16_t u16Length, void *pvMessage) 
{
    HandleRestart( pvUser, u16Length, pvMessage, 1 );
}

static void vDevTimerCallback(TimerHandle_t xTimers)
{
    uint32_t index = (uint32_t)pvTimerGetTimerID(xTimers);
    deviceTimer[index].count ++;
 //   LOG(ZCB, INFO, "Device 0x%04X did not received Heartbeat, timeSinceLastMsg = %d min\r\n", deviceTable[index].u16NodeId, deviceTimer[index].count);
    if (deviceTimer[index].count >= ZB_DEVICE_MESSAGE_TIMER_OUT_COUNT) {
        xTimerStop(xTimers, 0);
        deviceTable[index].eDeviceState = E_ZB_DEVICE_STATE_OFF_LINE;
    }
}

teZcbStatus ZCB_OtaImageNotify(uint8_t u8AddrMode, 
                               uint16_t u16Addr, 
                               uint8_t u8SrcEp, 
                               uint8_t u8DstEp,
                               char * ota_notify_hd)
{
  //  LOG(ZCB, INFO, "ZCB_OtaImageNotify\r\n");
    teZcbStatus eStatus = E_ZCB_ERROR;
    struct _OtaImageNotify
    {
        uint16_t    u16ManufacturerCode;
        uint16_t    u16ImageType;
        uint32_t    u32FileVersion;
    } PACKED sOtaImageNotify;
    
    uint8_t * p_image = (uint8_t *)ota_notify_hd;

    uint16_t u16ManuCode;
    memcpy(&u16ManuCode, p_image, sizeof(uint16_t));
    sOtaImageNotify.u16ManufacturerCode = pri_ntohs(u16ManuCode);
    p_image += sizeof(uint16_t);

    uint16_t u16ImageTp;
    memcpy(&u16ImageTp, p_image, sizeof(uint16_t));
    sOtaImageNotify.u16ImageType = pri_ntohs(u16ImageTp);
    p_image += sizeof(uint16_t);

    uint32_t u32FileVer;
    memcpy(&u32FileVer, p_image, sizeof(uint32_t));
    sOtaImageNotify.u32FileVersion = pri_ntohl(u32FileVer);
    p_image = NULL;
    
    eStatus = eOtaImageNotify(u8AddrMode,
                              u16Addr,
                              u8SrcEp,
                              u8DstEp,
                              E_CLD_OTA_QUERY_JITTER,
                              sOtaImageNotify.u32FileVersion,      //0x00000002
                              sOtaImageNotify.u16ImageType,        //0x0101
                              sOtaImageNotify.u16ManufacturerCode, //0x1037
                              0x64);
    return eStatus;
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
