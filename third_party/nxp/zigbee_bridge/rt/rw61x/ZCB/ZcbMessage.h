/*
 * Copyright 2021-2023 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
	 
#ifndef ZCBMESSAGE_H
#define ZCBMESSAGE_H
	 
#if defined __cplusplus
	 extern "C" {
#endif

typedef enum {
    BRIDGE_UNKNOW   = 0,
    BRIDGE_ADD_DEV,
    BRIDGE_REMOVE_DEV,
    BRIDGE_FACTORY_RESET,
    BRIDGE_WRITE_ATTRIBUTE,
	BRIDGE_RESTORE_JOINED_NODE,
} msgType;

typedef struct {
    uint16_t u16ClusterID;
    uint16_t u16AttributeID;
    uint64_t u64Data;
} ZcbAttribute_t;

typedef struct {
    bool AnnounceStart;
    bool HandleMask;
	SemaphoreHandle_t bridge_mutex;
    int msg_type;
    newdb_zcb_t zcb;
    void *msg_data;
} ZcbMsg_t;

extern ZcbMsg_t ZcbMsg;

#if defined __cplusplus
}
#endif

#endif
