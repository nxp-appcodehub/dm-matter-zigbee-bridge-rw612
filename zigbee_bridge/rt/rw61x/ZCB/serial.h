/*
 * Copyright 2021-2023 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SERIAL_H
#define SERIAL_H


#if defined __cplusplus
extern "C" {
#endif


/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/ 
 
/*******************************************************************************
 * Enumeration
 ******************************************************************************/

typedef enum
{
    E_SERIAL_OK,
    E_SERIAL_ERROR,
    E_SERIAL_NODATA,
} teSerial_Status;

/*******************************************************************************
 * Prototypes
 ******************************************************************************/


void eSerial_Init(void);
teSerial_Status eSerial_Read(uint8_t *data);
void eSerial_WriteBuffer(uint8_t *data, uint8_t length);


#if defined __cplusplus
}
#endif


#endif
