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

#include "board.h"
#include "fsl_usart.h"

#include "fsl_device_registers.h"
#include "fsl_debug_console.h"

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "ZigbeeDevices.h"
#include "ZigbeeConstant.h"
#include "SerialLink.h"
#include "serial.h"
#include "zcb.h"
#include "cmd.h"

#define ZB_DEVICE_TABLE_NULL_NODE_ID            0
#define ZB_DEVICE_TABLE_NULL_IEEE_ADDR          0
#define ZB_DEVICE_ENDPOINT_COUNT_DEFAULT        1


tsZbNetworkInfo zbNetworkInfo;
tsZbDeviceInfo deviceTable[MAX_ZD_DEVICE_NUMBERS];
tsZbDeviceAttribute attributeTable[MAX_ZD_ATTRIBUTE_NUMBERS_TOTAL];


static bool bZD_ValidityCheckOfNodeId(uint16_t u16NodeId)
{
    if ((u16NodeId == ZB_DEVICE_TABLE_NULL_NODE_ID) 
        || (u16NodeId >= E_ZB_BROADCAST_ADDRESS_LOWPOWERROUTERS)) {
     //   LOG(ZDM, WARN, "Not the legal device node id\r\n");
        return false;
    }
    return true;
}



static bool bZD_ValidityCheckOfIeeeAddr(uint64_t u64IeeeAddr)
{
    if (u64IeeeAddr == ZB_DEVICE_TABLE_NULL_IEEE_ADDR) {
  //      LOG(ZDM, WARN, "Not the legal device ieee addr\r\n");
        return false;
    }
    return true;
}



static bool bZD_ValidityCheckOfEndpointId(uint8_t u8Endpoint)
{
    //Endpoint 0:   For ZDO
    //Endpoint 1-240: For Application
    //Endpoint 242: For GreenPower
    
    if ((u8Endpoint == 0) || (u8Endpoint == 241) || (u8Endpoint > 242)) {
 //       LOG(ZDM, WARN, "Not the legal endpoint id\r\n");
        return false;
    }
    return true;
}



uint8_t uZDM_FindDevTableIndexByNodeId(uint16_t u16NodeId)
{
	if (!bZD_ValidityCheckOfNodeId(u16NodeId)) {		
		return 0xFF;
	}
    
	for(uint8_t i = 0; i < MAX_ZD_DEVICE_NUMBERS; i++)
	{
		if(deviceTable[i].u16NodeId == u16NodeId) {
			return i;
		}
	}
    
//	LOG(ZDM, WARN, "No this device exits in current device table!\r\n");
	return 0xFF;
}



tsZbDeviceInfo* tZDM_FindDeviceByIndex(uint8_t u8Index)
{
	if (u8Index > MAX_ZD_DEVICE_NUMBERS) {
		return NULL;
	}	
	return &(deviceTable[u8Index]);
}


tsZbDeviceInfo* tZDM_FindDeviceByNodeId(uint16_t u16NodeId)
{
	if (!bZD_ValidityCheckOfNodeId(u16NodeId)) {		
		return NULL;
	}
	
	for(uint8_t i = 0; i < MAX_ZD_DEVICE_NUMBERS; i++)
	{
		if(deviceTable[i].u16NodeId == u16NodeId) {
			return &(deviceTable[i]);
		}
	}
//	LOG(ZDM, WARN, "No this device exits in current device table!\r\n");
	return NULL;
}



tsZbDeviceInfo* tZDM_FindDeviceByIeeeAddress(uint64_t u64IeeeAddr)
{
	if (!bZD_ValidityCheckOfIeeeAddr(u64IeeeAddr)) {		
		return NULL;
	}

	for(uint8_t i = 0; i < MAX_ZD_DEVICE_NUMBERS; i++)
	{
		if(deviceTable[i].u64IeeeAddress == u64IeeeAddr) {
			return &(deviceTable[i]);
		}
	}
//	LOG(ZDM, WARN, "No this device exits in current device table!\r\n");
	return NULL;	
}



tsZbDeviceInfo* tZDM_AddNewDeviceToDeviceTable(uint16_t u16NodeId, uint64_t u64IeeeAddr)
{
	if (!bZD_ValidityCheckOfNodeId(u16NodeId)) {		
		return NULL;
	}

	if (!bZD_ValidityCheckOfIeeeAddr(u64IeeeAddr)) {		
		return NULL;
	}    

	for (uint8_t i = 0; i < MAX_ZD_DEVICE_NUMBERS; i++)
	{	
	    if (deviceTable[i].u16NodeId == ZB_DEVICE_TABLE_NULL_NODE_ID) {
			memset(&(deviceTable[i]), 0, sizeof(tsZbDeviceInfo));
			deviceTable[i].u16NodeId = u16NodeId;
			deviceTable[i].u64IeeeAddress = u64IeeeAddr;
			deviceTable[i].eDeviceState = E_ZB_DEVICE_STATE_NEW_JOINED;
            zbNetworkInfo.u16DeviceCount ++;
			return &(deviceTable[i]);
		}		
	}
//	LOG(ZDM, WARN, "The device table is full already!\r\n");
	return NULL;
}



tsZbDeviceAttribute* tZDM_AddNewAttributeToAttributeTable(uint16_t u16NodeId,
                                                          uint8_t u8Endpoint,
                                                          uint16_t u16ClusterId,
                                                          uint16_t u16AttributeId,
                                                          uint8_t u8DataType)
{
	if (!bZD_ValidityCheckOfNodeId(u16NodeId)) {		
		return NULL;
	}

    if (!bZD_ValidityCheckOfEndpointId(u8Endpoint)) {		
		return NULL;
	}

    for (uint16_t i = 0; i < MAX_ZD_ATTRIBUTE_NUMBERS_TOTAL; i++) {
        if (attributeTable[i].u16NodeId == ZB_DEVICE_TABLE_NULL_NODE_ID) {
            attributeTable[i].u16NodeId      = u16NodeId;
            attributeTable[i].u8Endpoint     = u8Endpoint;
            attributeTable[i].u16ClusterId   = u16ClusterId;
            attributeTable[i].u16AttributeId = u16AttributeId;
            attributeTable[i].u8DataType     = u8DataType;
            tsZbDeviceCluster* devCluster = tZDM_FindClusterEntryInDeviceTable(u16NodeId, u8Endpoint, u16ClusterId);
            devCluster->u8AttributeCount ++;
            return &(attributeTable[i]);
        }
    }
//    LOG(ZDM, WARN, "The attribute table is full already!\r\n");
    return NULL;
}



tsZbDeviceEndPoint* tZDM_FindEndpointEntryInDeviceTable(uint16_t u16NodeId, uint8_t u8Endpoint)
{
    if (!bZD_ValidityCheckOfEndpointId(u8Endpoint)) {		
		return NULL;
	}

    tsZbDeviceInfo* sDevice = tZDM_FindDeviceByNodeId(u16NodeId);
    for (uint8_t i = 0; i < MAX_ZD_CLUSTER_NUMBERS_PER_EP; i++)
    {
        if (sDevice->sZDEndpoint[i].u8EndpointId == u8Endpoint)
            return &(sDevice->sZDEndpoint[i]);
    }
    return NULL;
}



tsZbDeviceCluster* tZDM_FindClusterEntryInDeviceTable(uint16_t u16NodeId,
                                                      uint8_t u8Endpoint,
                                                      uint16_t u16ClusterId)
{
    tsZbDeviceEndPoint *sEndpoint = tZDM_FindEndpointEntryInDeviceTable(u16NodeId, u8Endpoint);
    for (uint8_t i = 0; i < MAX_ZD_CLUSTER_NUMBERS_PER_EP; i++) {
        if (sEndpoint->sZDCluster[i].u16ClusterId == u16ClusterId)
            return &(sEndpoint->sZDCluster[i]);
    }
    return NULL;
}



tsZbDeviceAttribute* tZDM_FindAttributeEntryByIndex(uint16_t u16Index)
{
	if (u16Index > MAX_ZD_ATTRIBUTE_NUMBERS_TOTAL) {
		return NULL;
	}	
	return &(attributeTable[u16Index]);    
}



tsZbDeviceAttribute* tZDM_FindAttributeEntryByElement(uint16_t u16NodeId,
                                                      uint8_t u8Endpoint,
                                                      uint16_t u16ClusterId,
                                                      uint16_t u16AttributeId)
{
	if (!bZD_ValidityCheckOfNodeId(u16NodeId)) {		
		return NULL;
	}

    if (!bZD_ValidityCheckOfEndpointId(u8Endpoint)) {		
		return NULL;
	}

    for (uint16_t i = 0; i < MAX_ZD_ATTRIBUTE_NUMBERS_TOTAL; i++) {
        if (attributeTable[i].u16NodeId == u16NodeId) {
            if (attributeTable[i].u8Endpoint == u8Endpoint) {                
                if (attributeTable[i].u16ClusterId == u16ClusterId) {
                    if (attributeTable[i].u16AttributeId == u16AttributeId) {
                        return &(attributeTable[i]);
                    }
                }
            }
        }        
    }
    return NULL;    
}



uint8_t uZDM_FindAttributeListByElement(uint16_t u16NodeId,
                                        uint8_t u8Endpoint,
                                        uint16_t u16ClusterId,
                                        uint16_t auAttrList[])
{
	if (!bZD_ValidityCheckOfNodeId(u16NodeId)) {		
		return 0xFF;
	}

    if (!bZD_ValidityCheckOfEndpointId(u8Endpoint)) {		
		return 0xFF;
	}

    uint8_t attrCnt = 0;
    for (uint16_t i = 0; i < MAX_ZD_ATTRIBUTE_NUMBERS_TOTAL; i++) {
        if (attributeTable[i].u16NodeId == u16NodeId) {
            if (attributeTable[i].u8Endpoint == u8Endpoint) {                
                if (attributeTable[i].u16ClusterId == u16ClusterId) {
                    auAttrList[attrCnt ++] = attributeTable[i].u16AttributeId; 
                }
            }
        }        
    }
    tsZbDeviceCluster * devClus = tZDM_FindClusterEntryInDeviceTable(u16NodeId,
                                                                     u8Endpoint,
                                                                     u16ClusterId);
    devClus->u8AttributeCount = attrCnt;
    return attrCnt;
}



void bZDM_EraseAttributeInfoByNodeId(uint16_t u16NodeId)
{
    for (uint16_t i = 0; i < MAX_ZD_ATTRIBUTE_NUMBERS_TOTAL; i++) {
        if (attributeTable[i].u16NodeId == u16NodeId) {
            if ((attributeTable[i].u8DataType == E_ZCL_CSTRING)
                || (attributeTable[i].u8DataType == E_ZCL_OSTRING)) {
                vPortFree(attributeTable[i].uData.sData.pData);
            }
            memset(&(attributeTable[i]), 0, sizeof(attributeTable));
        }
    }
}




bool bZDM_EraseDeviceFromDeviceTable(uint64_t u64IeeeAddr)
{
	if (!bZD_ValidityCheckOfIeeeAddr(u64IeeeAddr)) {		
		return NULL;
	}  

	for (uint8_t i = 0; i < MAX_ZD_DEVICE_NUMBERS; i++)
	{
		if (deviceTable[i].u64IeeeAddress == u64IeeeAddr) {
		    bZDM_EraseAttributeInfoByNodeId(deviceTable[i].u16NodeId);
			memset(&(deviceTable[i]), 0, sizeof(tsZbDeviceInfo));
			return true;
		}
	}
//	LOG(ZDM, WARN, "No device can be erased!\r\n");
	return false;
}



void vZDM_ClearAllDeviceTables()
{
	memset(deviceTable, 0, sizeof(tsZbDeviceInfo) * MAX_ZD_DEVICE_NUMBERS);
    memset(attributeTable, 0, sizeof(tsZbDeviceAttribute) * MAX_ZD_ATTRIBUTE_NUMBERS_TOTAL);
    memset(&zbNetworkInfo, 0, sizeof(tsZbNetworkInfo));
}



void vZbDeviceTable_Init()
{
    vZDM_ClearAllDeviceTables();
}



void vZDM_NewDeviceQualifyProcess(tsZbDeviceInfo* device)
{
    uint8_t i,j,k;
    bool loop = true;
    uint16_t au16AttrList[1] = {E_ZB_ATTRIBUTEID_BASIC_MODEL_ID};
    static uint8_t epArrayIndex = 0;
    uint16_t clusterId;
    while (loop) {
        switch (device->eDeviceState)
        {
            case E_ZB_DEVICE_STATE_NEW_JOINED:
            {
                /*
                  After sending device announce notification to the host, the coordinator will 
                  process PDM saving of EndDevice timeout value. If the host send the uart data
                  to the coordinator at this time, it would cause the coordinator stack dump and
                  watchdog reset.
                  It may be a bug of JN5189 zigbee stack, add some delay here to avoid seen issue.
                */
                vTaskDelay (pdMS_TO_TICKS(100));
                for (i = 0; i < 2; i++)
                {
                    if (eActiveEndpointRequest(device->u16NodeId) != E_ZCB_OK)
                    {
                        PRINTF("\n ### Sending active endpoint request fail");
                        if (i == 1) {
                            device->u8EndpointCount = ZB_DEVICE_ENDPOINT_COUNT_DEFAULT;
                            device->eDeviceState = E_ZB_DEVICE_STATE_GET_CLUSTER;
                        } else {
                            vTaskDelay (pdMS_TO_TICKS(50));
                        }
                    }
                    else
                    {
                        //wait 1s for the active endpoint response
                        if (eSL_MessageWait(E_SL_MSG_ACTIVE_ENDPOINT_RESPONSE, 1000, NULL, NULL) != E_SL_OK)
                        {
                            PRINTF("\n ### No active endpoint response is received");
                            if (i == 1) {
                                device->u8EndpointCount = ZB_DEVICE_ENDPOINT_COUNT_DEFAULT;
                                device->eDeviceState = E_ZB_DEVICE_STATE_GET_CLUSTER;
                            } else {
                                vTaskDelay (pdMS_TO_TICKS(50));
                            }
                        } else {
                            loop = false;
                            break;
                        }
                    }
                }
            }
                break;
                
            case E_ZB_DEVICE_STATE_GET_CLUSTER:
                epArrayIndex ++;
                for (i = 0; i < 2; i++)
                {                   
				if ((device->sZDEndpoint[epArrayIndex - 1].u8EndpointId==0xF2)||(device->sZDEndpoint[epArrayIndex - 1].u8EndpointId==0))
						continue;
			#if 0
                    if (eSimpleDescriptorRequest(device->u16NodeId, device->sZDEndpoint[epArrayIndex - 1].u8EndpointId) != E_ZCB_OK)
			#else  // force to EP=1
				    if (eSimpleDescriptorRequest(device->u16NodeId, 1) != E_ZCB_OK)
			#endif
                    {
        //                LOG(ZDM, ERR, "Sending simple descriptor request fail\n");                        
                    }
                    else
                    {
                        //wait 1s for the simple descriptor response
                        if (eSL_MessageWait(E_SL_MSG_SIMPLE_DESCRIPTOR_RESPONSE, 1000, NULL, NULL) != E_SL_OK) {
         //                   LOG(ZDM, ERR, "No simple descriptor response is received\n");
                        } else {
                            break;
                        }
                    }
                    vTaskDelay (pdMS_TO_TICKS(50));
                }
                loop = false;
                break;
                
            case E_ZB_DEVICE_STATE_READ_ATTRIBUTE:
            {
                for (i = 0; i < device->u8EndpointCount; i++)
                {
                    for (j = 0; j < device->sZDEndpoint[i].u8ClusterCount; j++)
                    {
                        clusterId = device->sZDEndpoint[i].sZDCluster[j].u16ClusterId;
                        switch (clusterId)
                        {
                            case E_ZB_CLUSTERID_BASIC:
                            {                
                                tZDM_AddNewAttributeToAttributeTable(device->u16NodeId,
                                                                     device->sZDEndpoint[i].u8EndpointId,
                                                                     clusterId,
                                                                     E_ZB_ATTRIBUTEID_BASIC_MODEL_ID,
                                                                     E_ZB_ATTRIBUTE_STRING_TYPE);                

                                for (k = 0; k < 2; k++)
                                {
                                    if (eReadAttributeRequest(E_ZD_ADDRESS_MODE_SHORT, 
                                                              device->u16NodeId, 
                                                              ZB_ENDPOINT_SRC_DEFAULT, 
                                                              device->sZDEndpoint[i].u8EndpointId, 
                                                              E_ZB_CLUSTERID_BASIC,  
                                                              ZB_MANU_CODE_DEFAULT, 
                                                              1, 
                                                              au16AttrList) != E_ZCB_OK)
                                    {
                              //          LOG(ZDM, ERR, "Sending basic model id read request fail\r\n");
                                        if (k == 1) {
                                            /*eMgmtLeaveRequst(device->u16NodeId, device->u64IeeeAddress, 0, 1);
                                            if (bZDM_EraseDeviceFromDeviceTable(device->u64IeeeAddress)) {
                                                ZCB_DEBUG( "Erase Device Successfullly\r\n");
                                            }*/
                                        } else {
                                            vTaskDelay (pdMS_TO_TICKS(50));
                                        }
                                    }
                                    else
                                    {
                                        //wait 1s for the basic mode id response
                                        if (eSL_MessageWait(E_SL_MSG_READ_ATTRIBUTE_RESPONSE, 1000, NULL, NULL) != E_SL_OK)
                                        {
                                 //           LOG(ZDM, ERR, "No basic model id response is received\r\n");
                                            if (k == 1) {
                                                /*eMgmtLeaveRequst(device->u16NodeId, device->u64IeeeAddress, 0, 1);
                                                if (bZDM_EraseDeviceFromDeviceTable(device->u64IeeeAddress)) {
                                                    ZCB_DEBUG( "Erase Device Successfullly\r\n");
                                                }*/
                                            } else {
                                                vTaskDelay (pdMS_TO_TICKS(50));
                                            }
                                        } else {
                                            loop = false;
                                            break;
                                        }                       
                                    }
                                }
                            }
                                break;
                                
                            default:
                                break;
                        }
                    }
                }
            }
                loop = false;
                break;
                
            case E_ZB_DEVICE_STATE_BIND_CLUSTER:
            {
                for (i = 0; i < device->u8EndpointCount; i++)
                {
                    for (j = 0; j < device->sZDEndpoint[i].u8ClusterCount; j++)
                    {
                        uint16_t clusterId = device->sZDEndpoint[i].sZDCluster[j].u16ClusterId;
                        switch (clusterId)
                        {
                            case E_ZB_CLUSTERID_BASIC:                            
                                break;
                                
                            case E_ZB_CLUSTERID_ONOFF:
                            {
                                tZDM_AddNewAttributeToAttributeTable(device->u16NodeId,
                                                                     device->sZDEndpoint[i].u8EndpointId,
                                                                     clusterId,
                                                                     E_ZB_ATTRIBUTEID_ONOFF_ONOFF,
                                                                     E_ZB_ATTRIBUTE_UINT64_TYPE);
                                eSendBindUnbindCommand(device->u64IeeeAddress,
                                                       device->sZDEndpoint[i].u8EndpointId,
                                                       E_ZB_CLUSTERID_ONOFF,
                                                       SEND_BIND_REQUEST_COMMAND);
                                vTaskDelay (pdMS_TO_TICKS(50));
                            }
                                break;
                                
                            case E_ZB_CLUSTERID_LEVEL_CONTROL:
                            {
                                tZDM_AddNewAttributeToAttributeTable(device->u16NodeId,
                                                                     device->sZDEndpoint[i].u8EndpointId,
                                                                     clusterId,
                                                                     E_ZB_ATTRIBUTEID_LEVEL_CURRENTLEVEL,
                                                                     E_ZB_ATTRIBUTE_UINT64_TYPE); 
                                eSendBindUnbindCommand(device->u64IeeeAddress, 
                                                       device->sZDEndpoint[i].u8EndpointId, 
                                                       E_ZB_CLUSTERID_LEVEL_CONTROL,
                                                       SEND_BIND_REQUEST_COMMAND);
                                vTaskDelay (pdMS_TO_TICKS(50));
                            }
                                break; 
                                
                            case E_ZB_CLUSTERID_COLOR_CONTROL:
                            {
                                switch (device->sZDEndpoint[i].u16DeviceType)
                                {
                                    case E_ZB_DEVICEID_LIGHT_COLOR_TEMP:
                                    {
                                        tZDM_AddNewAttributeToAttributeTable(device->u16NodeId,
                                                                             device->sZDEndpoint[i].u8EndpointId,
                                                                             clusterId,
                                                                             E_ZB_ATTRIBUTEID_COLOUR_COLOURTEMPERATURE,
                                                                             E_ZB_ATTRIBUTE_UINT64_TYPE);
                                        eSendBindUnbindCommand(device->u64IeeeAddress,
                                                               device->sZDEndpoint[i].u8EndpointId,
                                                               E_ZB_CLUSTERID_COLOR_CONTROL,
                                                               SEND_BIND_REQUEST_COMMAND);
                                        vTaskDelay (pdMS_TO_TICKS(50));
                                    }

                                        break;
                                        
                                    case E_ZB_DEVICEID_LIGHT_COLOR_EXT:
                                    {
                                        tZDM_AddNewAttributeToAttributeTable(device->u16NodeId,
                                                                             device->sZDEndpoint[i].u8EndpointId,
                                                                             clusterId,
                                                                             E_ZB_ATTRIBUTEID_COLOUR_COLOURTEMPERATURE,
                                                                             E_ZB_ATTRIBUTE_UINT64_TYPE);
                                        tZDM_AddNewAttributeToAttributeTable(device->u16NodeId,
                                                                             device->sZDEndpoint[i].u8EndpointId,
                                                                             clusterId,
                                                                             E_ZB_ATTRIBUTEID_COLOUR_CURRENTX,
                                                                             E_ZB_ATTRIBUTE_UINT64_TYPE);
                                        tZDM_AddNewAttributeToAttributeTable(device->u16NodeId,
                                                                             device->sZDEndpoint[i].u8EndpointId,
                                                                             clusterId,
                                                                             E_ZB_ATTRIBUTEID_COLOUR_CURRENTY,
                                                                             E_ZB_ATTRIBUTE_UINT64_TYPE);                                        
                                        eSendBindUnbindCommand(device->u64IeeeAddress,
                                                               device->sZDEndpoint[i].u8EndpointId,
                                                               E_ZB_CLUSTERID_COLOR_CONTROL,
                                                               SEND_BIND_REQUEST_COMMAND);
                                        vTaskDelay (pdMS_TO_TICKS(50));
                                    }
                                        break;
                                        
                                    default:
                                        break;
                                    
                                }
                            }
                                break;

                            case E_ZB_CLUSTERID_MEASUREMENTSENSING_TEMP:
                            {
                                tZDM_AddNewAttributeToAttributeTable(device->u16NodeId,
                                                                     device->sZDEndpoint[i].u8EndpointId,
                                                                     clusterId,
                                                                     E_ZB_ATTRIBUTEID_MS_TEMP_MEASURED,
                                                                     E_ZB_ATTRIBUTE_UINT64_TYPE);
                                eSendBindUnbindCommand(device->u64IeeeAddress,
                                                       device->sZDEndpoint[i].u8EndpointId,
                                                       E_ZB_CLUSTERID_MEASUREMENTSENSING_TEMP,
                                                       SEND_BIND_REQUEST_COMMAND);
                            }
                                break;
                                
                            case E_ZB_CLUSTERID_MEASUREMENTSENSING_HUM:
                            {
                                tZDM_AddNewAttributeToAttributeTable(device->u16NodeId,
                                                                     device->sZDEndpoint[i].u8EndpointId,
                                                                     clusterId,
                                                                     E_ZB_ATTRIBUTEID_MS_HUM_MEASURED,
                                                                     E_ZB_ATTRIBUTE_UINT64_TYPE);
                                eSendBindUnbindCommand(device->u64IeeeAddress,
                                                       device->sZDEndpoint[i].u8EndpointId,
                                                       E_ZB_CLUSTERID_MEASUREMENTSENSING_HUM,
                                                       SEND_BIND_REQUEST_COMMAND);                                
                            }
                                break;
                                
                            case E_ZB_CLUSTERID_MEASUREMENTSENSING_ILLUM:
                            {
                                tZDM_AddNewAttributeToAttributeTable(device->u16NodeId,
                                                                     device->sZDEndpoint[i].u8EndpointId,
                                                                     clusterId,
                                                                     E_ZB_ATTRIBUTEID_MS_ILLUM_MEASURED,
                                                                     E_ZB_ATTRIBUTE_UINT64_TYPE);
                                eSendBindUnbindCommand(device->u64IeeeAddress,
                                                       device->sZDEndpoint[i].u8EndpointId,
                                                       E_ZB_CLUSTERID_MEASUREMENTSENSING_ILLUM,
                                                       SEND_BIND_REQUEST_COMMAND);
                                vTaskDelay (pdMS_TO_TICKS(50));
                            }
                                break;

                            case E_ZB_CLUSTERID_OCCUPANCYSENSING:
                            {
                                tZDM_AddNewAttributeToAttributeTable(device->u16NodeId,
                                                                     device->sZDEndpoint[i].u8EndpointId,
                                                                     clusterId,
                                                                     E_ZB_ATTRIBUTEID_MS_OCC_OCCUPANCY,
                                                                     E_ZB_ATTRIBUTE_UINT64_TYPE);   
								teZcbStatus status=eSendBindUnbindCommand(device->u64IeeeAddress,
											   		   device->sZDEndpoint[i].u8EndpointId,
					   								   E_ZB_CLUSTERID_OCCUPANCYSENSING,
													   SEND_BIND_REQUEST_COMMAND);
								if (status)
									PRINTF("\n Bind Occupancy Sensor Err : 0x%x\n",status);
								vTaskDelay (pdMS_TO_TICKS(50));
                            }
                                break;
                                
                            default:
                                break;
                        }
                    }

                }
                
                device->eDeviceState = E_ZB_DEVICE_STATE_ACTIVE;
                loop = false;
            }
                break;
            case E_ZB_DEVICE_STATE_CFG_ATTRIBUTE:
                loop = false;
                break;

            case E_ZB_DEVICE_STATE_ACTIVE:
                epArrayIndex = 0;
                loop = false;
                break;
                
            default:
                loop = false;
                break;
        }
        
    }

    if (device->eDeviceState == E_ZB_DEVICE_STATE_ACTIVE) {
        epArrayIndex = 0;
      //  vZDM_cJSON_DeviceCreate(device);
     //   if (eSetPermitJoining(0) != E_ZCB_OK) 
           ;//   LOG(ZDM, ERR, "Setting permit joining fail\r\n");
    }
}

// ------------------------------------------------------------------
// END OF FILE
// -----------------------------

