/*
 * Copyright 2025 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */
/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

#include <jendefs.h>
#include <pdum_nwk.h>
#include <pdum_apl.h>


/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

struct pdum_tsAPdu_tag {
    struct pdum_tsAPduInstance_tag *psAPduInstances;
    uint16 u16FreeListHeadIdx;
    uint16 u16Size;
    uint16 u16NumInstances;
};

struct pdum_tsAPduInstance_tag {
    uint8 *au8Storage;
    uint16 u16Size;
    uint16 u16NextAPduInstIdx;
    uint16 u16APduIdx;
};

typedef struct pdum_tsAPduInstance_tag pdum_tsAPduInstance;
typedef struct pdum_tsAPdu_tag pdum_tsAPdu;


/****************************************************************************/
/***        Function Prototypes                                           ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/


/* NPDU Pool */
PRIVATE pdum_tsNPdu s_asNPduPool[24];

/* APDU Pool */
PRIVATE uint8 s_au8apduZDPInstance0Storage[100];
PRIVATE uint8 s_au8apduZDPInstance1Storage[100];
PRIVATE uint8 s_au8apduZDPInstance2Storage[100];
PUBLIC pdum_tsAPduInstance s_asapduZDPInstances[3] = {
    { s_au8apduZDPInstance0Storage, 0xFFFF, 0, 0 },
    { s_au8apduZDPInstance1Storage, 0xFFFF, 0, 0 },
    { s_au8apduZDPInstance2Storage, 0xFFFF, 0, 0 },
};
PRIVATE uint8 s_au8apduZCLInstance0Storage[100];
PRIVATE uint8 s_au8apduZCLInstance1Storage[100];
PRIVATE uint8 s_au8apduZCLInstance2Storage[100];
PRIVATE uint8 s_au8apduZCLInstance3Storage[100];
PRIVATE uint8 s_au8apduZCLInstance4Storage[100];
PRIVATE uint8 s_au8apduZCLInstance5Storage[100];
PRIVATE uint8 s_au8apduZCLInstance6Storage[100];
PRIVATE uint8 s_au8apduZCLInstance7Storage[100];
PRIVATE uint8 s_au8apduZCLInstance8Storage[100];
PRIVATE uint8 s_au8apduZCLInstance9Storage[100];
PUBLIC pdum_tsAPduInstance s_asapduZCLInstances[10] = {
    { s_au8apduZCLInstance0Storage, 0xFFFF, 0, 1 },
    { s_au8apduZCLInstance1Storage, 0xFFFF, 0, 1 },
    { s_au8apduZCLInstance2Storage, 0xFFFF, 0, 1 },
    { s_au8apduZCLInstance3Storage, 0xFFFF, 0, 1 },
    { s_au8apduZCLInstance4Storage, 0xFFFF, 0, 1 },
    { s_au8apduZCLInstance5Storage, 0xFFFF, 0, 1 },
    { s_au8apduZCLInstance6Storage, 0xFFFF, 0, 1 },
    { s_au8apduZCLInstance7Storage, 0xFFFF, 0, 1 },
    { s_au8apduZCLInstance8Storage, 0xFFFF, 0, 1 },
    { s_au8apduZCLInstance9Storage, 0xFFFF, 0, 1 },
};

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

extern pdum_tsAPdu s_asAPduPool[2];

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

extern void pdum_vNPduInit(pdum_tsNPdu *psNPduPool, uint16 u16Size);
extern void pdum_vAPduInit(pdum_tsAPdu *asAPduPool, uint16 u16NumAPdus);

#define PDUM_ALLOC_IDX  (0xeeee)    /* indicates APDU instance has been allocated */
PUBLIC uint16 PDUM_u16APduGetCrtUse(PDUM_thAPdu hAPdu)
{
    pdum_tsAPdu *psAPdu = (pdum_tsAPdu *)hAPdu;
    int i, c = 0;

    if (psAPdu->u16NumInstances > 0)
    {
        for (i = 0; i < psAPdu->u16NumInstances; i++)
        {
            if(psAPdu->psAPduInstances[i].u16NextAPduInstIdx == PDUM_ALLOC_IDX)
            {
                c++;
            }
        }
    }

    return c;
}

PUBLIC uint16 PDUM_u16APduGetMaxUse(PDUM_thAPdu hAPdu)
{
    pdum_tsAPdu *psAPdu = (pdum_tsAPdu *)hAPdu;
    int i, c = 0;

    if (psAPdu->u16NumInstances > 0)
    {
        for (i = psAPdu->u16NumInstances - 1; i >= 0; i--)
        {
            if (psAPdu->psAPduInstances[i].u16Size == 0xFFFF)
            {
                c++;
            }
            else
            {
                break;
            }
        }
    }

    return psAPdu->u16NumInstances - c;
}

PUBLIC void PDUM_vInit(void)
{
    uint32 i;
    for (i =0; i < 2; i++) { 
        s_asAPduPool[i].u16FreeListHeadIdx = 0;
    }
    pdum_vNPduInit(s_asNPduPool, 24);
    pdum_vAPduInit(s_asAPduPool, 2);
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/