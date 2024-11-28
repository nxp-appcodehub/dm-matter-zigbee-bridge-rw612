/*
 * Copyright 2021-2023 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SERIALLINK_H
#define SERIALLINK_H


#if defined __cplusplus
extern "C" {
#endif


/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define pri_ntohs(n) ((uint16_t)((((n) << 8) & 0xff00) | (((n) >> 8) & 0x00ff)))
#define pri_ntohl(n) ((uint32_t)((((n) << 24) & 0xff000000) | (((n) << 8) & 0x00ff0000) | \
                  (((n) >> 8) & 0x0000ff00) | (((n) >> 24) & 0x000000ff)))
#define pri_ntohd(n) ((uint64_t)((((n) << 56) & 0xff00000000000000) | (((n) << 40) & 0x00ff000000000000) | \
                  (((n) << 24) & 0x0000ff0000000000) | (((n) << 8) & 0x000000ff00000000) | \
                  (((n) >> 8) & 0x00000000ff000000) | (((n) >> 24) & 0x0000000000ff0000) | \
                  (((n) >> 40) & 0x000000000000ff00) | (((n) >> 56) & 0x00000000000000ff)))
                  
#define PACKED __attribute__((__packed__))
 
/*******************************************************************************
 * Variables
 ******************************************************************************/


/*******************************************************************************
 * Enumeration
 ******************************************************************************/

/** States for serial link */
typedef enum
{
    E_SL_OK,
    E_SL_ERROR,
    E_SL_NOMESSAGE,
    E_SL_ERROR_SERIAL,
    E_SL_ERROR_NOMEM,
} teSL_Status;
 

/** Serial link message types */
typedef enum
{
/* Common Commands */
    E_SL_MSG_STATUS                         =   0x8000,
    E_SL_MSG_LOG                            =   0x8001,

    E_SL_MSG_DATA_INDICATION                =   0x8002,

    E_SL_MSG_NODE_CLUSTER_LIST              =   0x8003,
    E_SL_MSG_NODE_ATTRIBUTE_LIST            =   0x8004,
    E_SL_MSG_NODE_COMMAND_ID_LIST           =   0x8005,
    E_SL_MSG_RESTART_PROVISIONED            =   0x8006,
    E_SL_MSG_RESTART_FACTORY_NEW            =   0x8007,
    E_SL_MSG_GET_VERSION                    =   0x0010,
    E_SL_MSG_VERSION_LIST                   =   0x8010,

    E_SL_MSG_SET_EXT_PANID                  =   0x0020,
    E_SL_MSG_SET_CHANNELMASK                =   0x0021,
    E_SL_MSG_SET_SECURITY                   =   0x0022,
    E_SL_MSG_SET_DEVICETYPE                 =   0x0023,
    E_SL_MSG_START_NETWORK                  =   0x0024,
    E_SL_MSG_START_SCAN                     =   0x0025,
    E_SL_MSG_NETWORK_JOINED_FORMED          =   0x8024,
    E_SL_MSG_NETWORK_REMOVE_DEVICE          =   0x0026,
    E_SL_MSG_NETWORK_WHITELIST_ENABLE       =   0x0027,
    E_SL_MSG_AUTHENTICATE_DEVICE_REQUEST    =   0x0028,
    E_SL_MSG_AUTHENTICATE_DEVICE_RESPONSE   =   0x8028,
    E_SL_MSG_OUT_OF_BAND_COMMISSIONING_DATA_REQUEST  = 0x0029,
    E_SL_MSG_OUT_OF_BAND_COMMISSIONING_DATA_RESPONSE = 0x8029,

    E_SL_MSG_RESET                          =   0x0011,
    E_SL_MSG_ERASE_PERSISTENT_DATA          =   0x0012,
    E_SL_MSG_ZLL_FACTORY_NEW                =   0x0013,
    E_SL_MSG_GET_PERMIT_JOIN                =   0x0014,
    E_SL_MSG_GET_PERMIT_JOIN_RESPONSE       =   0x8014,
    E_SL_MSG_BIND                           =   0x0030,
    E_SL_MSG_BIND_RESPONSE                  =   0x8030,
    E_SL_MSG_UNBIND                         =   0x0031,
    E_SL_MSG_UNBIND_RESPONSE                =   0x8031,

    E_SL_MSG_NETWORK_ADDRESS_REQUEST        =   0x0040,
    E_SL_MSG_NETWORK_ADDRESS_RESPONSE       =   0x8040,
    E_SL_MSG_IEEE_ADDRESS_REQUEST           =   0x0041,
    E_SL_MSG_IEEE_ADDRESS_RESPONSE          =   0x8041,
    E_SL_MSG_NODE_DESCRIPTOR_REQUEST        =   0x0042,
    E_SL_MSG_NODE_DESCRIPTOR_RESPONSE       =   0x8042,
    E_SL_MSG_SIMPLE_DESCRIPTOR_REQUEST      =   0x0043,
    E_SL_MSG_SIMPLE_DESCRIPTOR_RESPONSE     =   0x8043,
    E_SL_MSG_POWER_DESCRIPTOR_REQUEST       =   0x0044,
    E_SL_MSG_POWER_DESCRIPTOR_RESPONSE      =   0x8044,
    E_SL_MSG_ACTIVE_ENDPOINT_REQUEST        =   0x0045,
    E_SL_MSG_ACTIVE_ENDPOINT_RESPONSE       =   0x8045,
    E_SL_MSG_MATCH_DESCRIPTOR_REQUEST       =   0x0046,
    E_SL_MSG_MATCH_DESCRIPTOR_RESPONSE      =   0x8046,
    E_SL_MSG_MANAGEMENT_LEAVE_REQUEST       =   0x0047,
    E_SL_MSG_MANAGEMENT_LEAVE_RESPONSE      =   0x8047,
    E_SL_MSG_LEAVE_INDICATION               =   0x8048,
    E_SL_MSG_PERMIT_JOINING_REQUEST         =   0x0049,
    E_SL_MSG_MANAGEMENT_NETWORK_UPDATE_REQUEST  = 0x004A,
    E_SL_MSG_MANAGEMENT_NETWORK_UPDATE_RESPONSE = 0x804A,
    E_SL_MSG_SYSTEM_SERVER_DISCOVERY        =   0x004B,
    E_SL_MSG_SYSTEM_SERVER_DISCOVERY_RESPONSE = 0x804B,
    E_SL_MSG_DEVICE_ANNOUNCE                =   0x004D,
    E_SL_MSG_MANAGEMENT_LQI_REQUEST         =   0x004E,
    E_SL_MSG_MANAGEMENT_LQI_RESPONSE        =   0x804E,

    /* Group Cluster */
    E_SL_MSG_ADD_GROUP_REQUEST              =   0x0060,
    E_SL_MSG_ADD_GROUP_RESPONSE             =   0x8060,
    E_SL_MSG_VIEW_GROUP_REQUEST             =   0x0061,
    E_SL_MSG_VIEW_GROUP_RESPONSE            =   0x8061,
    E_SL_MSG_GET_GROUP_MEMBERSHIP           =   0x0062,
    E_SL_MSG_GET_GROUP_MEMBERSHIP_RESPONSE  =   0x8062,
    E_SL_MSG_REMOVE_GROUP_REQUEST           =   0x0063,
    E_SL_MSG_REMOVE_GROUP_RESPONSE          =   0x8063,
    E_SL_MSG_REMOVE_ALL_GROUPS              =   0x0064,
    E_SL_MSG_ADD_GROUP_IF_IDENTIFY          =   0x0065,

    /* Identify Cluster */
    E_SL_MSG_IDENTIFY_SEND                  =   0x0070,
    E_SL_MSG_IDENTIFY_QUERY                 =   0x0071,

    /* Level Cluster */
    E_SL_MSG_MOVE_TO_LEVEL                  =   0x0080,
    E_SL_MSG_MOVE_TO_LEVEL_ONOFF            =   0x0081,
    E_SL_MSG_MOVE_STEP                      =   0x0082,
    E_SL_MSG_MOVE_STOP_MOVE                 =   0x0083,
    E_SL_MSG_MOVE_STOP_ONOFF                =   0x0084,

    /* Scenes Cluster */
    E_SL_MSG_VIEW_SCENE                     =   0x00A0,
    E_SL_MSG_VIEW_SCENE_RESPONSE            =   0x80A0,
    E_SL_MSG_ADD_SCENE                      =   0x00A1,
    E_SL_MSG_ADD_SCENE_RESPONSE             =   0x80A1,
    E_SL_MSG_REMOVE_SCENE                   =   0x00A2,
    E_SL_MSG_REMOVE_SCENE_RESPONSE          =   0x80A2,
    E_SL_MSG_REMOVE_ALL_SCENES              =   0x00A3,
    E_SL_MSG_REMOVE_ALL_SCENES_RESPONSE     =   0x80A3,
    E_SL_MSG_STORE_SCENE                    =   0x00A4,
    E_SL_MSG_STORE_SCENE_RESPONSE           =   0x80A4,
    E_SL_MSG_RECALL_SCENE                   =   0x00A5,
    E_SL_MSG_SCENE_MEMBERSHIP_REQUEST       =   0x00A6,
    E_SL_MSG_SCENE_MEMBERSHIP_RESPONSE      =   0x80A6,

    /* Colour Cluster */
    E_SL_MSG_MOVE_TO_HUE                    =   0x00B0,
    E_SL_MSG_MOVE_HUE                       =   0x00B1,
    E_SL_MSG_STEP_HUE                       =   0x00B2,
    E_SL_MSG_MOVE_TO_SATURATION             =   0x00B3,
    E_SL_MSG_MOVE_SATURATION                =   0x00B4,
    E_SL_MSG_STEP_SATURATION                =   0x00B5,
    E_SL_MSG_MOVE_TO_HUE_SATURATION         =   0x00B6,
    E_SL_MSG_MOVE_TO_COLOUR                 =   0x00B7,
    E_SL_MSG_MOVE_COLOUR                    =   0x00B8,
    E_SL_MSG_STEP_COLOUR                    =   0x00B9,

/* ZLL Commands */
    /* Touchlink */
    E_SL_MSG_INITIATE_TOUCHLINK             =   0x00D0,
    E_SL_MSG_TOUCHLINK_STATUS               =   0x00D1,
    E_SL_MSG_TOUCHLINK_FACTORY_RESET        =   0x00D2,

    /* Identify Cluster */
    E_SL_MSG_IDENTIFY_TRIGGER_EFFECT        =   0x00E0,

    /* On/Off Cluster */
    E_SL_MSG_ONOFF                          =   0x0092,
    E_SL_MSG_ONOFF_TIMED                    =   0x0093,
    E_SL_MSG_ONOFF_EFFECTS                  =   0x0094,
    E_SL_MSG_ONOFF_UPDATE                   =   0x8095,

    /* Scenes Cluster */
    E_SL_MSG_ADD_ENHANCED_SCENE             =   0x00A7,
    E_SL_MSG_VIEW_ENHANCED_SCENE            =   0x00A8,
    E_SL_MSG_COPY_SCENE                     =   0x00A9,

    /* Colour Cluster */
    E_SL_MSG_ENHANCED_MOVE_TO_HUE           =   0x00BA,
    E_SL_MSG_ENHANCED_MOVE_HUE              =   0x00BB,
    E_SL_MSG_ENHANCED_STEP_HUE              =   0x00BC,
    E_SL_MSG_ENHANCED_MOVE_TO_HUE_SATURATION =  0x00BD,
    E_SL_MSG_COLOUR_LOOP_SET                =   0x00BE,
    E_SL_MSG_STOP_MOVE_STEP                 =   0x00BF,
    E_SL_MSG_MOVE_TO_COLOUR_TEMPERATURE     =   0x00C0,
    E_SL_MSG_MOVE_COLOUR_TEMPERATURE        =   0x00C1,
    E_SL_MSG_STEP_COLOUR_TEMPERATURE        =   0x00C2,

/* ZHA Commands */
    /* Door Lock Cluster */
    E_SL_MSG_LOCK_UNLOCK_DOOR                   =      0x00F0,

    /* Attributes */
    E_SL_MSG_READ_ATTRIBUTE_REQUEST             =      0x0100,
    E_SL_MSG_READ_ATTRIBUTE_RESPONSE            =      0x8100,
    E_SL_MSG_DEFAULT_RESPONSE                   =      0x8101,
    E_SL_MSG_ATTRIBUTE_REPORT                   =      0x8102,
    E_SL_MSG_WRITE_ATTRIBUTE_REQUEST            =      0x0110,
    E_SL_MSG_WRITE_ATTRIBUTE_RESPONSE           =      0x8110,
    E_SL_MSG_CONFIG_REPORTING_REQUEST           =      0x0120,
    E_SL_MSG_CONFIG_REPORTING_RESPONSE          =      0x8120,
    E_SL_MSG_REPORT_ATTRIBUTES                  =      0x8121,
    E_SL_MSG_ATTRIBUTE_DISCOVERY_REQUEST        =      0x0140,
    E_SL_MSG_ATTRIBUTE_DISCOVERY_RESPONSE       =      0x8140,

    /* Persistant data manager messages */
    E_SL_MSG_PDM_AVAILABLE_REQUEST              =      0x0300,
    E_SL_MSG_PDM_AVAILABLE_RESPONSE             =      0x8300,
    E_SL_MSG_PDM_SAVE_RECORD_REQUEST            =      0x0200,
    E_SL_MSG_PDM_SAVE_RECORD_RESPONSE           =      0x8200,
    E_SL_MSG_PDM_LOAD_RECORD_REQUEST            =      0x0201,
    E_SL_MSG_PDM_LOAD_RECORD_RESPONSE           =      0x8201,
    E_SL_MSG_PDM_DELETE_ALL_RECORDS_REQUEST     =      0x0202,
    E_SL_MSG_PDM_DELETE_ALL_RECORDS_RESPONSE    =      0x8202,

    /* Appliance Statistics Cluster 0x0B03 */
    // http://www.nxp.com/documents/user_manual/JN-UG-3076.pdf
    E_SL_MSG_ASC_LOG_MSG                        =      0x0301,   // Was 0x0500, was 0x0301
    E_SL_MSG_ASC_LOG_MSG_RESPONSE               =      0x8301,

    /* IAS Cluster */
    E_SL_MSG_SEND_IAS_ZONE_ENROLL_RSP			=	   0x0400,
    E_SL_MSG_IAS_ZONE_STATUS_CHANGE_NOTIFY		=	   0x8401,

    /* OTA Cluster */
    E_SL_MSG_LOAD_NEW_IMAGE                     =      0x0500,
    E_SL_MSG_BLOCK_REQUEST                      =      0x8501,
    E_SL_MSG_BLOCK_SEND                         =      0x0502,
    E_SL_MSG_UPGRADE_END_REQUEST                =      0x8503,
    E_SL_MSG_UPGRADE_END_RESPONSE               =      0x0504,
    E_SL_MSG_IMAGE_NOTIFY                       =      0x0505,
    E_SL_MSG_SEND_WAIT_FOR_DATA_PARAMS          =      0x0506,

} teSL_MsgType;


 /** Status message */
typedef struct
{
    enum
    {
        E_SL_MSG_STATUS_SUCCESS,
        E_SL_MSG_STATUS_INCORRECT_PARAMETERS,
        E_SL_MSG_STATUS_UNHANDLED_COMMAND,
        E_SL_MSG_STATUS_COMMAND_FAILED,
        E_SL_MSG_STATUS_BUSY,
        E_SL_MSG_STATUS_STACK_ALREADY_STARTED,
    } PACKED eStatus;
    uint8_t             u8SequenceNo;           /**< Sequence number of outgoing message */
    uint16_t            u16MessageType;         /**< Type of message that this is status to */
    char                acMessage[];            /**< Optional message */
} PACKED tsSL_Msg_Status;


typedef void (*tprSL_MessageCallback)(void *pvUser, uint16_t u16Length, void *pvMessage);


 /*******************************************************************************
 * Prototypes
 ******************************************************************************/
 
teSL_Status eSL_Init(void);
teSL_Status eSL_AddListener(uint16_t u16Type, tprSL_MessageCallback prCallback, void *pvUser);
teSL_Status eSL_SendMessage(uint16_t u16Type, uint16_t u16Length, void *pvMessage, uint8_t *pu8SequenceNo);
teSL_Status eSL_SendMessageNoWait(uint16_t u16Type, uint16_t u16Length, void *pvMessage, uint8_t *pu8SequenceNo);
teSL_Status eSL_MessageWait(uint16_t u16Type, uint32_t u32WaitTimeout, uint16_t *pu16Length, void **ppvMessage);


#if defined __cplusplus
}
#endif


#endif /* SERIALLINK_H */
