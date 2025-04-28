/*
 * Copyright 2021-2023 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ZIGBEEDEVICES_H
#define ZIGBEEDEVICES_H

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

#include "ZigbeeConstant.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#define PACKED __attribute__((__packed__))

#define MAX_NB_READ_ATTRIBUTES                  10
#define MAX_ZD_DEVICE_NUMBERS                   50
#define MAX_ZD_ENDPOINT_NUMBERS_PER_DEV          5
#define MAX_ZD_CLUSTER_NUMBERS_PER_EP           15
#define MAX_ZD_ATTRIBUTE_NUMBERS_TOTAL         512       


/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

typedef enum
{
    E_ZD_ADDRESS_MODE_BOUND = 0x00,
    E_ZD_ADDRESS_MODE_GROUP = 0x01,
    E_ZD_ADDRESS_MODE_SHORT = 0x02,
    E_ZD_ADDRESS_MODE_IEEE = 0x03,
    E_ZD_ADDRESS_MODE_BROADCAST = 0x04,
    E_ZD_ADDRESS_MODE_NO_TRANSMIT = 0x05,
    E_ZD_ADDRESS_MODE_BOUND_NO_ACK = 0x06,
    E_ZD_ADDRESS_MODE_SHORT_NO_ACK = 0x07,
    E_ZD_ADDRESS_MODE_IEEE_NO_ACK = 0x08,
} teZDAddressMode;


  /* Broadcast modes for data requests */
typedef enum
{
    E_ZD_BROADCAST_ALL, /* broadcast to all nodes, including all end devices */
    E_ZD_BROADCAST_RX_ON, /* broadcast to all nodes with their receivers enabled when idle */
    E_ZD_BROADCAST_ZC_ZR /* broadcast only to coordinator and routers */
} teZDBroadcastMode;


  /* On/Off Command - Payload */
typedef enum 
{ 
    E_CLD_ONOFF_CMD_OFF                      = 0x00,     /* Mandatory */
    E_CLD_ONOFF_CMD_ON,                                  /* Mandatory */
    E_CLD_ONOFF_CMD_TOGGLE,                              /* Mandatory */
} teZDOnOffCommand;



typedef struct
{
    teZDAddressMode eAddressMode;
    union
    {
      uint16_t u16GroupAddress;
      uint16_t u16ShortAddress;
      uint64_t u64IEEEAddress;
      teZDBroadcastMode eBroadcastMode;
    } uAddress;
}PACKED tsZDAddress;

typedef struct
{
    uint8_t u8SequenceNo;
    uint8_t u8Status;
    uint16_t u16ShortAddress;
    uint8_t u8Length;
    uint8_t u8Endpoint;
    uint16_t u16ProfileID;
    uint16_t u16DeviceID;
    struct
    {
        uint8_t u8DeviceVersion :4;
        uint8_t u8Reserved :4;
    } PACKED sBitField;
    uint8_t    u8ClusterCount;
    uint16_t   au16Clusters[MAX_ZD_CLUSTER_NUMBERS_PER_EP];
} PACKED tsZDSimpleDescRsp;

typedef struct
{
    uint8_t         u8AddressMode;
    uint16_t        u16ShortAddress;
    uint8_t         u8SourceEndPointId;
    uint8_t         u8DestinationEndPointId;
    uint16_t        u16ClusterId;
    uint16_t        u16AttributeId;
    uint8_t         bDirectionIsServerToClient;
    uint8_t         bIsManufacturerSpecific;
    uint16_t        u16ManufacturerCode;
    uint8_t         u8MaximumNumberOfIdentifiers;
} PACKED tsZDAttrDiscReq;



typedef struct
{
    uint8_t         u8AddressMode;
    uint16_t        u16ShortAddress;
    uint8_t         u8SourceEndPointId;
    uint8_t         u8DestinationEndPointId;
    uint16_t        u16ClusterId;
    uint8_t         bDirectionIsServerToClient;
    uint8_t         bIsManufacturerSpecific;
    uint16_t        u16ManufacturerCode;
    uint8_t         u8NumberOfAttributes;
    uint16_t        au16AttributeList[MAX_NB_READ_ATTRIBUTES]; /* 10 is max number hard-coded in ZCB */
} PACKED tsZDReadAttrReq;



typedef struct
{
    uint8_t   u8SequenceNumber;
    uint16_t  u16ShortAddress;
    uint8_t   u8SourceEndPointId;
    uint16_t  u16ClusterId;
    uint16_t  u16AttributeId;
    uint8_t   u8AttributeStatus;
    uint8_t   u8AttributeDataType;
    uint16_t  u16SizeOfAttributesInBytes;
    uint8_t   au8AttributeValue[];
} PACKED tsZDReadAttrResp;



typedef struct
{
    uint16_t        u16ShortAddress;
}PACKED tsZDActiveEndPointReq;



typedef struct
{
    uint8_t   u8SequenceNumber;
    uint8_t   u8Status;
    uint16_t  u16ShortAddress;
    uint8_t   u8EndPointCount;
    uint8_t   au8EndPointList[MAX_ZD_ENDPOINT_NUMBERS_PER_DEV];
} PACKED tsZDActiveEndPointResp;



typedef enum
{
    E_ZB_ATTRIBUTE_UINT64_TYPE       = 0x80,
    E_ZB_ATTRIBUTE_STRING_TYPE       = 0x81
} tsZbAttributeGeneralType;



typedef enum
{
	E_ZB_DEVICE_STATE_NULL           = 0x00,        
	E_ZB_DEVICE_STATE_NEW_JOINED     = 0x01,
	E_ZB_DEVICE_STATE_ACTIVE         = 0x02,
	E_ZB_DEVICE_STATE_OFF_LINE       = 0x03,
	E_ZB_DEVICE_STATE_LEFT           = 0x04,

    E_ZB_DEVICE_STATE_GET_ENDPOINT   = 0x20,
    E_ZB_DEVICE_STATE_GET_CLUSTER    = 0x21,
    E_ZB_DEVICE_STATE_BIND_CLUSTER   = 0x22,
    E_ZB_DEVICE_STATE_CFG_ATTRIBUTE  = 0x23,
    E_ZB_DEVICE_STATE_READ_ATTRIBUTE = 0x24,

	E_ZB_DEVICE_STATE_UNKNOWN        = 0xff 
} teZbDeviceState;



typedef enum
{
	E_ZB_NETWORK_STATE_NWK_UNFORMED = 0x00,
	E_ZB_NETWORK_STATE_NWK_FORMED   = 0x01,
	E_ZB_NETWORK_STATE_NWK_UNKNOWN  = 0xff
} teZbNetworkState;



typedef union
{
    uint64_t u64Data;
	struct
	{
		uint8_t u8Length;
		uint8_t *pData;
	} sData;	
} PACKED tuZbDeviceAttributeData;



typedef struct 
{
    uint16_t u16NodeId;
    uint8_t u8Endpoint;
    uint16_t u16ClusterId;
	uint16_t u16AttributeId;
	uint8_t u8DataType;
	tuZbDeviceAttributeData uData;
} PACKED tsZbDeviceAttribute;



typedef struct
{
	uint16_t u16ClusterId;
	uint8_t u8AttributeCount;
} PACKED tsZbDeviceCluster;



typedef struct
{
	uint8_t u8EndpointId;
    uint16_t u16DeviceType;
	uint8_t u8ClusterCount;
	tsZbDeviceCluster sZDCluster[MAX_ZD_CLUSTER_NUMBERS_PER_EP];
} PACKED tsZbDeviceEndPoint;



typedef struct
{
	uint16_t u16NodeId;
	uint64_t u64IeeeAddress;
	teZbDeviceState eDeviceState; 
	uint32_t timeSinceLastMessage;  //seconds
	uint8_t u8EndpointCount;
	tsZbDeviceEndPoint sZDEndpoint[MAX_ZD_ENDPOINT_NUMBERS_PER_DEV];
} PACKED tsZbDeviceInfo;



typedef struct
{
	uint16_t u16PanId;
	uint64_t u64IeeeAddress;
	uint8_t u8Channel;
	uint16_t u16DeviceCount;
	teZbNetworkState eNetworkState;
} tsZbNetworkInfo;



uint8_t uZDM_FindDevTableIndexByNodeId(uint16_t u16NodeId);

tsZbDeviceInfo* tZDM_FindDeviceByIndex(uint8_t u8Index);

tsZbDeviceInfo* tZDM_FindDeviceByNodeId(uint16_t u16NodeId);

tsZbDeviceInfo* tZDM_FindDeviceByIeeeAddress(uint64_t u64IeeeAddr);

tsZbDeviceInfo* tZDM_AddNewDeviceToDeviceTable(uint16_t u16NodeId, uint64_t u64IeeeAddr);

tsZbDeviceEndPoint* tZDM_FindEndpointEntryInDeviceTable(uint16_t u16NodeId, uint8_t u8Endpoint);

tsZbDeviceCluster* tZDM_FindClusterEntryInDeviceTable(uint16_t u16NodeId,
                                                      uint8_t u8Endpoint,
                                                      uint16_t u16ClusterId);

tsZbDeviceAttribute* tZDM_AddNewAttributeToAttributeTable(uint16_t u16NodeId,
                                                          uint8_t u8Endpoint,
                                                          uint16_t u16ClusterId,
                                                          uint16_t u16AttributeId,
                                                          uint8_t u8DataType);

tsZbDeviceAttribute* tZDM_FindAttributeEntryByIndex(uint16_t u16Index);

tsZbDeviceAttribute* tZDM_FindAttributeEntryByElement(uint16_t u16NodeId,
                                                      uint8_t u8Endpoint,
                                                      uint16_t u16ClusterId,
                                                      uint16_t u16AttributeId);

uint8_t uZDM_FindAttributeListByElement(uint16_t u16NodeId,
                                        uint8_t u8Endpoint,                                        
                                        uint16_t u16ClusterId,
                                        uint16_t auAttrList[]);

void bZDM_EraseAttributeInfoByNodeId(uint16_t u16NodeId);

bool bZDM_EraseDeviceFromDeviceTable(uint64_t u64IeeeAddr);

void vZDM_ClearAllDeviceTables();

void vZbDeviceTable_Init();


void vZDM_NewDeviceQualifyProcess(tsZbDeviceInfo* device);

void vZDM_cJSON_DeviceCreate(tsZbDeviceInfo *device);

void vZDM_cJSON_AttrUpdate(tsZbDeviceAttribute * attr);

void vZDM_cJSON_DeviceDelete(tsZbDeviceInfo *device);

void vZDM_cJSON_DevStateUpdate(tsZbDeviceInfo *device);

void vZDM_cJSON_DeviceRecovery(char *cjsonStr);

#if defined __cplusplus
}
#endif

#endif  /* ZIGBEEDEVICES_H */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
