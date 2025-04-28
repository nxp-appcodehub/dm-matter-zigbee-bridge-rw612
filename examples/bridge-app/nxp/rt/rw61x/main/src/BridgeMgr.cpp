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

#include <app-common/zap-generated/attribute-type.h>

#include "BridgeMgr.h"
#include "ZcbMessage.h"
#include "newDb.h"
#include "zcb.h"
#include "ZigbeeConstant.h"
#include "ZigbeeDevices.h"

#include "CHIPProjectAppConfig.h"

#include "ram_storage.h"

using chip::Protocols::InteractionModel::Status;

// define global variable
ZcbMsg_t ZcbMsg = {
    .AnnounceStart = false,
    .HandleMask = true,
    .msg_type = BRIDGE_UNKNOW,
};

// Cluster Endpoint
DECLARE_DYNAMIC_ENDPOINT(bridgedLightEndpoint, LIGHT_CLUSTER_LIST);
DECLARE_DYNAMIC_ENDPOINT(bridgedDimmableEndpoint, DIMMABLE_CLUSTER_LIST);
DECLARE_DYNAMIC_ENDPOINT(bridgedColorEndpoint, COLORCONTROL_CLUSTER_LIST); 
DECLARE_DYNAMIC_ENDPOINT(bridgedTempSensorEndpoint, TEMPSENSOR_CLUSTER_LIST);

Device* BridgeDevMgr::gDevices[CHIP_DEVICE_CONFIG_DYNAMIC_ENDPOINT_COUNT];

BridgeDevMgr::BridgeDevMgr()
{
}

BridgeDevMgr::~BridgeDevMgr()
{
    RemoveAllDevice();
}

void BridgeDevMgr::AddOnOffNode(ZigbeeDev_t *ZigbeeDev)
{
    std::string onoff_label = "Zigbee Light ";
    DeviceOnOff *Light = new DeviceOnOff((onoff_label + std::to_string(ZigbeeDev->zcb.saddr)).c_str(), "Office");

    Light->SetZigbee(*ZigbeeDev);
    Light->SetChangeCallback(&HandleDeviceOnOffStatusChanged);


    AddDeviceEndpoint(Light, &bridgedLightEndpoint,
                Span<const EmberAfDeviceType>(gBridgedOnOffDeviceTypes),
                Span<DataVersion>(Light->DataVersions), 1);

}

void BridgeDevMgr::AddDimmableNode(ZigbeeDev_t *ZigbeeDev)
{
    std::string dimmable_label = "Zigbee Dimmable ";
    DeviceDimmable *Dimmable = new DeviceDimmable((dimmable_label + std::to_string(ZigbeeDev->zcb.saddr)).c_str(), "Office");

    Dimmable->SetZigbee(*ZigbeeDev);
    Dimmable->SetChangeCallback(&HandleDeviceDimmableStatusChanged);

    AddDeviceEndpoint(Dimmable, &bridgedDimmableEndpoint,
                Span<const EmberAfDeviceType>(gBridgedDimmableDeviceTypes),
                Span<DataVersion>(Dimmable->DataVersions), 1);
}
void BridgeDevMgr::AddColorNode(ZigbeeDev_t *ZigbeeDev)
{
    std::string color_label = "Zigbee Color ";
    DeviceColor *Color = new DeviceColor((color_label + std::to_string(ZigbeeDev->zcb.saddr)).c_str(), "Office");

    Color->SetZigbee(*ZigbeeDev);
    Color->SetChangeCallback(&HandleDeviceColorStatusChanged);

    AddDeviceEndpoint(Color, &bridgedColorEndpoint,
                Span<const EmberAfDeviceType>(gBridgedColorControlDeviceTypes),
                Span<DataVersion>(Color->DataVersions), 1);
}

void BridgeDevMgr::AddTempSensorNode(ZigbeeDev_t *ZigbeeDev)
{
    std::string sensor_label = "Zigbee TempSensor ";
    DeviceTempSensor *TempSensor = new DeviceTempSensor((sensor_label + std::to_string(ZigbeeDev->zcb.saddr)).c_str(), "Office",
                                                        minMeasuredValue, maxMeasuredValue, initialMeasuredValue);

    TempSensor->SetZigbee(*ZigbeeDev);
    TempSensor->SetChangeCallback(&HandleDeviceTempSensorStatusChanged);

    AddDeviceEndpoint(TempSensor, &bridgedTempSensorEndpoint,
                Span<const EmberAfDeviceType>(gBridgedTempSensorDeviceTypes),
                Span<DataVersion>(TempSensor->DataVersions), 1);
}

void BridgeDevMgr::AddNewZcbNode(newdb_zcb_t zcb)
{
    ZigbeeDev_t ZigbeeDevice;

    ZigbeeDevice.zcb = zcb;
    AddZcbNode(&ZigbeeDevice);
}

int BridgeDevMgr::AddZcbNode(ZigbeeDev_t* ZigbeeDev)
{
	if (ZigbeeDev->zcb.uSupportedClusters.sClusterBitmap.hasColor) {
		PRINTF("\n ### Add Color Light\n");
		AddColorNode(ZigbeeDev);
		return 1;
	}

	if (ZigbeeDev->zcb.uSupportedClusters.sClusterBitmap.hasDimmable) {
		PRINTF("\n ### Add Dimmable Light\n ");
		AddDimmableNode(ZigbeeDev);
		return 1;
	}
	
    if (ZigbeeDev->zcb.uSupportedClusters.sClusterBitmap.hasOnOff) {
		PRINTF("\n ### Add OnOff Light\n ");
        AddOnOffNode(ZigbeeDev);
        return 1;
    }
	
    if (ZigbeeDev->zcb.uSupportedClusters.sClusterBitmap.hasTemperatureSensing) {
		PRINTF("\n ### Add Temperature Sensor\n ");
        AddTempSensorNode(ZigbeeDev);
        return 1;
    }

    if (ZigbeeDev->zcb.uSupportedClusters.sClusterBitmap.hasOccupancySensing) {
		PRINTF("\n ### Add Occupancy Sensor");
      //  AddOccupancySensorNode(ZigbeeDev); 
        return 1;
    }

    return 0;
}

void BridgeDevMgr::start()
{
    mFirstDynamicEndpointId = static_cast<chip::EndpointId>(
        static_cast<int>(emberAfEndpointFromIndex(static_cast<uint16_t>(emberAfFixedEndpointCount() - 1))) + 1);
    mCurrentEndpointId = mFirstDynamicEndpointId;

	ZcbMsg.bridge_mutex = xSemaphoreCreateMutex();

    // start monitor
    start_threads();
}

void BridgeDevMgr::ZcbMonitor(void *context)
{
    BridgeDevMgr *ThisMgr = (BridgeDevMgr *)context;
    while( 1 )
    {
  //	    OSA_TimeDelay(100);
    	vTaskDelay(100);
	if (xSemaphoreTake(ZcbMsg.bridge_mutex, portMAX_DELAY) != pdTRUE)
		ChipLogError(DeviceLayer, "### Failed to take semaphore ### ");
  
     switch (ZcbMsg.msg_type)
     {
                    case BRIDGE_ADD_DEV: {
                        ThisMgr->AddNewZcbNode(ZcbMsg.zcb);
                        ZcbMsg.zcb = {0};
                        ZcbMsg.msg_type = BRIDGE_UNKNOW;
                        }
                        break;

                    case BRIDGE_REMOVE_DEV: {
                        ThisMgr->RemoveDevice(gDevices[ZcbMsg.zcb.matterIndex]);
                        ZcbMsg.zcb = {0};
                        ZcbMsg.msg_type = BRIDGE_UNKNOW;
                        }
                        break;

                    case BRIDGE_FACTORY_RESET: {
                        ThisMgr->RemoveAllDevice();
                        ZcbMsg.zcb = {0};
                        ZcbMsg.msg_type = BRIDGE_UNKNOW;
                        }
                        break;

                    case BRIDGE_WRITE_ATTRIBUTE: {
                        ZcbAttribute_t *Data = (ZcbAttribute_t*)ZcbMsg.msg_data;
                        ThisMgr->WriteAttributeToDynamicEndpoint(ZcbMsg.zcb, Data->u16ClusterID, Data->u16AttributeID, Data->u64Data, ZCL_INT16S_ATTRIBUTE_TYPE);
                        ZcbMsg.zcb = {0};
                        ZcbMsg.msg_type = BRIDGE_UNKNOW;
                        }
                        break;

					case BRIDGE_RESTORE_JOINED_NODE:
						{
							ThisMgr->RetrieveJoinedNodes(ZcbMsg.zcb);						
							ZcbMsg.zcb = {0};
							ZcbMsg.msg_type = BRIDGE_UNKNOW;
						}
						break;
						
                    default:
                        break;
     }
	 
	 xSemaphoreGive(ZcbMsg.bridge_mutex);
    }
}

int BridgeDevMgr::start_threads()
{

#define MONITOR_TASK_STACK_SIZE   1024
#define MONITOR_TASK_PRIORITY     1

BaseType_t rt= xTaskCreate(&BridgeDevMgr::ZcbMonitor,
				"ZcbMonitor", 
				MONITOR_TASK_STACK_SIZE, 
				NULL, 
				MONITOR_TASK_PRIORITY,
				NULL);

if (rt != pdPASS) {
	ChipLogError(DeviceLayer, "### ZcbMonitor is Fail ### ");
	return -1;
}

    return 0;
}

void BridgeDevMgr::RemoveAllDevice()
{
    for (uint16_t i = 0; i < CHIP_DEVICE_CONFIG_DYNAMIC_ENDPOINT_COUNT; i++ ) 
	{
        if ( gDevices[i] != NULL) {
            RemoveDevice(gDevices[i]);
        }
    }

    mCurrentEndpointId = mFirstDynamicEndpointId;
}

void BridgeDevMgr::RemoveDevice(Device *dev)
{
    RemoveDeviceEndpoint(dev);
    if ( dev->GetZigbee()->zcb.uSupportedClusters.sClusterBitmap.hasOnOff ) {
        RemoveOnOffNode(static_cast<DeviceOnOff *>(dev));
    }

    if ( dev->GetZigbee()->zcb.uSupportedClusters.sClusterBitmap.hasDimmable ) {
        RemoveDimmableNode(static_cast<DeviceDimmable *>(dev));
    }

    if ( dev->GetZigbee()->zcb.uSupportedClusters.sClusterBitmap.hasDimmable ) {
        RemoveColorNode(static_cast<DeviceColor *>(dev));
    }

    if ( dev->GetZigbee()->zcb.uSupportedClusters.sClusterBitmap.hasTemperatureSensing ) {
        RemoveTempMeasurementNode(static_cast<DeviceTempSensor *>(dev));
    }

}

void BridgeDevMgr::RemoveOnOffNode(DeviceOnOff* dev)
{
    delete dev;
    dev = nullptr;
}

void BridgeDevMgr::RemoveDimmableNode(DeviceDimmable* dev)
{
    delete dev;
    dev = nullptr;
}

void BridgeDevMgr::RemoveColorNode(DeviceColor* dev)
{
    delete dev;
    dev = nullptr;
}

void BridgeDevMgr::RemoveTempMeasurementNode(DeviceTempSensor* dev)
{
    delete dev;
    dev = nullptr;
}

// -----------------------------------------------------------------------------------------
// Device Management
// -----------------------------------------------------------------------------------------
extern NodeDB JoinedNodes[DEV_NUM];
extern void UpdateJoinedNodes(uint16_t EP);

extern char * myDB_filename;

int BridgeDevMgr::AddDeviceEndpoint(Device * dev, EmberAfEndpointType * ep, const Span<const EmberAfDeviceType> & deviceTypeList,
                      const Span<DataVersion> & dataVersionStorage, chip::EndpointId parentEndpointId = chip::kInvalidEndpointId)
{
	JoinedNodesSaved savedNodes;

    uint8_t index = 0;
	int ret=0;

    if (dev->IsReachable() == true)
    {
        ChipLogProgress(DeviceLayer, "The endpoints has been added !!!");
        return -1;
    }

	ret=ramStorageReadFromFlash(myDB_filename,(uint8_t *)&savedNodes,sizeof(savedNodes));
	if (ret)
	{
		uint8_t j;
		uint16_t tmp=0;
	//		PRINTF("\n ### ramStorageReadFromFlash=%d,Joined Nodes=%d",ret,savedNodes.totalNodes);
			for (j=0;j<savedNodes.totalNodes;j++)
			{
				if (savedNodes.joinedNodes[j].ep>tmp)
					tmp=savedNodes.joinedNodes[j].ep;
			}		
	//			PRINTF("\n ### Idx=%d,Type=%d,short=0x%x,mac=0x%llx,ep=%d",j,savedNodes.joinedNodes[j].type,savedNodes.joinedNodes[j].shortaddr,savedNodes.joinedNodes[j].mac,savedNodes.joinedNodes[j].ep);
		mCurrentEndpointId=tmp+1;
	//	PRINTF("\n @@@ mCurrentEndpointId=%d",mCurrentEndpointId);
	}

	while (index < CHIP_DEVICE_CONFIG_DYNAMIC_ENDPOINT_COUNT )
    {
        if (nullptr == gDevices[index] )
        {
            gDevices[index] = dev;
            CHIP_ERROR ret;
            while (1)
            {
                // Todo: Update this to schedule the work rather than use this lock
                DeviceLayer::StackLock lock;
                dev->SetEndpointId(mCurrentEndpointId);
                // dev->SetParentEndpointId(parentEndpointId);
                ret =  emberAfSetDynamicEndpoint(index, mCurrentEndpointId, ep, dataVersionStorage, deviceTypeList, parentEndpointId);
                if (ret == CHIP_NO_ERROR)
                {
                    dev->SetReachable(true);
               //     dev->GetZigbee()->zcb.matterIndex = index; 
               //     ZcbMsg.zcb.matterIndex=index;
                    ChipLogProgress(DeviceLayer, "Added device %s SAddr 0x%x to dynamic endpoint %d (index=%d)", 
												dev->GetName(),dev->GetZigbeeSaddr(), mCurrentEndpointId, index);
				
  				    UpdateJoinedNodes(mCurrentEndpointId);
                    return index;
                }
			//	else
				//	ChipLogProgress(DeviceLayer, "### emberAfSetDynamicEndpoint is :%d ",ret);
				
                if (ret != CHIP_ERROR_ENDPOINT_EXISTS) //0x7f
                {
                    return -1;
                }
                // Handle wrap condition
                if (++mCurrentEndpointId < mFirstDynamicEndpointId)
                {
                    mCurrentEndpointId = mFirstDynamicEndpointId;
                }
            }
        }
        index++;
    }
    ChipLogProgress(DeviceLayer, "### Failed to add dynamic endpoint: No endpoints available !!! ");
    return -1;
}

int BridgeDevMgr::RemoveDeviceEndpoint(Device * dev)
{
    uint8_t index = 0;
    while (index < CHIP_DEVICE_CONFIG_DYNAMIC_ENDPOINT_COUNT)
    {
        if (gDevices[index] == dev)
        {
            // Todo: Update this to schedule the work rather than use this lock
            DeviceLayer::StackLock lock;
            EndpointId ep   = emberAfClearDynamicEndpoint(index);
            gDevices[index] = nullptr;
            ChipLogProgress(DeviceLayer, "Removed device %s from dynamic endpoint %d (index=%d)", dev->GetName(), ep, index);
            // Silence complaints about unused ep when progress logging
            // disabled.
         //   UNUSED_VAR(ep);
            return index;
        }
        index++;
    }
    return -1;
}

uint8_t gIndex=0;
int BridgeDevMgr::RestoreNodeEndpoint(Device * dev, uint16_t EpId,EmberAfEndpointType * ep, const Span<const EmberAfDeviceType> & deviceTypeList,
                      const Span<DataVersion> & dataVersionStorage, chip::EndpointId parentEndpointId = chip::kInvalidEndpointId)
{
	CHIP_ERROR ret;

    if (dev->IsReachable() == true)
    {
        PRINTF("\n ### The endpoints has been added! ");
        return -1;
    }	
	gDevices[gIndex] = dev;
	dev->SetEndpointId(EpId);
	ret=emberAfSetDynamicEndpoint(gIndex, EpId, ep, dataVersionStorage, deviceTypeList, parentEndpointId);
	dev->SetReachable(true);
	dev->GetZigbee()->zcb.matterIndex = gIndex;
	return 0;
}

void BridgeDevMgr::RetrieveJoinedNode(ZigbeeDev_t *ZigbeeDev,uint8_t DevType,uint16_t EP)
{
  switch (DevType)
  {
	case 1:
	{
    	std::string onoff_label = "Zigbee Light ";
    	DeviceOnOff *Light = new DeviceOnOff((onoff_label + std::to_string(ZigbeeDev->zcb.saddr)).c_str(), "Office");

    	Light->SetZigbee(*ZigbeeDev);
    	Light->SetChangeCallback(&HandleDeviceOnOffStatusChanged);
	
    	RestoreNodeEndpoint(Light,EP,&bridgedLightEndpoint,
                Span<const EmberAfDeviceType>(gBridgedOnOffDeviceTypes),
                Span<DataVersion>(Light->DataVersions), 1);		
		gIndex+=1;
	}  	
		break;

	case 2:
	{
		std::string dimmable_label = "Zigbee Dimmable ";
		DeviceDimmable *Dimmable = new DeviceDimmable((dimmable_label + std::to_string(ZigbeeDev->zcb.saddr)).c_str(), "Office");
		
		Dimmable->SetZigbee(*ZigbeeDev);
		Dimmable->SetChangeCallback(&HandleDeviceDimmableStatusChanged);
		
		RestoreNodeEndpoint(Dimmable,EP,&bridgedDimmableEndpoint,
					Span<const EmberAfDeviceType>(gBridgedDimmableDeviceTypes),
					Span<DataVersion>(Dimmable->DataVersions), 1);
		gIndex+=1;
	}
		break;

	case 3:
	{
    	std::string color_label = "Zigbee Color ";
    	DeviceColor *Color = new DeviceColor((color_label + std::to_string(ZigbeeDev->zcb.saddr)).c_str(), "Office");

    	Color->SetZigbee(*ZigbeeDev);
    	Color->SetChangeCallback(&HandleDeviceColorStatusChanged);

    	RestoreNodeEndpoint(Color,EP,&bridgedColorEndpoint,
                Span<const EmberAfDeviceType>(gBridgedColorControlDeviceTypes),
                Span<DataVersion>(Color->DataVersions), 1);	 
		gIndex+=1;
	}
	break;
	default:
		break;
	}
}

void BridgeDevMgr::RetrieveJoinedNodes(newdb_zcb_t zcb)
{
    ZigbeeDev_t ZigbeeDevice;

    ZigbeeDevice.zcb = zcb;
    RetrieveJoinedNode(&ZigbeeDevice,zcb.type,zcb.DynamicEP);
}

// -----------------------------------------------------------------------------------------
// Device callback
// -----------------------------------------------------------------------------------------
void CallReportingCallback(intptr_t closure)
{
    auto path = reinterpret_cast<app::ConcreteAttributePath *>(closure);
    MatterReportingAttributeChangeCallback(*path);
    Platform::Delete(path);
}

void ScheduleReportingCallback(Device * dev, ClusterId cluster, AttributeId attribute)
{
    auto * path = Platform::New<app::ConcreteAttributePath>(dev->GetEndpointId(), cluster, attribute);
    PlatformMgr().ScheduleWork(CallReportingCallback, reinterpret_cast<intptr_t>(path));
}

void HandleDeviceStatusChanged(Device * dev, Device::Changed_t itemChangedMask)
{
    if (itemChangedMask & Device::kChanged_Reachable)
    {
        ScheduleReportingCallback(dev, BridgedDeviceBasicInformation::Id, BridgedDeviceBasicInformation::Attributes::Reachable::Id);
    }

    if (itemChangedMask & Device::kChanged_Name)
    {
        ScheduleReportingCallback(dev, BridgedDeviceBasicInformation::Id, BridgedDeviceBasicInformation::Attributes::NodeLabel::Id);
    }
}

Status HandleReadBridgedDeviceBasicAttribute(Device * dev, chip::AttributeId attributeId, uint8_t * buffer,
                                                    uint16_t maxReadLength)
{
    using namespace BridgedDeviceBasicInformation::Attributes;

 //   ChipLogProgress(DeviceLayer, "HandleReadBridgedDeviceBasicAttribute: attrId=%d, maxReadLength=%d", attributeId, maxReadLength); 

    if ((attributeId == Reachable::Id) && (maxReadLength == 1))
    {
        *buffer = dev->IsReachable() ? 1 : 0;
    }
    else if ((attributeId == NodeLabel::Id) && (maxReadLength == 32))
    {
        MutableByteSpan zclNameSpan(buffer, maxReadLength);
        MakeZclCharString(zclNameSpan, dev->GetName());
    }
    else if ((attributeId == ClusterRevision::Id) && (maxReadLength == 2))
    {
        uint16_t rev = ZCL_BRIDGED_DEVICE_BASIC_INFORMATION_CLUSTER_REVISION;
        memcpy(buffer, &rev, sizeof(rev));
    }
    else if ((attributeId == FeatureMap::Id) && (maxReadLength == 4))
    {
        uint32_t featureMap = ZCL_BRIDGED_DEVICE_BASIC_INFORMATION_FEATURE_MAP;
        memcpy(buffer, &featureMap, sizeof(featureMap));
    }
    else
    {
        return Status::Failure;
    }

    return Status::Success;
}

Status HandleReadOnOffAttribute(DeviceOnOff * dev, chip::AttributeId attributeId, uint8_t * buffer, uint16_t maxReadLength)
{
  //  ChipLogProgress(DeviceLayer, "HandleReadOnOffAttribute: attrId=%d, maxReadLength=%d", attributeId, maxReadLength);  

    if ((attributeId == OnOff::Attributes::OnOff::Id) && (maxReadLength == 1))
    {
        *buffer = dev->IsOn() ? 1 : 0;
    }
    else if ((attributeId == OnOff::Attributes::ClusterRevision::Id) && (maxReadLength == 2))
    {
        uint16_t rev = ZCL_ON_OFF_CLUSTER_REVISION;
        memcpy(buffer, &rev, sizeof(rev));
    }
    else
    {
        return Status::Failure;
    }

    return Status::Success;
}

Status HandleReadTempMeasurementAttribute(DeviceTempSensor * dev, chip::AttributeId attributeId, uint8_t * buffer,
                                                 uint16_t maxReadLength)
{
    using namespace TemperatureMeasurement::Attributes;

    if ((attributeId == MeasuredValue::Id) && (maxReadLength == 2))
    {
        int16_t measuredValue = dev->GetMeasuredValue();
        memcpy(buffer, &measuredValue, sizeof(measuredValue));
    }
    else if ((attributeId == MinMeasuredValue::Id) && (maxReadLength == 2))
    {
        int16_t minValue = dev->mMin;
        memcpy(buffer, &minValue, sizeof(minValue));
    }
    else if ((attributeId == MaxMeasuredValue::Id) && (maxReadLength == 2))
    {
        int16_t maxValue = dev->mMax;
        memcpy(buffer, &maxValue, sizeof(maxValue));
    }
    else if ((attributeId == FeatureMap::Id) && (maxReadLength == 4))
    {
        uint32_t featureMap = ZCL_TEMPERATURE_SENSOR_FEATURE_MAP;
        memcpy(buffer, &featureMap, sizeof(featureMap));
    }
    else if ((attributeId == ClusterRevision::Id) && (maxReadLength == 2))
    {
        uint16_t clusterRevision = ZCL_TEMPERATURE_SENSOR_CLUSTER_REVISION;
        memcpy(buffer, &clusterRevision, sizeof(clusterRevision));
    }
    else
    {
        return Status::Failure;
    }

    return Status::Success;
}

void BridgeDevMgr::HandleDeviceOnOffStatusChanged(DeviceOnOff * dev, DeviceOnOff::Changed_t itemChangedMask)
{
    if (itemChangedMask & (DeviceOnOff::kChanged_Reachable | DeviceOnOff::kChanged_Name | DeviceOnOff::kChanged_Location))
    {
        HandleDeviceStatusChanged(static_cast<Device *>(dev), (Device::Changed_t) itemChangedMask);
    }

    if (itemChangedMask & DeviceOnOff::kChanged_OnOff)
    {
        ScheduleReportingCallback(dev, OnOff::Id, OnOff::Attributes::OnOff::Id);
    }
}

void BridgeDevMgr::HandleDeviceDimmableStatusChanged(DeviceDimmable * dev, DeviceDimmable::Changed_t itemChangedMask)
{
    if (itemChangedMask & (DeviceDimmable::kChanged_Reachable | DeviceDimmable::kChanged_Name | DeviceDimmable::kChanged_Location))
    {
        HandleDeviceStatusChanged(static_cast<Device *>(dev), (Device::Changed_t) itemChangedMask);
    }

    if (itemChangedMask & DeviceDimmable::kChanged_Dimmable)
    {
        ScheduleReportingCallback(dev, LevelControl::Id, LevelControl::Attributes::CurrentLevel::Id);
    }
}
void BridgeDevMgr::HandleDeviceColorStatusChanged(DeviceColor * dev, DeviceColor::Changed_t itemChangedMask)
{
    if (itemChangedMask & (DeviceColor::kChanged_Reachable | DeviceColor::kChanged_Name | DeviceColor::kChanged_Location))
    {
        HandleDeviceStatusChanged(static_cast<Device *>(dev), (Device::Changed_t) itemChangedMask);
    }

    if (itemChangedMask & DeviceColor::kChanged_Color)
    {
        ScheduleReportingCallback(dev, ColorControl::Id, ColorControl::Attributes::CurrentHue::Id);
    }
}

void BridgeDevMgr::HandleDeviceTempSensorStatusChanged(DeviceTempSensor * dev, DeviceTempSensor::Changed_t itemChangedMask)
{
    if (itemChangedMask &
        (DeviceTempSensor::kChanged_Reachable | DeviceTempSensor::kChanged_Name | DeviceTempSensor::kChanged_Location))
    {
        HandleDeviceStatusChanged(static_cast<Device *>(dev), (Device::Changed_t) itemChangedMask);
    }
    if (itemChangedMask & DeviceTempSensor::kChanged_MeasurementValue)
    {
        ScheduleReportingCallback(dev, TemperatureMeasurement::Id, TemperatureMeasurement::Attributes::MeasuredValue::Id);
    }
}


Status emberAfExternalAttributeReadCallback(EndpointId endpoint, ClusterId clusterId,
                                                   const EmberAfAttributeMetadata * attributeMetadata, uint8_t * buffer,
                                                   uint16_t maxReadLength)
{
    uint16_t endpointIndex = emberAfGetDynamicIndexFromEndpoint(endpoint);

    Status ret = Status::Failure;
    if ((endpointIndex < CHIP_DEVICE_CONFIG_DYNAMIC_ENDPOINT_COUNT) && (BridgeDevMgr::gDevices[endpointIndex] != nullptr))
    {
        Device * dev = BridgeDevMgr::gDevices[endpointIndex];

        if (clusterId == BridgedDeviceBasicInformation::Id)
        {
            ret = HandleReadBridgedDeviceBasicAttribute(dev, attributeMetadata->attributeId, buffer, maxReadLength);
        }
        else if (clusterId == OnOff::Id)
        {
            ret = HandleReadOnOffAttribute(static_cast<DeviceOnOff *>(dev), attributeMetadata->attributeId, buffer, maxReadLength);
        }
        else if (clusterId == TemperatureMeasurement::Id)
        {
            ret = HandleReadTempMeasurementAttribute(static_cast<DeviceTempSensor *>(dev), attributeMetadata->attributeId, buffer,maxReadLength);
        }
    }

    return ret;
}


// -----------------------------------------------------------------------------------------
//  zcb node report attribute to Bridge
// -----------------------------------------------------------------------------------------
bool BridgeDevMgr::WriteAttributeToDynamicEndpoint(newdb_zcb_t zcb, uint16_t u16ClusterID,uint16_t u16AttributeID, uint64_t u64Data, uint8_t u8DataType)
{
//	PRINTF("\n ^^^ WriteAttributeToDynamicEndpoint=%d,Dev=0x%p\n",zcb.matterIndex,gDevices[zcb.matterIndex]);

    if ( gDevices[zcb.matterIndex] == NULL) {
        return false;
    }

	switch (u16ClusterID)
	{
		case E_ZB_CLUSTERID_ONOFF:
		{
			DeviceOnOff *OnOff = static_cast<DeviceOnOff *>(gDevices[zcb.matterIndex]);
			OnOff->SetOnOffValue(static_cast<int8_t>(u64Data));
		}
		break;
			
		case E_ZB_CLUSTERID_LEVEL_CONTROL:
		{
			DeviceOnOff *OnOff = static_cast<DeviceOnOff *>(gDevices[zcb.matterIndex]);
			OnOff->SetLevelValue(static_cast<int8_t>(u64Data));
		}
		break;

		case E_ZB_CLUSTERID_MEASUREMENTSENSING_ILLUM:
		case E_ZB_CLUSTERID_MEASUREMENTSENSING_TEMP:
		case E_ZB_CLUSTERID_OCCUPANCYSENSING:	
		{
			DeviceTempSensor *TempSensor = static_cast<DeviceTempSensor *>(gDevices[zcb.matterIndex]);
			TempSensor->SetMeasuredValue(static_cast<int16_t>(u64Data));
		}
		break;

		default: break;
	}

    return true;
}


CHIP_ERROR ProcessOnOffClusterCommand(const chip::app::ConcreteCommandPath & aCommandPath,const chip::TLV::TLVReader & commandDataReader)
{
	CHIP_ERROR TLVError = CHIP_NO_ERROR;
	chip::TLV::TLVReader aDataTlv(commandDataReader);
	  switch (aCommandPath.mCommandId)
        {
        case app::Clusters::OnOff::Commands::Off::Id: {
        app::Clusters::OnOff::Commands::Off::DecodableType commandData;
        TLVError = DataModel::Decode(aDataTlv, commandData); 
        if (TLVError == CHIP_NO_ERROR) {
		BridgedOnOff(aCommandPath.mEndpointId,0);
        }
            break;
        }
        case app::Clusters::OnOff::Commands::On::Id: {
        app::Clusters::OnOff::Commands::On::DecodableType commandData;
        TLVError = DataModel::Decode(aDataTlv, commandData);
        if (TLVError == CHIP_NO_ERROR) {
		BridgedOnOff(aCommandPath.mEndpointId,1);

        }
            break;
        }
        case app::Clusters::OnOff::Commands::Toggle::Id: {
        app::Clusters::OnOff::Commands::Toggle::DecodableType commandData;
        TLVError = DataModel::Decode(aDataTlv, commandData);
        if (TLVError == CHIP_NO_ERROR) {
		BridgedOnOff(aCommandPath.mEndpointId,2);

        }
            break;
        }
        default: {
            return CHIP_NO_ERROR;
        }
        }
	return CHIP_NO_ERROR;
}

CHIP_ERROR ProcessLevelControlClusterCommand(const chip::app::ConcreteCommandPath & aCommandPath,const chip::TLV::TLVReader & commandDataReader)
{
	 CHIP_ERROR TLVError = CHIP_NO_ERROR;
	 chip::TLV::TLVReader aDataTlv(commandDataReader);
	 switch (aCommandPath.mCommandId)
        {
        case app::Clusters::LevelControl::Commands::MoveToLevel::Id: {
        app::Clusters::LevelControl::Commands::MoveToLevel::DecodableType commandData;
        TLVError = DataModel::Decode(aDataTlv, commandData); 
        if (TLVError == CHIP_NO_ERROR) {
//			PRINTF("\n ### Move to Level : Level=%d,TransTime=%d,EP=%d\n",commandData.level,commandData.transitionTime.Value(),aCommandPath.mEndpointId);
	      BridgedLevelControl(aCommandPath.mEndpointId,commandData.level,commandData.transitionTime.Value());
        }
            break;
        }
        case app::Clusters::LevelControl::Commands::Move::Id: {
        app::Clusters::LevelControl::Commands::Move::DecodableType commandData;
        TLVError = DataModel::Decode(aDataTlv, commandData);
        if (TLVError == CHIP_NO_ERROR) {
        }
            break;
        }
        case app::Clusters::LevelControl::Commands::Step::Id: {
        app::Clusters::LevelControl::Commands::Step::DecodableType commandData;
        TLVError = DataModel::Decode(aDataTlv, commandData);
        if (TLVError == CHIP_NO_ERROR) {
        }
            break;
        }
        case app::Clusters::LevelControl::Commands::Stop::Id: {
        app::Clusters::LevelControl::Commands::Stop::DecodableType commandData;
        TLVError = DataModel::Decode(aDataTlv, commandData);
        if (TLVError == CHIP_NO_ERROR) {
        }
            break;
        }
        case app::Clusters::LevelControl::Commands::MoveToLevelWithOnOff::Id: {
        app::Clusters::LevelControl::Commands::MoveToLevelWithOnOff::DecodableType commandData;
        TLVError = DataModel::Decode(aDataTlv, commandData);
        if (TLVError == CHIP_NO_ERROR) {
        }
            break;
        }
        case app::Clusters::LevelControl::Commands::MoveWithOnOff::Id: {
        app::Clusters::LevelControl::Commands::MoveWithOnOff::DecodableType commandData;
        TLVError = DataModel::Decode(aDataTlv, commandData);
        if (TLVError == CHIP_NO_ERROR) {
        }
            break;
        }
        case app::Clusters::LevelControl::Commands::StepWithOnOff::Id: {
        app::Clusters::LevelControl::Commands::StepWithOnOff::DecodableType commandData;
        TLVError = DataModel::Decode(aDataTlv, commandData);
        if (TLVError == CHIP_NO_ERROR) {
        }
            break;
        }
        case app::Clusters::LevelControl::Commands::StopWithOnOff::Id: {
        app::Clusters::LevelControl::Commands::StopWithOnOff::DecodableType commandData;
        TLVError = DataModel::Decode(aDataTlv, commandData);
        if (TLVError == CHIP_NO_ERROR) {
        }
            break;
        }
        default: {
            return CHIP_NO_ERROR;
        }
        }
	return CHIP_NO_ERROR;
}

CHIP_ERROR ProcessColorControlClusterCommand(const chip::app::ConcreteCommandPath & aCommandPath,const chip::TLV::TLVReader & commandDataReader)
{
       CHIP_ERROR TLVError = CHIP_NO_ERROR;
	chip::TLV::TLVReader aDataTlv(commandDataReader);
	switch (aCommandPath.mCommandId)
	{
        case app::Clusters::ColorControl::Commands::MoveToHue::Id: {
            app::Clusters::ColorControl::Commands::MoveToHue::DecodableType commandData;
            TLVError = DataModel::Decode(aDataTlv, commandData);
            if (TLVError == CHIP_NO_ERROR)
            {
            PRINTF("\n ### Move to Hue : Hue=0x%x,Dir=%d,TransTime=%d,EP=%d",commandData.hue,commandData.direction,commandData.transitionTime,aCommandPath.mEndpointId);
			BridgedMoveToHue(aCommandPath.mEndpointId,commandData.hue,(uint8_t)(commandData.direction),commandData.transitionTime);
            }
            break;
        }
        case app::Clusters::ColorControl::Commands::MoveHue::Id: {
            app::Clusters::ColorControl::Commands::MoveHue::DecodableType commandData;
            TLVError = DataModel::Decode(aDataTlv, commandData);
            if (TLVError == CHIP_NO_ERROR)
            {
            }
            break;
        }
        case app::Clusters::ColorControl::Commands::StepHue::Id: {
            app::Clusters::ColorControl::Commands::StepHue::DecodableType commandData;
            TLVError = DataModel::Decode(aDataTlv, commandData);
            if (TLVError == CHIP_NO_ERROR)
            {
            }
            break;
        }
        case app::Clusters::ColorControl::Commands::MoveToSaturation::Id: {
            app::Clusters::ColorControl::Commands::MoveToSaturation::DecodableType commandData;
            TLVError = DataModel::Decode(aDataTlv, commandData);
            if (TLVError == CHIP_NO_ERROR)
            {
				PRINTF("\n ### Move to Saturation : Sat=0x%x,TransTime=%d,EP=%d\n",commandData.saturation,commandData.transitionTime,aCommandPath.mEndpointId);
				BridgedMoveToSaturation(aCommandPath.mEndpointId,commandData.saturation,commandData.transitionTime);
            }
            break;
        }
        case app::Clusters::ColorControl::Commands::MoveSaturation::Id: {
            app::Clusters::ColorControl::Commands::MoveSaturation::DecodableType commandData;
            TLVError = DataModel::Decode(aDataTlv, commandData);
            if (TLVError == CHIP_NO_ERROR)
            {
            }
            break;
        }
        case app::Clusters::ColorControl::Commands::StepSaturation::Id: {
            app::Clusters::ColorControl::Commands::StepSaturation::DecodableType commandData;
            TLVError = DataModel::Decode(aDataTlv, commandData);
            if (TLVError == CHIP_NO_ERROR)
            {
            }
            break;
        }
        case app::Clusters::ColorControl::Commands::MoveToHueAndSaturation::Id: {
            app::Clusters::ColorControl::Commands::MoveToHueAndSaturation::DecodableType commandData;
            TLVError = DataModel::Decode(aDataTlv, commandData);
            if (TLVError == CHIP_NO_ERROR)
            {
            }
            break;
        }
        case app::Clusters::ColorControl::Commands::MoveToColor::Id: {
            app::Clusters::ColorControl::Commands::MoveToColor::DecodableType commandData;
            TLVError = DataModel::Decode(aDataTlv, commandData);
            if (TLVError == CHIP_NO_ERROR)
            {
				PRINTF("\n ### Move to Color : X=%d,Y=%d,TransTime=%d,EP=%d\n",commandData.colorX,commandData.colorY,commandData.transitionTime,aCommandPath.mEndpointId);
				BridgedMoveToColor(aCommandPath.mEndpointId,commandData.colorX,commandData.colorY,commandData.transitionTime);
            }
            break;
        }
        case app::Clusters::ColorControl::Commands::MoveColor::Id: {
            app::Clusters::ColorControl::Commands::MoveColor::DecodableType commandData;
            TLVError = DataModel::Decode(aDataTlv, commandData);
            if (TLVError == CHIP_NO_ERROR)
            {
            }
            break;
        }
        case app::Clusters::ColorControl::Commands::StepColor::Id: {
            app::Clusters::ColorControl::Commands::StepColor::DecodableType commandData;
            TLVError = DataModel::Decode(aDataTlv, commandData);
            if (TLVError == CHIP_NO_ERROR)
            {
            }
            break;
        }
        case app::Clusters::ColorControl::Commands::MoveToColorTemperature::Id: {
            app::Clusters::ColorControl::Commands::MoveToColorTemperature::DecodableType commandData;
            TLVError = DataModel::Decode(aDataTlv, commandData);
            if (TLVError == CHIP_NO_ERROR)
            {
				PRINTF("\n ### Move to Temperature : Temp=0x%x,TransTime=%d,EP=%d\n",commandData.colorTemperatureMireds,commandData.transitionTime,aCommandPath.mEndpointId);
				BridgedMoveToColorTemperature(aCommandPath.mEndpointId,commandData.colorTemperatureMireds,commandData.transitionTime);
	     	}
            break;
        }
        case app::Clusters::ColorControl::Commands::EnhancedMoveToHue::Id: {
            app::Clusters::ColorControl::Commands::EnhancedMoveToHue::DecodableType commandData;
            TLVError = DataModel::Decode(aDataTlv, commandData);
            if (TLVError == CHIP_NO_ERROR)
            {
            }
            break;
        }
        case app::Clusters::ColorControl::Commands::EnhancedMoveHue::Id: {
            app::Clusters::ColorControl::Commands::EnhancedMoveHue::DecodableType commandData;
            TLVError = DataModel::Decode(aDataTlv, commandData);
            if (TLVError == CHIP_NO_ERROR)
            {
            }
            break;
        }
        case app::Clusters::ColorControl::Commands::EnhancedStepHue::Id: {
            app::Clusters::ColorControl::Commands::EnhancedStepHue::DecodableType commandData;
            TLVError = DataModel::Decode(aDataTlv, commandData);
            if (TLVError == CHIP_NO_ERROR)
            {
            }
            break;
        }
        case app::Clusters::ColorControl::Commands::EnhancedMoveToHueAndSaturation::Id: {
            app::Clusters::ColorControl::Commands::EnhancedMoveToHueAndSaturation::DecodableType commandData;
            TLVError = DataModel::Decode(aDataTlv, commandData);
            if (TLVError == CHIP_NO_ERROR)
            {
            }
            break;
        }
        case app::Clusters::ColorControl::Commands::ColorLoopSet::Id: {
            app::Clusters::ColorControl::Commands::ColorLoopSet::DecodableType commandData;
            TLVError = DataModel::Decode(aDataTlv, commandData);
            if (TLVError == CHIP_NO_ERROR)
            {
            }
            break;
        }
        case app::Clusters::ColorControl::Commands::StopMoveStep::Id: {
            app::Clusters::ColorControl::Commands::StopMoveStep::DecodableType commandData;
            TLVError = DataModel::Decode(aDataTlv, commandData);
            if (TLVError == CHIP_NO_ERROR)
            {
            }
            break;
        }
        case app::Clusters::ColorControl::Commands::MoveColorTemperature::Id: {
            app::Clusters::ColorControl::Commands::MoveColorTemperature::DecodableType commandData;
            TLVError = DataModel::Decode(aDataTlv, commandData);      
            if (TLVError == CHIP_NO_ERROR)
            {
            }
            break;
        }
        case app::Clusters::ColorControl::Commands::StepColorTemperature::Id: {
            app::Clusters::ColorControl::Commands::StepColorTemperature::DecodableType commandData;
            TLVError = DataModel::Decode(aDataTlv, commandData);
            if (TLVError == CHIP_NO_ERROR)
            {
            }
            break;
        }
        default: {
            return CHIP_NO_ERROR;
        }
        }
	return CHIP_NO_ERROR;
}

CHIP_ERROR MatterPreCommandReceivedCallback(const chip::app::ConcreteCommandPath & commandPath,const chip::Access::SubjectDescriptor & subjectDescriptor,const chip::TLV::TLVReader & commandDataReader)
{
	CHIP_ERROR err = CHIP_NO_ERROR;

       switch(commandPath.mClusterId)
       {
        case Clusters::OnOff::Id:
       		err = ProcessOnOffClusterCommand(commandPath, commandDataReader);
			break;
		case Clusters::LevelControl::Id:
			err = ProcessLevelControlClusterCommand(commandPath, commandDataReader);
			break;
		case Clusters::ColorControl::Id:
			err = ProcessColorControlClusterCommand(commandPath, commandDataReader);
			break;
		default :
			break;
	   }

	return err;
}
