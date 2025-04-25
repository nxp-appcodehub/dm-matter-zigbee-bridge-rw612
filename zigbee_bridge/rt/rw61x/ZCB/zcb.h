/*
 * Copyright 2021-2023 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef  ZCB_H
#define  ZCB_H

#include <stdint.h>

#include <sys/time.h>

#include "ZigbeeConstant.h"

#include "newDb.h"

#if defined __cplusplus
extern "C" {
#endif


// ------------------------------------------------------------------
// Endpoints
// ------------------------------------------------------------------

#define ZB_ENDPOINT_ZHA             1   // ZigBee-HA
#define ZB_ENDPOINT_ONOFF           1   // ON/OFF cluster
#define ZB_ENDPOINT_GROUP           1   // GROUP cluster
#define ZB_ENDPOINT_SCENE           1   // SCENE cluster
#define ZB_ENDPOINT_SIMPLE          1   // SimpleDescriptor cluster
#define ZB_ENDPOINT_LAMP            1   // ON/OFF, Color control
#define ZB_ENDPOINT_TUNNEL          1   // For tunnel messages
#define ZB_ENDPOINT_ATTR            1   // For attrs

#define ZB_ENDPOINT_SRC_DEFAULT          1
#define ZB_ENDPOINT_DST_DEFAULT          1
#define ZB_MANU_CODE_DEFAULT             0x1037
#define ZB_COORDINATOR_SHORT_ADDRESS     0x0000

#define SEND_DIR_FROM_CLIENT_TO_SERVER   0
#define SEND_DIR_FROM_SERVER_TO_CLIENT   1

#define MANUFACTURER_SPECIFIC_FALSE      0
#define MANUFACTURER_SPECIFIC_TRUE       1

#define ATTRIBUTE_DIR_TX_SERVER          0
#define ATTRIBUTE_DIR_RX_CLIENT          1

#define SEND_UNBIND_REQUEST_COMMAND      0
#define SEND_BIND_REQUEST_COMMAND        1


/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/** Enumerated type of statuses - This fits in with the Zigbee ZCL status codes */
typedef enum
{
    /* Zigbee ZCL status codes */
    E_ZCB_OK                            = 0x00,
    E_ZCB_ERROR                         = 0x01,
    
    /* ZCB internal status codes */
    E_ZCB_ERROR_NO_MEM                  = 0x10,
    E_ZCB_COMMS_FAILED                  = 0x11,
    E_ZCB_UNKNOWN_NODE                  = 0x12,
    E_ZCB_UNKNOWN_ENDPOINT              = 0x13,
    E_ZCB_UNKNOWN_CLUSTER               = 0x14,
    E_ZCB_REQUEST_NOT_ACTIONED          = 0x15,
    
    /* Zigbee ZCL status codes */
    E_ZCB_NOT_AUTHORISED                = 0x7E, 
    E_ZCB_RESERVED_FIELD_NZERO          = 0x7F,
    E_ZCB_MALFORMED_COMMAND             = 0x80,
    E_ZCB_UNSUP_CLUSTER_COMMAND         = 0x81,
    E_ZCB_UNSUP_GENERAL_COMMAND         = 0x82,
    E_ZCB_UNSUP_MANUF_CLUSTER_COMMAND   = 0x83,
    E_ZCB_UNSUP_MANUF_GENERAL_COMMAND   = 0x84,
    E_ZCB_INVALID_FIELD                 = 0x85,
    E_ZCB_UNSUP_ATTRIBUTE               = 0x86,
    E_ZCB_INVALID_VALUE                 = 0x87,
    E_ZCB_READ_ONLY                     = 0x88,
    E_ZCB_INSUFFICIENT_SPACE            = 0x89,
    E_ZCB_DUPLICATE_EXISTS              = 0x8A,
    E_ZCB_NOT_FOUND                     = 0x8B,
    E_ZCB_UNREPORTABLE_ATTRIBUTE        = 0x8C,
    E_ZCB_INVALID_DATA_TYPE             = 0x8D,
    E_ZCB_INVALID_SELECTOR              = 0x8E,
    E_ZCB_WRITE_ONLY                    = 0x8F,
    E_ZCB_INCONSISTENT_STARTUP_STATE    = 0x90,
    E_ZCB_DEFINED_OUT_OF_BAND           = 0x91,
    E_ZCB_INCONSISTENT                  = 0x92,
    E_ZCB_ACTION_DENIED                 = 0x93,
    E_ZCB_TIMEOUT                       = 0x94,
    
    E_ZCB_HARDWARE_FAILURE              = 0xC0,
    E_ZCB_SOFTWARE_FAILURE              = 0xC1,
    E_ZCB_CALIBRATION_ERROR             = 0xC2,
} teZcbStatus;

/** Union type for all Zigbee attribute data types */
typedef union
{
    uint8_t     u8Data;
    uint16_t    u16Data;
    uint32_t    u32Data;
    uint64_t    u64Data;
} tuZcbAttributeData;


/** Enumerated type of module modes */
typedef enum
{
    E_MODE_COORDINATOR      = 0,        /**< Start module as a coordinator */
    E_MODE_ROUTER           = 1,        /**< Start module as a router */
    E_MODE_HA_COMPATABILITY = 2,        /**< Start module as router in HA compatability mode */
} teModuleMode;


/** Enumerated type of allowable channels */
typedef enum
{
    E_CHANNEL_AUTOMATIC     = 0,
    E_CHANNEL_MINIMUM       = 11,
    E_CHANNEL_MAXIMUM       = 26
} teChannel;


/** Enumerated type of supported authorisation schemes */
typedef enum
{
    E_AUTH_SCHEME_NONE,
    E_AUTH_SCHEME_RADIUS_PAP,
} teAuthScheme;


/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

uint16_t zcbNodeGetShortAddress( char * mac );
uint64_t zcbNodeGetExtendedAddress( uint16_t shortAddress );
// int32_t eGetzcbNodeNum(void);

/** Initialise control bridge connected to serial port */
void eZCB_Init(void);
/** Finished with control bridge - call this to tidy up */
teZcbStatus eZCB_Finish(void);
teZcbStatus eZCB_GetCoordinatorVersion(void);

/**  ZCL Command Control  **/
teZcbStatus eOnOff(uint8_t u8AddrMode,
                   uint16_t u16Addr,
                   uint8_t u8SrcEp,
                   uint8_t u8DstEp,
                   uint8_t u8Mode);

teZcbStatus eOn_Off( uint16_t u16ShortAddress, uint8_t u8Mode );

teZcbStatus eLevelControlMove(uint8_t u8AddrMode, 
                              uint16_t u16Addr, 
                              uint8_t u8SrcEp, 
                              uint8_t u8DstEp,
                              uint8_t u8OnOff,
                              uint8_t u8Mode,
                              uint8_t u8Rate);

teZcbStatus eLevelControlMoveToLevel(uint16_t u16Addr, 
                                     //uint8_t u8OnOff,
                                     uint8_t u8Level,
                                     uint16_t u16Time);

teZcbStatus eLevelControlMoveStep(uint8_t u8AddrMode, 
                                  uint16_t u16Addr, 
                                  uint8_t u8SrcEp, 
                                  uint8_t u8DstEp,
                                  uint8_t u8OnOff,
                                  uint8_t u8Mode,
                                  uint8_t u8Size,
                                  uint16_t u16Time);

teZcbStatus eColorControlMoveToColor(uint16_t u16Addr, 
                                     uint16_t u16ColorX,
                                     uint16_t u16ColorY,
                                     uint16_t u16Time);


teZcbStatus eColorControlMoveToTemp(uint16_t u16Addr, 
                                    uint16_t u16ColorTemp,
                                    uint16_t u16Time);


teZcbStatus eColorControlMoveToHue(uint16_t u16Addr, 
                                   uint8_t u8Hue,
                                   uint8_t u8Dir,
                                   uint16_t u16Time);

teZcbStatus eColorControlMoveToSaturation(uint16_t u16Addr, 
                                   uint8_t u8Hue,
                                   uint16_t u16Time);

teZcbStatus eIASZoneEnrollResponse(uint8_t u8AddrMode, 
                                   uint16_t u16Addr, 
                                   uint8_t u8SrcEp, 
                                   uint8_t u8DstEp, 
                                   uint8_t u8EnrollRspCode,
                                   uint8_t u8ZoneId);

teZcbStatus ZCB_OtaImageNotify(uint8_t u8AddrMode, 
                               uint16_t u16Addr, 
                               uint8_t u8SrcEp, 
                               uint8_t u8DstEp,
                               char * ota_notify_hd);

teZcbStatus eZCB_Finish(void);
bool EnumJoinedNodes(void);
void UpdateJoinedNodes(uint16_t EP);
void SaveJoinedNodes(void);
void RestoreJoinedNodes(void);

void BridgedOnOff(uint16_t ep,uint8_t mode);
void BridgedLevelControl(uint16_t ep,uint8_t level,uint16_t time);
void BridgedMoveToHue(uint16_t ep,uint8_t hue,uint8_t dir,uint16_t time);
void BridgedMoveToSaturation(uint16_t ep,uint8_t sat,uint16_t time);
void BridgedMoveToColorTemperature(uint16_t ep,uint16_t temp,uint16_t time);
void BridgedMoveToColor(uint16_t ep,uint16_t x,uint16_t y,uint16_t time);
#define DEV_NUM 5

typedef struct {
	uint8_t type; //router : OnOff/Dimmable/ColorLight ; enddevice:sensor/switch
	uint16_t ep;
    uint16_t shortaddr; //16bit short addr
    uint64_t mac;//64bit 
} NodeDB;

typedef struct {
	uint8_t totalNodes;
	NodeDB joinedNodes[DEV_NUM];
} JoinedNodesSaved;

#if defined __cplusplus
}
#endif

#endif  /* ZCB_H */

// ------------------------------------------------------------------
// End of file
// ------------------------------------------------------------------

