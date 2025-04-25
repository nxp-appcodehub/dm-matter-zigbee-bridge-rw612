/*
 *    Copyright (c) 2023 Project CHIP Authors
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#pragma once

#include "Device.h"
#include "CHIPProjectAppConfig.h"

#include <app/app-platform/ContentApp.h>
#include <app/app-platform/ContentAppPlatform.h>
#include <protocols/interaction_model/StatusCode.h>
using chip::Protocols::InteractionModel::Status;

class BridgeDevMgr
{
public:
    BridgeDevMgr();
    virtual ~BridgeDevMgr();

    void start();
    void RemoveAllDevice();
	static void ZcbMonitor(void *context);
    static void HandleDeviceOnOffStatusChanged(DeviceOnOff * dev, DeviceOnOff::Changed_t itemChangedMask);
    static void HandleDeviceDimmableStatusChanged(DeviceDimmable * dev, DeviceDimmable::Changed_t itemChangedMask);
    static void HandleDeviceColorStatusChanged(DeviceColor * dev, DeviceColor::Changed_t itemChangedMask); 
    static void HandleDeviceTempSensorStatusChanged(DeviceTempSensor * dev, DeviceTempSensor::Changed_t itemChangedMask);

    friend Status emberAfExternalAttributeReadCallback(EndpointId endpoint, ClusterId clusterId,
                                                   const EmberAfAttributeMetadata * attributeMetadata, uint8_t * buffer,
                                                   uint16_t maxReadLength);
    friend Status emberAfExternalAttributeWriteCallback(EndpointId endpoint, ClusterId clusterId,
                                                    const EmberAfAttributeMetadata * attributeMetadata, uint8_t * buffer);
private:
    void AddOnOffNode(ZigbeeDev_t *ZigbeeDev);
    void AddDimmableNode(ZigbeeDev_t *ZigbeeDev);
    void AddColorNode(ZigbeeDev_t *ZigbeeDev);
    void AddTempSensorNode(ZigbeeDev_t *ZigbeeDev);

    void RemoveOnOffNode(DeviceOnOff* dev);
	void RemoveDimmableNode(DeviceDimmable* dev);
	void RemoveColorNode(DeviceColor* dev);
    void RemoveTempMeasurementNode(DeviceTempSensor* dev);

    void AddNewZcbNode(newdb_zcb_t zcb);
    int  AddZcbNode(ZigbeeDev_t* ZigbeeDev);
    void RemoveDevice(Device *dev);
    int  start_threads();

    int AddDeviceEndpoint(Device * dev, EmberAfEndpointType * ep, const Span<const EmberAfDeviceType> & deviceTypeList,
                      const Span<DataVersion> & dataVersionStorage, chip::EndpointId parentEndpointId);
    int RemoveDeviceEndpoint(Device * dev);

    bool WriteAttributeToDynamicEndpoint(newdb_zcb_t zcb,uint16_t u16ClusterID,uint16_t u16AttributeID, uint64_t u64Data, uint8_t u8DataType);

	void RetrieveJoinedNode(ZigbeeDev_t *ZigbeeDev,uint8_t type,uint16_t ep);
	void RetrieveJoinedNodes(newdb_zcb_t zcb);	
	int RestoreNodeEndpoint(Device * dev,uint16_t EpId,EmberAfEndpointType * ep, const Span<const EmberAfDeviceType> & deviceTypeList,
                      const Span<DataVersion> & dataVersionStorage, chip::EndpointId parentEndpointId);

    BridgeDevMgr (const BridgeDevMgr&) = delete;
    BridgeDevMgr &operator=(const BridgeDevMgr&) = delete;

protected:
    static Device* gDevices[CHIP_DEVICE_CONFIG_DYNAMIC_ENDPOINT_COUNT];
    EndpointId mCurrentEndpointId;
    EndpointId mFirstDynamicEndpointId;
};
