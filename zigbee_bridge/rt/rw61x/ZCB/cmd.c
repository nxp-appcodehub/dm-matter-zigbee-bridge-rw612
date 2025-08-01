/*
 * Copyright 2021-2023,2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// Include local headers
#include "cmd.h"
#include "zcb.h"
#include "ZigbeeDevices.h"
#include "fsl_os_abstraction.h"
#include "fsl_debug_console.h"

#include "jendefs.h"
#include "zps_apl.h"
#include "zps_apl_zdo.h"
#include "zps_apl_aib.h"
#include "zps_apl_af.h"
#include "zps_nwk_nib.h"
#include "bdb_api.h"
#include "zcl.h"
#include "app_common.h"
#include "app_main.h"
#include "pdum_gen.h"


// ------------------------------------------------------------------
// External prototype
// ------------------------------------------------------------------

extern tsZbNetworkInfo zbNetworkInfo;
extern uint32_t u32ChannelMask;
extern tsBDB sBDB;
extern uint8_t CONTROLBRIDGE_ZLO_ENDPOINT;
static uint8 u8SeqNum = 0;
extern OSA_MUTEX_HANDLE_DEFINE(zb_task_lock);

// ------------------------------------------------------------------
// Function implementations
// ------------------------------------------------------------------

teZcbStatus eStartFindAndBind() {
    APP_tsEvent sButtonEvent;
    sButtonEvent.eType = APP_E_EVENT_SERIAL_FIND_BIND_START;
    ZQ_bQueueSend(&APP_msgAppEvents, &sButtonEvent);
    return E_ZCB_OK;
}

teZcbStatus eSetChannelMask(uint32_t u32ChannelMask) {
    uint8_t u8Channel;
    uint32_t u32Value = u32ChannelMask;
    
    // Set the APS channel mask
    uint8_t u8Status = ZPS_eAplAibSetApsChannelMask(u32Value);
    
    if ((u32Value > 10) && (u32Value < 27)) {
        /* ChannelList is a single channel */
        u32Value = (1 << u32Value);
    }
    
    u32Value &= 0x07fff800;
    
    if (u32Value == 0) {
        /* ChannelList is 0 (or supplied channel was invalid), indicating to scan all channels */
        u32Value = 0x07fff800;
    }
    
    // Find first enabled channel and set it
    for (u8Channel = 11; u8Channel < 27; u8Channel++) {
        if (u32Value & (1 << u8Channel)) {
            ZPS_vNwkNibSetChannel(ZPS_pvAplZdoGetNwkHandle(), u8Channel);
            break;
        }
    }
    
    // Update global variables
    u32ChannelMask = u32Value;
    sBDB.sAttrib.u32bdbPrimaryChannelSet = u32Value;
    sBDB.sAttrib.u32bdbSecondaryChannelSet = 0;
    
    return E_ZCB_OK;
}

teZcbStatus eStartNetwork(void) {
    APP_tsEvent sButtonEvent;
    sButtonEvent.eType = APP_E_EVENT_SERIAL_FORM_NETWORK;
    ZQ_bQueueSend(&APP_msgAppEvents, &sButtonEvent);
    PRINTF("\n ### Starting ZB network\n");
    return E_ZCB_OK;
}
#include "dbg.h"
teZcbStatus eStartSteer(void) {
    APP_tsEvent sButtonEvent;
    sButtonEvent.eType = APP_E_EVENT_SERIAL_NWK_STEER;
    ZQ_bQueueSend(&APP_msgAppEvents, &sButtonEvent);
    PRINTF("\n ### Starting ZB steering\n");
    return E_ZCB_OK;
}

teZcbStatus eErasePersistentData(void) {

    APP_tsEvent sButtonEvent;
    sButtonEvent.eType = APP_E_EVENT_POR_FACTORY_RESET;
    ZQ_bQueueSend(&APP_msgAppEvents, &sButtonEvent);
    PRINTF("\n ### Starting ZB factory reset\n");
    return E_ZCB_OK;
}

teZcbStatus eResetDevice(void) {
    APP_tsEvent sButtonEvent;
    sButtonEvent.eType = APP_E_EVENT_POR_PDM_RESET;
    ZQ_bQueueSend(&APP_msgAppEvents, &sButtonEvent);
    PRINTF("\n ### Starting ZB reset\n");
    return E_ZCB_OK;
}

teZcbStatus eMgmtLeaveRequst(uint16_t u16ShortAddr,
                             uint64_t u64MacAddr, 
                             uint8_t u8Rejoin, 
                             uint8_t u8RmChildren) 
{
    OSA_MutexLock((osa_mutex_handle_t) zb_task_lock, osaWaitForever_c);
    PDUM_thAPduInstance    hAPduInst;
    ZPS_teStatus eStatus = ZPS_APL_APS_E_INVALID_PARAMETER;
    uint8_t u8Seq = 0;

    hAPduInst = PDUM_hAPduAllocateAPduInstance ( apduZDP );

    if (PDUM_INVALID_HANDLE != hAPduInst)
    {
        ZPS_tsAplZdpMgmtLeaveReq    sMgmtLeaveReq;
        ZPS_tuAddress               uDstAddr;

        uDstAddr.u16Addr = u16ShortAddr;

        sMgmtLeaveReq.u64DeviceAddress =  u64MacAddr;
        sMgmtLeaveReq.u8Flags          =  u8Rejoin ? (1 << 7) : 0;
        sMgmtLeaveReq.u8Flags         |=  ((u8RmChildren ? 1 : 0) << 6);

        eStatus = ZPS_eAplZdpMgmtLeaveRequest ( hAPduInst,
                                                uDstAddr,
                                                FALSE,
                                                &u8Seq,
                                                &sMgmtLeaveReq);
        if (eStatus)
        {
            PDUM_eAPduFreeAPduInstance(hAPduInst);
        }
        
    }
    OSA_MutexUnlock((osa_mutex_handle_t) zb_task_lock);
    return eStatus;
}

teZcbStatus eActiveEndpointRequest(uint16_t u16ShortAddr) {

    OSA_MutexLock((osa_mutex_handle_t) zb_task_lock, osaWaitForever_c);
    PDUM_thAPduInstance    hAPduInst;
    ZPS_teStatus eStatus = ZPS_APL_APS_E_INVALID_PARAMETER;

    hAPduInst =  PDUM_hAPduAllocateAPduInstance ( apduZDP );

    if (PDUM_INVALID_HANDLE != hAPduInst)
    {
        ZPS_tsAplZdpActiveEpReq    sActiveEpReq;
        ZPS_tuAddress              uDstAddr;

        /* always send to node of interest rather than a cache */

        uDstAddr.u16Addr                  = u16ShortAddr;
        sActiveEpReq.u16NwkAddrOfInterest =  u16ShortAddr;

        eStatus = ZPS_eAplZdpActiveEpRequest ( hAPduInst,
                                            uDstAddr,
                                            FALSE,
                                            &u8SeqNum,
                                            &sActiveEpReq );
        if (eStatus)
        {
            PDUM_eAPduFreeAPduInstance(hAPduInst);
        }
    }
    OSA_MutexUnlock((osa_mutex_handle_t) zb_task_lock);
    return eStatus;
}

teZcbStatus eSimpleDescriptorRequest(uint16_t u16ShortAddr, uint8_t u8DstEp) {
    
    OSA_MutexLock((osa_mutex_handle_t) zb_task_lock, osaWaitForever_c);
    PDUM_thAPduInstance    hAPduInst;
    ZPS_teStatus eStatus = ZPS_APL_APS_E_INVALID_PARAMETER;
    hAPduInst =  PDUM_hAPduAllocateAPduInstance ( apduZDP );

    if (PDUM_INVALID_HANDLE != hAPduInst)
    {
        ZPS_tsAplZdpSimpleDescReq    sSimpleDescReq;
        ZPS_tuAddress                uDstAddr;

        /* always send to node of interest rather than a cache */

        uDstAddr.u16Addr                    =  u16ShortAddr;
        sSimpleDescReq.u16NwkAddrOfInterest =  u16ShortAddr;
        sSimpleDescReq.u8EndPoint           =  u8DstEp;

        eStatus = ZPS_eAplZdpSimpleDescRequest ( hAPduInst,
                                              uDstAddr,
                                              FALSE,
                                              &u8SeqNum,
                                              &sSimpleDescReq );
        if (eStatus)
        {
            PDUM_eAPduFreeAPduInstance(hAPduInst);
        }
    }
    OSA_MutexUnlock((osa_mutex_handle_t) zb_task_lock);
    return eStatus;
}

teZcbStatus eIeeeAddressRequest(uint16_t u16DstShortAddr,
                                uint16_t u16InterestShortAddr,
                                uint8_t u8RstType,
                                uint8_t u8Index) {

    OSA_MutexLock((osa_mutex_handle_t) zb_task_lock, osaWaitForever_c);
    PDUM_thAPduInstance    hAPduInst;
    ZPS_teStatus eStatus = ZPS_APL_APS_E_INVALID_PARAMETER;

    hAPduInst =  PDUM_hAPduAllocateAPduInstance ( apduZDP );

    if ( PDUM_INVALID_HANDLE != hAPduInst )
    {
        ZPS_tsAplZdpIeeeAddrReq    sAplZdpIeeeAddrReq;
        ZPS_tuAddress              uDstAddr;

        /* always send to node of interest rather than a cache */
        uDstAddr.u16Addr                        =  u16DstShortAddr;
        sAplZdpIeeeAddrReq.u16NwkAddrOfInterest =  u16InterestShortAddr;
        sAplZdpIeeeAddrReq.u8RequestType        =  u8RstType;
        sAplZdpIeeeAddrReq.u8StartIndex         =  u8Index;

        eStatus = ZPS_eAplZdpIeeeAddrRequest ( hAPduInst,
                                            uDstAddr,
                                            FALSE,
                                            &u8SeqNum,
                                            &sAplZdpIeeeAddrReq );
        if (eStatus)
        {
            PDUM_eAPduFreeAPduInstance(hAPduInst);
        }
    }
    OSA_MutexUnlock((osa_mutex_handle_t) zb_task_lock);
    return eStatus;
}

teZcbStatus eReadAttributeRequest(uint8_t u8AddrMode, 
                                  uint16_t u16Addr, 
                                  uint8_t u8SrcEp, 
                                  uint8_t u8DstEp,
                                  uint16_t u16ClusterId,
                                  uint16_t u16ManuCode,
                                  uint8_t u8NumOfAttr,
                                  uint16_t* au16AttrList)
{
    OSA_MutexLock((osa_mutex_handle_t) zb_task_lock, osaWaitForever_c);
    tsZCL_Address sAddress;
    sAddress.eAddressMode = u8AddrMode;
    sAddress.uAddress.u16DestinationAddress = u16Addr;
    ZPS_teStatus eStatus = ZPS_APL_APS_E_INVALID_PARAMETER;

    eStatus = eZCL_SendReadAttributesRequest(u8SrcEp, u8DstEp, u16ClusterId, 
                                             SEND_DIR_FROM_CLIENT_TO_SERVER, &sAddress, &u8SeqNum, u8NumOfAttr, 
                                             MANUFACTURER_SPECIFIC_FALSE, u16ManuCode, au16AttrList);
    OSA_MutexUnlock((osa_mutex_handle_t) zb_task_lock);
    return eStatus;
}

teZcbStatus eSendBindUnbindCommand(uint64_t u64TargetIeeeAddr,
                                   uint8_t u8SrcEndpoint,
                                   uint16_t u16ClusterId,
                                   bool bBind) 
{
    OSA_MutexLock((osa_mutex_handle_t) zb_task_lock, osaWaitForever_c);
    ZPS_teStatus eStatus = ZPS_APL_APS_E_INVALID_PARAMETER;
    
    PDUM_thAPduInstance    hAPduInst;
    ZPS_tsAplZdpBindUnbindReq    sAplZdpBindReq;
    ZPS_tuAddress uAddr;
    uAddr.u64Addr = u64TargetIeeeAddr;

    uint8_t u8DstAddrMode                                        =  0x3;
    sAplZdpBindReq.uAddressField.sExtended.u64DstAddress =  ZPS_u64NwkNibGetExtAddr(ZPS_pvAplZdoGetNwkHandle());
    sAplZdpBindReq.uAddressField.sExtended.u8DstEndPoint =  ZB_ENDPOINT_ATTR;

    if(bBind)
    {
        eStatus = ZPS_eAplZdoBind ( u16ClusterId,
                                    u8SrcEndpoint,
                                    uAddr.u16Addr,
                                    uAddr.u64Addr,
                                    ZB_ENDPOINT_ATTR );
    }
    else
    {
        eStatus = ZPS_eAplZdoUnbind (u16ClusterId,
                                        u8SrcEndpoint,
                                        uAddr.u16Addr,
                                        uAddr.u64Addr,
                                        ZB_ENDPOINT_ATTR );
    }

    hAPduInst =  PDUM_hAPduAllocateAPduInstance ( apduZDP );

    if (PDUM_INVALID_HANDLE != hAPduInst)
    {
        ZPS_tuAddress uDstAddr;
        /* always send to node of interest rather than a cache */
        uDstAddr.u64Addr             =  u64TargetIeeeAddr;
        sAplZdpBindReq.u64SrcAddress =  u64TargetIeeeAddr;
        sAplZdpBindReq.u8SrcEndpoint =  u8SrcEndpoint;
        sAplZdpBindReq.u16ClusterId  =  u16ClusterId;
        sAplZdpBindReq.u8DstAddrMode =  u8DstAddrMode;

        eStatus = ZPS_eAplZdpBindUnbindRequest( hAPduInst,
                                                uDstAddr,
                                                TRUE,
                                                &u8SeqNum,
                                                bBind,
                                                &sAplZdpBindReq );
        if (eStatus)
        {
            PDUM_eAPduFreeAPduInstance(hAPduInst);
        }
    }
    OSA_MutexUnlock((osa_mutex_handle_t) zb_task_lock);
    return eStatus;
}

