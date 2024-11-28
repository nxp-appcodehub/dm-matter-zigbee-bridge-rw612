/*
 * Copyright 2021-2023 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ZIGBEE_CMD_H
#define ZIGBEE_CMD_H

#if defined __cplusplus
	 extern "C" {
#endif

/** Address mode of zigbee */
typedef enum
{
    E_AM_BOUND,
    E_AM_GROUP,
    E_AM_SHORT,
} teAddr_Mode;

/* Request type*/
typedef enum
{
    E_RT_SINGLE,
    E_RT_EXTENDED,
} teRequest_Type;

#define ZB_DEVICE_OTA_IMAGE_PATH_MAX_LENGTH      48

void ZigBeeCmdRegister(void);

uint64_t simple_strtoul(const char *cp, char **endp, uint8_t base);
bool addr_valid(const char *cp, uint8_t base);

#if defined __cplusplus
}
#endif

#endif /* ZIGBEE_CMD_H_ */
