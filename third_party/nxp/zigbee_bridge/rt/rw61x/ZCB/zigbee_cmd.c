/*
 * Copyright 2021-2023 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "shell.h" 
#include "FreeRTOS.h"
#include "task.h"
//#include "fs_cmd.h" 
//#include "lfs.h" 

#include "ZigbeeDevices.h"
#include "zigbee_cmd.h"
#include "cmd.h"
#include "zcb.h"

#include "fsl_debug_console.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define TOLOWER(x) ((x) | 0x20)

#define isxdigit(c)    (('0' <= (c) && (c) <= '9') || ('a' <= (c) && (c) <= 'f') || ('A' <= (c) && (c) <= 'F'))

#define isdigit(c)    ('0' <= (c) && (c) <= '9')

#define ZB_DEVICE_MAX_OTA_HEADER_SIZE          69
#define ZB_DEVICE_OTA_HEADER_MANU_CODE_OFFSET  10
//extern lfs_t g_lfs;


/*******************************************************************************
 * Enumeration
 ******************************************************************************/

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
static int32_t zb_nwk_form(p_shell_context_t context, int32_t argc, char **argv);
static int32_t zb_nwk_pjoin(p_shell_context_t context, int32_t argc, char **argv);
static int32_t zb_zcl_onoff(p_shell_context_t context, int32_t argc, char **argv);
static int32_t zb_zcl_level(p_shell_context_t context, int32_t argc, char **argv);
static int32_t zb_zcl_color(p_shell_context_t context, int32_t argc, char **argv);
static int32_t zb_zcl_ota(p_shell_context_t context, int32_t argc, char **argv);
static int32_t zb_zdo_bind(p_shell_context_t context, int32_t argc, char **argv);
static int32_t zb_zdo_unbind(p_shell_context_t context, int32_t argc, char **argv);
static int32_t zb_zdo_leave(p_shell_context_t context, int32_t argc, char **argv);
static int32_t zb_gen_reset(p_shell_context_t context, int32_t argc, char **argv);
static int32_t zb_gen_erasepdm(p_shell_context_t context, int32_t argc, char **argv);
static int32_t zb_zdo_ieereq(p_shell_context_t context, int32_t argc, char **argv);
static int32_t zb_zdo_nwkreq(p_shell_context_t context, int32_t argc, char **argv);
static int32_t zb_zdo_activereq(p_shell_context_t context, int32_t argc, char **argv);
static int32_t zb_zdo_nodereq(p_shell_context_t context, int32_t argc, char **argv);
static int32_t zb_zdo_simplereq(p_shell_context_t context, int32_t argc, char **argv);
static int32_t zb_coord_flash(p_shell_context_t context, int32_t argc, char **argv);

static bool is_a_endpoint(const char *cp);

/*******************************************************************************
 * Variables
 ******************************************************************************/
char g_OtaImagePath[ZB_DEVICE_OTA_IMAGE_PATH_MAX_LENGTH] = {0};

static const char zb_nwk_formHelp[] = "Usage:\r\n"
        "zb-nwk-form <Channel> <PanId>\r\n";

static const char zb_nwk_pjoinHelp[] = "Usage:\r\n"
        "zb-nwk-pjoin <Seconds>\r\n";

static const char zb_zcl_onoffHelp[] = "Usage:\r\n"
        "zb-zcl-onoff [bound|group|short] <Address> <SrcEp> <DstEp> [off|on|toggle]\r\n";

static const char zb_zcl_levelHelp[] = "Usage:\r\n"
        "zb-zcl-level   move        [bound|group|short]  <Address>  <SrcEp> <DstEp> <WithonOff> <Mode>  <Rate>\r\n"
        "zb-zcl-level   movetolevel [bound|group|short]  <Address>  <SrcEp> <DstEp> <WithonOff> <Level> <Time>\r\n"
        "zb-zcl-level   step        [bound|group|short]  <Address>  <SrcEp> <DstEp> <WithonOff> <Mode>  <Size> <Time>\r\n";

static const char zb_zcl_colorHelp[] = "Usage:\r\n"
        "zb-zcl-color movetohue    [bound|group|short]  <Address> <SrcEp> <DstEp> <Hue> <Dir>   <Time>\r\n"
        "zb-zcl-color movetocolor  [bound|group|short]  <Address> <SrcEp> <DstEp> <X>   <Y>     <Time>\r\n"
        "zb-zcl-color movetosat    [bound|group|short]  <Address> <SrcEp> <DstEp> <Saturation>  <Time>\r\n"
        "zb-zcl-color movetotemp   [bound|group|short]  <Address> <SrcEp> <DstEp> <TempK>  <Time>\r\n";

static const char zb_zcl_otaHelp[] = "Usage:\r\n"
        "zb-zcl-ota load <path>\r\n"
        "zb-zcl-ota notify [bound|group|short] <Address> <SrcEp> <DstEp>\r\n";

static const char zb_zdo_leaveHelp[] = "Usage:\r\n"
        "zb-zdo-leave <TargetAddress> <ExtendAddress> <Rejoin> <RemoveChildren>\r\n";

static const char zb_zdo_activereqHelp[] = "Usage:\r\n"
        "zb-zdo-activereq <Address>\r\n";

static const char zb_zdo_simplereqHelp[] = "Usage:\r\n"
        "zb-zdo-simplereq <Address> <DstEp>\r\n";

static const char zb_zdo_nodereqHelp[] = "Usage:\r\n"
        "zb-zdo-nodereq <Address>\r\n";

static const char zb_zdo_ieereqHelp[] = "Usage:\r\n"
        "zb-zdo-ieereq <TargetAddress> <ExtendAddress> [single|extend] <StartIndex>\r\n";

static const char zb_zdo_nwkreqHelp[] = "Usage:\r\n"
        "zb-zdo-nwkreq <TargetAddress> <LookupAddress> [single|extend] <StartIndex>\r\n";

static const char zb_zcl_readattrHelp[] = "Usage:\r\n"
        "zb-zcl-readattr <Address> <SrcEp> <DstEp> <ClusterID> [ToServer|ToClient] <AttrCount> <Attrib> [Standard|Custom] <ManuID>\r\n";

static const char zb_zcl_writeattrHelp[] = "Usage:\r\n"
        "zb-zcl-writeattr <Address> <SrcEp> <DstEp> <ClusterID> [ToServer|ToClient] <Attrib> <Type> <Data> [Standard|Custom] <ManuID>\r\n";

static const char zb_zdo_bindHelp[] = "Usage:\r\n"
        "zb-zdo-bind   <TargetAddress> <TargetEp> <Addrtype> <Address> <ClusterId> <DstEp>\r\n";

static const char zb_zdo_unbindHelp[] = "Usage:\r\n"
        "zb-zdo-unbind <TargetAddress> <TargetEp> <Addrtype> <Address> <ClusterId> <DstEp>\r\n";

static const char zb_coord_flashHelp[] = "Usage:\r\n"
        "zb_coord_flash version\r\n"
        "zb_coord_flash upgrade <path>\r\n";

static const shell_command_context_t s_zigbee_cmd[]={
        {"zb-nwk-form",          "\"zb-nwk-form\":          Form the network\r\n",             zb_nwk_form,         SHELL_OPTIONAL_PARAMS},
        {"zb-nwk-pjoin",         "\"zb-nwk-pjoin\":         Permit to join\r\n",               zb_nwk_pjoin,        1},
        {"zb-zcl-onoff",         "\"zb-zcl-onoff\":         Turn on/off the light\r\n",        zb_zcl_onoff,        SHELL_OPTIONAL_PARAMS},
        {"zb-zcl-level",         "\"zb-zcl-level\":         Control level of the light\r\n",   zb_zcl_level,        SHELL_OPTIONAL_PARAMS},
        {"zb-zcl-color",         "\"zb-zcl-color\":         Control the color\r\n",            zb_zcl_color,        SHELL_OPTIONAL_PARAMS},
     //   {"zb-zcl-ota",           "\"zb-zcl-ota\":           OTA Upgrade zigbee device\r\n",    zb_zcl_ota,          SHELL_OPTIONAL_PARAMS},
        {"zb-zdo-bind",          "\"zb-zdo-bind\":          Bind the device\r\n",              zb_zdo_bind,         SHELL_OPTIONAL_PARAMS},
        {"zb-zdo-unbind",        "\"zb-zdo-unbind\":        Unbind the device\r\n",            zb_zdo_unbind,       SHELL_OPTIONAL_PARAMS},
        {"zb-zdo-leave",         "\"zb-zdo-leave\":         Manage leave\r\n",                 zb_zdo_leave,        SHELL_OPTIONAL_PARAMS},
        {"zb-gen-reset",         "\"zb-gen-reset\":         Reset the ZigBee device\r\n",      zb_gen_reset,        0},
        {"zb-gen-erasepdm",      "\"zb-gen-erasepdm\":      Erase PDM\r\n",                    zb_gen_erasepdm,     0},
        {"zb-zdo-ieereq",        "\"zb-zdo-ieereq\":        Request IEEAddr\r\n",              zb_zdo_ieereq,       SHELL_OPTIONAL_PARAMS},
        {"zb-zdo-nwkreq",        "\"zb-zdo-nwkreq\":        Request NWKAddr\r\n",              zb_zdo_nwkreq,       SHELL_OPTIONAL_PARAMS},
        {"zb-zdo-activereq",     "\"zb-zdo-activereq\":     Request active endpoints\r\n",     zb_zdo_activereq,    1},
        {"zb-zdo-simplereq",     "\"zb-zdo-simplereq\":     Request simple descriptor\r\n",    zb_zdo_simplereq,    SHELL_OPTIONAL_PARAMS},
        {"zb-zdo-nodereq",       "\"zb-zdo-nodereq\":       Request node descriptor\r\n",      zb_zdo_nodereq,      1},
   //     {"zb-coord-flash",       "\"zb-coord-flash\":       Coordinator flash upgrade\r\n",    zb_coord_flash,      SHELL_OPTIONAL_PARAMS}, 
};

/*******************************************************************************
 * Code
 ******************************************************************************/
static int32_t zb_nwk_form(p_shell_context_t context, int32_t argc, char **argv)
{
    teZcbStatus rt = E_ZCB_ERROR;
    uint32_t ChannelMask;

    switch (argc)
    {
    case 2:
        if (strcmp(argv[1], HELP_STRING) == 0) {
           ;// context->printf_data_func("%s",zb_nwk_formHelp);
        } else {
            goto err;
        }
        break;
    case 3:
        ChannelMask = atoi(argv[1]);
        if (ChannelMask <= 26) {
            if(addr_valid(argv[2], 16)) {
                rt = eSetChannelMask(ChannelMask);
                assert(rt == E_ZCB_OK);
                rt = eSetExPANID(simple_strtoul(argv[2], NULL, 16));
                assert(rt == E_ZCB_OK);
                rt = eStartNetwork();
                assert(rt == E_ZCB_OK);
            } else {
                goto err;
            }
        } else {
            goto err;
        }
        break;
    default :
        goto err;
        break;
    }

    return 0;
err:
  //  context->printf_data_func("Error: Incorrect command or parameters\r\n");
    return -1;
}

static int32_t zb_nwk_pjoin(p_shell_context_t context, int32_t argc, char **argv)
{
    teZcbStatus rt = E_ZCB_ERROR;
    uint32_t Interval = 0;

    if (strcmp(argv[1], HELP_STRING) == 0) {
        context->printf_data_func("%s",zb_nwk_pjoinHelp);
    } else {
        Interval = atoi(argv[1]);
        if (Interval <= 255) {
            rt = eSetPermitJoining(Interval);
            assert(rt == E_ZCB_OK);
        } else {
            goto err;
        }
    }

    return 0;
err:
    context->printf_data_func("Error: Incorrect command or parameters\r\n");
    return -1;
}

static int32_t zb_zcl_onoff(p_shell_context_t context, int32_t argc, char **argv)
{
    teZcbStatus rt = E_ZCB_ERROR;
    uint8_t AddrMode = E_AM_SHORT;
    uint8_t CommandId = 2;//default : toggle

    switch (argc)
        {
            case 2:
                if (strcmp(argv[1], HELP_STRING) == 0) {
                    context->printf_data_func("%s", zb_zcl_onoffHelp);
                } else {
                    goto err;
                }
                break;

            case 6:
                if (strcmp(argv[1], "bound") == 0) {
                    AddrMode = E_AM_BOUND;
                } else if (strcmp(argv[1], "group") == 0) {
                    AddrMode = E_AM_GROUP;
                } else if (strcmp(argv[1], "short") == 0) {
                    AddrMode = E_AM_SHORT;
                } else {
                    goto err;
                }
                if(strcmp(argv[5], "off") == 0) {
                    CommandId = 0;
                } else if(strcmp(argv[5], "on") == 0) {
                    CommandId = 1;
                } else if (strcmp(argv[5], "toggle") == 0) {
                    CommandId = 2;
                } else {
                    goto err;
                }
                if (addr_valid(argv[2], 4) && is_a_endpoint(argv[3]) && is_a_endpoint(argv[4])) {
                  //  rt = eOnOff(AddrMode, strtol(argv[2], NULL, 16), atoi(argv[3]), atoi(argv[4]), CommandId);
                  //  assert(rt == E_ZCB_OK);
                } else {
                    goto err;
                }
                break;

            default:
                goto err;
                break;
        }

    return 0;
err:
    context->printf_data_func("Error: Incorrect command or parameters\r\n");
    return -1;
}

static int32_t zb_zcl_level(p_shell_context_t context, int32_t argc, char **argv)
{
    teZcbStatus rt = E_ZCB_ERROR;
    uint8_t AddrMode = 0;

    switch (argc)
    {
        case 2:
            if (strcmp(argv[1], HELP_STRING) == 0) {
                context->printf_data_func("%s", zb_zcl_levelHelp);
            } else {
                goto err;
            }
            break;
        case 9:
            if (strcmp(argv[2], "bound") == 0) {
                AddrMode = E_AM_BOUND;
            } else if (strcmp(argv[2], "group") == 0) {
                AddrMode = E_AM_GROUP;
            } else if (strcmp(argv[2], "short") == 0) {
                AddrMode = E_AM_SHORT;
            } else {
                goto err;
            }
            if (strcmp(argv[1], "move") == 0) {
                if (addr_valid(argv[3], 4) && is_a_endpoint(argv[4]) && is_a_endpoint(argv[5])
                    && (atoi(argv[6]) == 0 || atoi(argv[6]) == 1)
                    && (atoi(argv[7]) == 0 || atoi(argv[7]) == 1)
                    && (atoi(argv[8]) >= 0 || atoi(argv[8]) <= 0xFF)) {
                    rt = eLevelControlMove(AddrMode, strtol(argv[3], NULL, 16), atoi(argv[4]), atoi(argv[5]), atoi(argv[6]), atoi(argv[7]), atoi(argv[8]));
                    assert(rt == E_ZCB_OK);
                } else {
                    goto err;
                }
            } else if (strcmp(argv[1], "movetolevel") == 0) {
                if (addr_valid(argv[3], 4) && is_a_endpoint(argv[4]) && is_a_endpoint(argv[5])
                    && (atoi(argv[6]) == 0 || atoi(argv[6]) == 1)
                    && (atoi(argv[7]) >= 0 && atoi(argv[7]) <= 0xFF)
                    && (atoi(argv[8]) >= 0 && atoi(argv[8]) <= 0xFFFF)) {
//                    rt = eLevelControlMoveToLevel(AddrMode, strtol(argv[3], NULL, 16), atoi(argv[4]), atoi(argv[5]), atoi(argv[6]), atoi(argv[7]), atoi(argv[8]));
  //                  assert(rt == E_ZCB_OK);
                    } else {
                        goto err;
                    }
            } else {
                goto err;
            }
            break;
        case 10:
            if (strcmp(argv[2], "bound") == 0) {
                AddrMode = E_AM_BOUND;
            } else if (strcmp(argv[2], "group") == 0) {
                AddrMode = E_AM_GROUP;
            } else if (strcmp(argv[2], "short") == 0) {
                AddrMode = E_AM_SHORT;
            } else {
                goto err;
            }
            if (strcmp(argv[1], "step") == 0) {
                if (addr_valid(argv[3], 4) && is_a_endpoint(argv[4]) && is_a_endpoint(argv[5])
                    && (atoi(argv[6]) == 0 || atoi(argv[6]) == 1)
                    && (atoi(argv[7]) == 0 || atoi(argv[7]) == 1)
                    && (atoi(argv[8]) >= 0 && atoi(argv[8]) <= 0xFF)
                    && (atoi(argv[9]) >= 0 && atoi(argv[9]) <= 0xFFFF)) {
                    rt = eLevelControlMoveStep(AddrMode, strtol(argv[3], NULL, 16), atoi(argv[4]), atoi(argv[5]), atoi(argv[6]), atoi(argv[7]), atoi(argv[8]), atoi(argv[9]));
                    assert(rt == E_ZCB_OK);
                } else {
                    goto err;
                }
            } else {
                goto err;
            }
            break;
        default:
            goto err;
            break;
    }

    return 0;
err:
    context->printf_data_func("Error: Incorrect command or parameters\r\n");
    return -1;
}

static int32_t zb_zcl_color(p_shell_context_t context, int32_t argc, char **argv)
{
    teZcbStatus rt = E_ZCB_ERROR;
    uint8_t AddrMode = E_AM_SHORT;

    switch (argc)
    {
        case 2:
            if (strcmp(argv[1], HELP_STRING) == 0) {
                context->printf_data_func("%s", zb_zcl_colorHelp);
            } else {
                goto err;
            }
            break;
        case 8:
            if (strcmp(argv[2], "bound") == 0) {
                AddrMode = E_AM_BOUND;
            } else if (strcmp(argv[2], "group") == 0) {
                AddrMode = E_AM_GROUP;
            } else if (strcmp(argv[2], "short") == 0) {
                AddrMode = E_AM_SHORT;
            } else {
                goto err;
            }
            if (strcmp(argv[1], "movetotemp") == 0) {
                if (addr_valid(argv[3], 4) && is_a_endpoint(argv[4]) && is_a_endpoint(argv[5])
                                    && (atoi(argv[6]) >= 0 && atoi(argv[6]) <= 0xFF)
                                    && (atoi(argv[7]) >= 0 && atoi(argv[7]) <= 0xFFFF)) {
//                    rt = eColorControlMoveToTemp(AddrMode, strtol(argv[3], NULL, 16), atoi(argv[4]), atoi(argv[5]), atoi(argv[6]), atoi(argv[7]));
 //                   assert(rt == E_ZCB_OK);
                } else {
                    goto err;
                }
            } else {
                goto err;
            }
            break;
        case 9:
            if (strcmp(argv[2], "bound") == 0) {
                AddrMode = E_AM_BOUND;
            } else if (strcmp(argv[2], "group") == 0) {
                AddrMode = E_AM_GROUP;
            } else if (strcmp(argv[2], "short") == 0) {
                AddrMode = E_AM_SHORT;
            } else {
                goto err;
            }
            if (strcmp(argv[1], "movetocolor") == 0) {
                if (addr_valid(argv[3], 4) && is_a_endpoint(argv[4]) && is_a_endpoint(argv[5])
                    && (atoi(argv[6]) >=0 && atoi(argv[6]) <= 0xFF)
                    && (atoi(argv[7]) >=0 && atoi(argv[7]) <= 0xFF)
                    && (atoi(argv[8]) >=0 && atoi(argv[8]) <= 0xFFFF)) {
//                    rt = eColorControlMoveToColor(AddrMode, strtol(argv[3], NULL, 16), atoi(argv[4]), atoi(argv[5]), atoi(argv[6]), atoi(argv[7]), atoi(argv[8]));
  //                  assert(rt == E_ZCB_OK);
                } else {
                    goto err;
                }
            } else if (strcmp(argv[1], "movetohue") == 0) {
                if (addr_valid(argv[3], 4) && is_a_endpoint(argv[4]) && is_a_endpoint(argv[5])
                    && (atoi(argv[6]) >= 0 && atoi(argv[6]) <= 0xFF)
                    && (atoi(argv[7]) >= 0 && atoi(argv[7]) <= 3)
                    && (atoi(argv[8]) >= 0 && atoi(argv[8]) <= 0xFFFF)) {
      //              rt = eColorControlMoveToHue(AddrMode, strtol(argv[3], NULL, 16), atoi(argv[4]), atoi(argv[5]), atoi(argv[6]), atoi(argv[7]), atoi(argv[8]));
       //             assert(rt == E_ZCB_OK);
                } else {
                    goto err;
                }
            } else {
                goto err;
            }
            break;
        default:
            goto err;
            break;
    }

    return 0;
err:
    context->printf_data_func("Error: Incorrect command or parameters\r\n");
    return -1;
}

#if 0 
static int32_t zb_zcl_ota(p_shell_context_t context, int32_t argc, char **argv)
{
    int32_t rt = E_ZCB_ERROR;
    uint8_t AddrMode = 0;
    static uint8_t ota_notify_header[8] = {0};

    switch (argc)
    {
        case 2:
            if (strcmp(argv[1], HELP_STRING) == 0) {
                context->printf_data_func("%s", zb_zcl_otaHelp);
            } else {
                goto err;
            }
            break;
        case 3:
        {
            if (strcmp(argv[1], "load") == 0) {
                char *p_full_path = (char *)__alloc_fullpath(argv[2]);                               
                assert(p_full_path != NULL);

                if (strlen(p_full_path) >= ZB_DEVICE_OTA_IMAGE_PATH_MAX_LENGTH) {
                    context->printf_data_func("Error: The path is too long\r\n");
                    return -1;
                }
                    
                memset(g_OtaImagePath, 0, ZB_DEVICE_OTA_IMAGE_PATH_MAX_LENGTH);
                strcpy(g_OtaImagePath, p_full_path);
                
                uint8_t tmp_buf[ZB_DEVICE_MAX_OTA_HEADER_SIZE];
                lfs_file_t file;

                rt = lfs_file_open(&g_lfs, &file, p_full_path, LFS_O_RDONLY);
                if (rt != 0) {
                    context->printf_data_func("Error: Cannot open the file\r\n");
                    vPortFree(p_full_path);
                    return -1;
                } 
                vPortFree(p_full_path);

                memset(tmp_buf, 0, sizeof(tmp_buf));
                rt = lfs_file_read(&g_lfs, &file, tmp_buf, ZB_DEVICE_MAX_OTA_HEADER_SIZE);                
                if (rt < 0) {
                    context->printf_data_func("Error: Cannot read the file\r\n");
                    rt = lfs_file_close(&g_lfs, &file);
                    return -1;
                } else if (rt < ZB_DEVICE_MAX_OTA_HEADER_SIZE){
                    context->printf_data_func("Error: The OTA file is invalid\r\n");
                    rt = lfs_file_close(&g_lfs, &file);
                    return -1;
                } else {
                    memset(ota_notify_header, 0, sizeof(ota_notify_header));
                    memcpy(ota_notify_header, &tmp_buf[ZB_DEVICE_OTA_HEADER_MANU_CODE_OFFSET], sizeof(ota_notify_header));
                    rt = eOtaLoadNewImage(tmp_buf, 0);
                    assert(rt == E_ZCB_OK);
                }
                    
                rt = lfs_file_close(&g_lfs, &file);
                assert(rt == 0);
            } else {
                goto err;
            }
        }
            break;
        case 6:            
            if (strcmp(argv[2], "bound") == 0) {
                AddrMode = E_AM_BOUND;
            } else if (strcmp(argv[2], "group") == 0) {
                AddrMode = E_AM_GROUP;
            } else if (strcmp(argv[2], "short") == 0) {
                AddrMode = E_AM_SHORT;
            } else {
                goto err;
            }      
            if (strcmp(argv[1], "notify") == 0) {
                if ((ota_notify_header[0] == 0) && (ota_notify_header[1] == 0)) {
                    context->printf_data_func("Error: Invalid manu code, need reload image\r\n");
                    return -1;
                } else {
                    rt = ZCB_OtaImageNotify(AddrMode, strtol(argv[3], NULL, 16), atoi(argv[4]), atoi(argv[5]), ota_notify_header);
                    assert(rt == E_ZCB_OK); 
                }
            } else {
                goto err;
            }
            break;
        default:
            goto err;
            break;        
    }
    
    return 0;
err:
    context->printf_data_func("Error: Incorrect command or parameters\r\n");
    return -1;
}
#endif

static int32_t zb_zdo_bind(p_shell_context_t context, int32_t argc, char **argv)
{
    context->printf_data_func("Bind the device...\r\n");
    // TODO: add the function

    return 0;
}

static int32_t zb_zdo_unbind(p_shell_context_t context, int32_t argc, char **argv)
{
    context->printf_data_func("Unbind the device...\r\n");
    // TODO: add the function

    return 0;
}

static int32_t zb_zdo_leave(p_shell_context_t context, int32_t argc, char **argv)
{
    teZcbStatus rt = E_ZCB_ERROR;

    switch (argc)
    {
        case 2:
            if (strcmp(argv[1], HELP_STRING) == 0) {
                context->printf_data_func("%s", zb_zdo_leaveHelp);
            } else {
                goto err;
            }
            break;
        case 5:
            if (addr_valid(argv[1], 4) && addr_valid(argv[2], 16)
                && (atoi(argv[3]) == 0 || atoi(argv[3]) == 1)
                && (atoi(argv[4]) == 0 || atoi(argv[4]) == 1)) {
                rt = eMgmtLeaveRequst(strtol(argv[1], NULL, 16),simple_strtoul(argv[2], NULL, 16), atoi(argv[3]), atoi(argv[4]));
                assert(rt == E_ZCB_OK);
            } else {
                goto err;
            }
            break;
        default:
            goto err;
            break;
    }

    return 0;
err:
    context->printf_data_func("Error: Incorrect command or parameters\r\n");
    return -1;
}

static int32_t zb_zdo_activereq(p_shell_context_t context, int32_t argc, char **argv)
{
    teZcbStatus rt = E_ZCB_ERROR;

    switch (argc)
    {
        case 2:
            if (strcmp(argv[1], HELP_STRING) == 0) {
                context->printf_data_func("%s", zb_zdo_activereqHelp);
            } else if (addr_valid(argv[1], 4)) {
                rt = eActiveEndpointRequest(strtol(argv[1], NULL, 16));
                assert(rt == E_ZCB_OK);
            } else {
                goto err;
            }
            break;
    }
    return 0;

err:
    context->printf_data_func("Error: Incorrect command or parameters\r\n");
    return -1;
}

static int32_t zb_zdo_nodereq(p_shell_context_t context, int32_t argc, char **argv)
{
    teZcbStatus rt = E_ZCB_ERROR;

    switch (argc)
    {
        case 2:
            if (strcmp(argv[1], HELP_STRING) == 0) {
                context->printf_data_func("%s", zb_zdo_nodereqHelp);
            } else if (addr_valid(argv[1], 4)) {
                rt = eNodeDescriptorRequest(strtol(argv[1], NULL, 16));
                assert(rt == E_ZCB_OK);
            } else {
                goto err;
            }
            break;
    }
    return 0;

err:
    context->printf_data_func("Error: Incorrect command or parameters\r\n");
    return -1;
}

static int32_t zb_zdo_simplereq(p_shell_context_t context, int32_t argc, char **argv)
{
    teZcbStatus rt = E_ZCB_ERROR;

    switch (argc)
    {
        case 2:
            if (strcmp(argv[1], HELP_STRING) == 0) {
                context->printf_data_func("%s", zb_zdo_simplereqHelp);
            } else {
                goto err;
            }
            break;
        case 3:
            if (addr_valid(argv[1], 4) && is_a_endpoint(argv[2])) {
                rt = eSimpleDescriptorRequest(strtol(argv[1], NULL, 16), atoi(argv[2]));
                assert(rt == E_ZCB_OK);
            } else {
                goto err;
            }
    }
    return 0;

err:
    context->printf_data_func("Error: Incorrect command or parameters\r\n");
    return -1;
}

static int32_t zb_zdo_ieereq(p_shell_context_t context, int32_t argc, char **argv)
{
    teZcbStatus rt = E_ZCB_ERROR;
    uint8_t RequestType = 0;

    switch (argc)
    {
        case 2:
            if (strcmp(argv[1], HELP_STRING) == 0) {
                context->printf_data_func("%s", zb_zdo_ieereqHelp);
            } else {
                goto err;
            }
            break;
        case 5:
            if (strcmp(argv[3], "single") == 0) {
                RequestType = E_RT_SINGLE;
            } else if (strcmp(argv[3], "extended") == 0) {
                RequestType = E_RT_EXTENDED;
            } else {
                goto err;
            }
            if (addr_valid(argv[1], 4) && addr_valid(argv[2], 4)
                    && (atoi(argv[4]) >= 0 && atoi(argv[4]) <= 255)) {
                rt = eIeeeAddressRequest(strtol(argv[1], NULL, 16), strtol(argv[2], NULL, 16), RequestType, atoi(argv[4]));
                assert(rt == E_ZCB_OK);
            } else {
                goto err;
            }
            break;
        default:
            goto err;
            break;
    }
    return 0;
err:
    context->printf_data_func("Error: Incorrect command or parameters\r\n");
    return -1;
}

static int32_t zb_zdo_nwkreq(p_shell_context_t context, int32_t argc, char **argv)
{
    teZcbStatus rt = E_ZCB_ERROR;
    uint8_t RequestType = 0;

    switch (argc)
    {
        case 2:
            if (strcmp(argv[1], HELP_STRING) == 0) {
                context->printf_data_func("%s", zb_zdo_nwkreqHelp);
            }
            else {
                goto err;
            }
            break;
        case 5:
            if (strcmp(argv[3], "single") == 0) {
                RequestType = E_RT_SINGLE;
            } else if (strcmp(argv[3], "extended") == 0) {
                RequestType = E_RT_EXTENDED;
            } else {
                goto err;
            }
            if (addr_valid(argv[1], 4) && addr_valid(argv[2], 16)
                    && (atoi(argv[4]) >= 0 && atoi(argv[4]) <= 255)) {
                rt = eNwkAddressRequest(strtol(argv[1], NULL, 16), simple_strtoul(argv[2], NULL, 16), RequestType, atoi(argv[4]));
                assert(rt == E_ZCB_OK);
            } else {
                goto err;
            }
            break;
        default:
            goto err;
            break;
    }
    return 0;

err:
    context->printf_data_func("Error: Incorrect command or parameters\r\n");
    return -1;
}

static int32_t zb_gen_reset(p_shell_context_t context, int32_t argc, char **argv)
{
    teZcbStatus rt = E_ZCB_ERROR;
    rt = eResetDevice();
    assert(rt == E_ZCB_OK);

    return 0;
}

static int32_t zb_gen_erasepdm(p_shell_context_t context, int32_t argc, char **argv)
{
    teZcbStatus rt = E_ZCB_ERROR;
    rt = eErasePersistentData();
    assert(rt == E_ZCB_OK);

    /*Also clear device table reserved in host.*/
    vZDM_ClearAllDeviceTables(); 

    return 0;
}

#if 0
static int32_t zb_coord_flash(p_shell_context_t context, int32_t argc, char **argv)
{
    int32_t rt;

    switch (argc)
    {
        case 2:
            if (strcmp(argv[1], HELP_STRING) == 0) {
                context->printf_data_func("%s", zb_coord_flashHelp);
            } else if (strcmp(argv[1], "version") == 0){
                rt = eZCB_GetCoordinatorVersion();
                return rt;
            } else {
                goto err;
            }
            break;
        case 3:
        {
            if (strcmp(argv[1], "upgrade") == 0) {
                char *p_full_path = (char *)__alloc_fullpath(argv[2]);                               
                assert(p_full_path != NULL);

                if (strlen(p_full_path) >= ZB_DEVICE_OTA_IMAGE_PATH_MAX_LENGTH) {
                    context->printf_data_func("Error: The path is too long\r\n");
                    return -1;
                }
                    
                memset(g_OtaImagePath, 0, ZB_DEVICE_OTA_IMAGE_PATH_MAX_LENGTH);
                strcpy(g_OtaImagePath, p_full_path);
                
                uint8_t tmp_buf[ZB_DEVICE_MAX_OTA_HEADER_SIZE];
                lfs_file_t file;

                rt = lfs_file_open(&g_lfs, &file, p_full_path, LFS_O_RDONLY);               
                if (rt != 0) {
                    context->printf_data_func("Error: Cannot open the file\r\n");
                    vPortFree(p_full_path);
                    return -1;
                } 
                vPortFree(p_full_path);
                
                rt = lfs_file_size(&g_lfs, &file);              
                if (rt < 0) {
                    context->printf_data_func("Error: Cannot get the size of file\r\n");
                    return -1;
                }
                context->printf_data_func("the size of file = %d\r\n", rt);
                
                memset(tmp_buf, 0, sizeof(tmp_buf));               
                rt = lfs_file_read(&g_lfs, &file, tmp_buf, ZB_DEVICE_MAX_OTA_HEADER_SIZE);                
                if (rt < 0) {
                    context->printf_data_func("Error: Cannot read the file\r\n");
                    rt = lfs_file_close(&g_lfs, &file);
                    return -1;
                } else if (rt < ZB_DEVICE_MAX_OTA_HEADER_SIZE){
                    context->printf_data_func("Error: The OTA file is invalid\r\n");
                    rt = lfs_file_close(&g_lfs, &file);
                    return -1;
                } else {
                    rt = eOtaLoadNewImage(tmp_buf, 1);
                    //assert(rt == E_ZCB_OK);
                }

                rt = lfs_file_close(&g_lfs, &file);
                assert(rt == 0);
                return rt;
            } else {
                goto err;
            }
        }
            break;
        default:
            goto err;
            break;
    }
    return 0;
    
err:
    context->printf_data_func("Error: Incorrect command or parameters\r\n");
    return -1;
}
#endif

bool addr_valid(const char *cp, uint8_t base)
{
    uint32_t length = 0;
    if (cp != NULL) {
        if (cp[0] == '0' && TOLOWER(cp[1]) == 'x')
            cp += 2;
        while (*cp != '\0') {
            length++;
            if (!isxdigit(*cp)){
                return 0;
            }
            cp ++;
        }
    }
    return (length <= base);
}

static bool is_a_endpoint(const char *cp)
{
    uint64_t result = 0, value;

    if (cp != NULL) {
        while (*cp != '\0') {
            if (!isdigit(*cp)) {
                return 0;
            }
            value = *cp - '0';
            result = result*10 + value;
            cp ++;
        }
    }
    return (result > 0 && result <=240);
}

uint64_t simple_strtoul(const char *cp, char **endp, uint8_t base)
{
    uint64_t result = 0,value;

    if (!base) {
        base = 10;
        if (*cp == '0') {
            base = 8;
            cp++;
            if ((TOLOWER(*cp) == 'x') && isxdigit(cp[1])) {
                cp++;
                base = 16;
            }
        }
    } else if (base == 16) {
        if (cp[0] == '0' && TOLOWER(cp[1]) == 'x')
            cp += 2;
    }
    while (isxdigit(*cp) &&
           (value = isdigit(*cp) ? *cp-'0' : TOLOWER(*cp)-'a'+ 10) < base) {
        result = result*base + value;
        cp++;
    }
    if (endp)
        *endp = (char *)cp;
    return result;
}

#if 0
void ZigBeeCmdRegister(void)
{
    for (uint32_t i = 0; i < (sizeof(s_zigbee_cmd) / sizeof(s_zigbee_cmd[0])); i++) {
            SHELL_RegisterCommand(&s_zigbee_cmd[i]);
        }
}
#endif
