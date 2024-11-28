/*
 * Copyright 2021-2023 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
 
#ifndef CMD_H
#define CMD_H


#if defined __cplusplus
extern "C" {
#endif

#include "zcb.h"


/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

typedef enum 
{
    E_CLD_OTA_QUERY_JITTER,
    E_CLD_OTA_MANUFACTURER_ID_AND_JITTER,
    E_CLD_OTA_ITYPE_MDID_JITTER,
    E_CLD_OTA_ITYPE_MDID_FVERSION_JITTER
}teOTA_ImageNotifyPayloadType;


/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

int eGetPermitJoining( void );
teZcbStatus eSetPermitJoining(uint8_t u8Interval);
teZcbStatus eSetChannelMask(uint32_t u32ChannelMask);
teZcbStatus eSetDeviceType(teModuleMode eModuleMode);
teZcbStatus eStartNetwork(void);

teZcbStatus eErasePersistentData(void);
teZcbStatus eResetDevice(void);
teZcbStatus eMgmtLeaveRequst(uint16_t u16ShortAddr,
                             uint64_t u64MacAddr, 
                             uint8_t u8Rejoin, 
                             uint8_t u8RmChildren);
teZcbStatus eSetExPANID(uint64_t u64Epid);

teZcbStatus eActiveEndpointRequest(uint16_t u16ShortAddr);
teZcbStatus eNodeDescriptorRequest(uint16_t u16ShortAddr);

teZcbStatus eSimpleDescriptorRequest(uint16_t u16ShortAddr, uint8_t u8DstEp);
teZcbStatus eMatchDescriptorRequest(uint16_t u16ShortAddr,
                                    uint16_t u16ProfileId,
                                    uint8_t u8NoInputCluster,
                                    uint16_t u16InClusterId,
                                    uint8_t u8NoOutputCluster,
                                    uint16_t u16OutClusterId);

teZcbStatus eNwkAddressRequest(uint16_t u16DstShortAddr,
                               uint64_t u64InterestMacAddr,
                               uint8_t u8RstType,
                               uint8_t u8Index);

teZcbStatus eIeeeAddressRequest(uint16_t u16DstShortAddr,
                                uint16_t u16InterestShortAddr,
                                uint8_t u8RstType,
                                uint8_t u8Index);

teZcbStatus eReadAttributeRequest(uint8_t u8AddrMode, 
                                  uint16_t u16Addr, 
                                  uint8_t u8SrcEp, 
                                  uint8_t u8DstEp,
                                  uint16_t u16ClusterId,
                                  uint16_t u16ManuCode,
                                  uint8_t u8NumOfAttr,
                                  uint16_t* au16AttrList);

teZcbStatus eSendBindUnbindCommand(uint64_t u64TargetIeeeAddr,
                                   uint8_t u8TargetEp,
                                   uint16_t u16ClusterID,
                                   bool bBind);

teZcbStatus eConfigureReportingCommand(uint8_t u8AddrMode, 
                                       uint16_t u16Addr, 
                                       uint8_t u8SrcEp, 
                                       uint8_t u8DstEp,
                                       uint16_t u16ClusterId,                                       
                                       uint16_t u16ManuCode, 
                                       uint8_t u8AttributeType,
                                       uint16_t u16AttributeID,
                                       uint16_t u16Min,
                                       uint16_t u16Max,
                                       uint8_t u8Change);

teZcbStatus eOtaLoadNewImage(const char * image, bool bCoordUpgrade);

teZcbStatus eOtaImageBlockSend(uint8_t u8AddrMode, 
                               uint16_t u16Addr, 
                               uint8_t u8SrcEp, 
                               uint8_t u8DstEp,
                               uint8_t u8SequenceNumber,
                               uint8_t u8Status,
                               uint32_t u32FileOffset,
                               uint32_t u32FileVersion,
                               uint16_t u16ImageType,
                               uint16_t u16ManuCode,
                               uint8_t u8DataSize,
                               uint8_t *au8Data);

teZcbStatus eOtaUpgradeEndResponse(uint8_t u8AddrMode, 
                                   uint16_t u16Addr, 
                                   uint8_t u8SrcEp, 
                                   uint8_t u8DstEp,
                                   uint8_t u8SequenceNumber,
                                   uint32_t u32UpgradeTime,
                                   uint32_t u32CurrentTime,
                                   uint32_t u32FileVersion,
                                   uint16_t u16ImageType,
                                   uint16_t u16ManuCode);

teZcbStatus eOtaImageNotify(uint8_t u8AddrMode, 
                            uint16_t u16Addr, 
                            uint8_t u8SrcEp, 
                            uint8_t u8DstEp,
                            teOTA_ImageNotifyPayloadType ePayloadType,                                       
                            uint32_t u32FileVersion, 
                            uint16_t u16ImageType,
                            uint16_t u16ManuCode,
                            uint8_t u8QueryJitter);

teZcbStatus eOtaSendWaitForDataParams(uint8_t u8AddrMode, 
                                      uint16_t u16Addr, 
                                      uint8_t u8SrcEp, 
                                      uint8_t u8DstEp,
                                      uint8_t u8SequenceNumber,
                                      uint8_t u8Status,
                                      uint32_t u32UpgradeTime,
                                      uint32_t u32CurrentTime,
                                      uint16_t u16DelayMs);


#if defined __cplusplus
}
#endif


#endif  /* CMD_H */

