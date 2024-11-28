/*
 *
 *    Copyright (c) 2022 Project CHIP Authors
 *    Copyright 2023-2024 NXP
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

#include "AppCLIBase.h"
#include "AppTaskBase.h"
#include <ChipShellCollection.h>
#include <app/server/Server.h>
#include <cstring>
#include <lib/shell/Engine.h>
#include <platform/CHIPDeviceLayer.h>

#include "cmd.h"
#include "zcb.h"
#include "shell.h"
#include "zigbee_cmd.h"
#include "ZigbeeDevices.h"

#define MATTER_CLI_LOG(message) (chip::Shell::streamer_printf(chip::Shell::streamer_get(), message))

static CHIP_ERROR commissioningManager(int argc, char * argv[])
{
    CHIP_ERROR error = CHIP_NO_ERROR;
    if (strncmp(argv[0], "on", 2) == 0)
    {
        chip::NXP::App::GetAppTask().StartCommissioningHandler();
    }
    else if (strncmp(argv[0], "off", 3) == 0)
    {
        chip::NXP::App::GetAppTask().StopCommissioningHandler();
    }
    else
    {
        MATTER_CLI_LOG("wrong args should be either \"mattercommissioning on\" or \"mattercommissioning off\"");
        error = CHIP_ERROR_INVALID_ARGUMENT;
    }
    return error;
}

static CHIP_ERROR cliFactoryReset(int argc, char * argv[])
{
    chip::NXP::App::GetAppTask().FactoryResetHandler();
    return CHIP_NO_ERROR;
}

static CHIP_ERROR cliReset(int argc, char * argv[])
{
    chip::NXP::App::GetAppCLI().ResetCmdHandle();
    return CHIP_NO_ERROR;
}

#define TOLOWER(x) ((x) | 0x20)
#define isxdigit(c)    (('0' <= (c) && (c) <= '9') || ('a' <= (c) && (c) <= 'f') || ('A' <= (c) && (c) <= 'F'))
#define isdigit(c)    ('0' <= (c) && (c) <= '9')

bool is_a_endpoint(const char *cp)
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

CHIP_ERROR zb_erasepdm(int argc, char *argv[])
{
    teZcbStatus rt = E_ZCB_ERROR;
    rt = eErasePersistentData();
    assert(rt == E_ZCB_OK);

    /*Also clear device table reserved in host.*/
    vZDM_ClearAllDeviceTables(); 

    return CHIP_NO_ERROR;
}

CHIP_ERROR zb_nwk_form(int argc, char *argv[])
{
    teZcbStatus rt = E_ZCB_ERROR;
    uint32_t ChannelMask;

    switch (argc)
    {
    case 0:
    	PRINTF("\n ### No Args: Must Has Channel\n ");
		goto err;
		break;
    case 1:
			ChannelMask = atoi(argv[0]);
			if ((ChannelMask <= 26) && (ChannelMask >= 11))
			{
                rt = eSetChannelMask(ChannelMask);
                assert(rt == E_ZCB_OK);
                rt = eStartNetwork();
                assert(rt == E_ZCB_OK);
			}
		else {
			PRINTF("\n ### Channel must between 11 ~ 26\n ");
            goto err;
        }
        break;
	
    default :
        goto err;
        break;
    }

    return CHIP_NO_ERROR;
err:
    return CHIP_ERROR_INVALID_ARGUMENT;
}

CHIP_ERROR zb_nwk_pjoin(int argc, char *argv[])
{
    teZcbStatus rt = E_ZCB_ERROR;
    uint32_t Interval = 0;

    switch (argc)
    {
      case 0:
		PRINTF("\n ### No Args , Must has value between 0 ~ 255\n "); 
        goto err;		
	 	break;

	  case 1:
		  if (strcmp(argv[0], HELP_STRING) == 0) {
			 ;
		  } else {
			  Interval = atoi(argv[0]);
			  if (Interval <= 255) 
			  {
				  rt = eSetPermitJoining(Interval);
				  assert(rt == E_ZCB_OK);
			  } else {
				  PRINTF("\n ### Invalid Args , Must between 0 ~ 255\n "); 
				  goto err;
			  }
		  }
	 	break;

        default:
			PRINTF("\n ### Invalid Args , Must between 0 ~ 255\n "); 
            goto err;
            break;		
    }

    return CHIP_NO_ERROR;
err:
    return CHIP_ERROR_INVALID_ARGUMENT;
}

CHIP_ERROR zb_zdo_leave(int argc, char *argv[])
{
    teZcbStatus rt = E_ZCB_ERROR;

    switch (argc)
    {
	    case 0:
			PRINTF("\n ### No Args , Must be : ShortAddr MACAddr\n ");
			goto err;
			break;
        case 1:
            if (strcmp(argv[0], HELP_STRING) == 0) {
              ;
            } else {
				PRINTF("\n ### Incorrect Args , Must be : ShortAddr MACAddr\n ");
                goto err;
            }
            break;
		case 2:
			if (addr_valid(argv[0], 4) && addr_valid(argv[1], 16))
			{
				rt = eMgmtLeaveRequst(strtol(argv[0], NULL, 16),simple_strtoul(argv[1], NULL, 16),0,0);// Force Rejoin & Remove to 0
                assert(rt == E_ZCB_OK);			
			}
			else 
			{
				PRINTF("\n ### Incorrect Args , Must be ShortAddr MACAddr\n ");
				goto err;
			}	
			break;
        case 4:
            if (addr_valid(argv[0], 4) && addr_valid(argv[1], 16)
                && (atoi(argv[2]) == 0 || atoi(argv[2]) == 1)
                && (atoi(argv[3]) == 0 || atoi(argv[3]) == 1)) {
                rt = eMgmtLeaveRequst(strtol(argv[0], NULL, 16),simple_strtoul(argv[1], NULL, 16), atoi(argv[2]), atoi(argv[3]));
                assert(rt == E_ZCB_OK);
            } else {
                goto err;
            }
            break;
        default:
            goto err;
            break;
    }

    return CHIP_NO_ERROR;
err:
    return CHIP_ERROR_INVALID_ARGUMENT;
}

CHIP_ERROR zb_zcl_onoff(int argc, char **argv)
{
    teZcbStatus rt = E_ZCB_ERROR;
    uint8_t AddrMode = 0;
    uint8_t CommandId = 0;

    switch (argc)
        {
        	case 0:
				PRINTF("\n *** Must has 2 Args : ShortAddr On/Off/Toggle \n");
                goto err;
				break;
            case 1: //changed from 2 to 1
                if (strcmp(argv[0], HELP_STRING) == 0) {
                  ;
                } else {
                  PRINTF("\n *** Incorect Args , Must be : ShortAddr On/Off/Toggle \n");
                    goto err;
                }
                break;

			case 2: //simplify args : ShortAddr & On/Off/Toggle
				AddrMode = E_AM_SHORT;
				if (addr_valid(argv[0], 4))
				{
				  if(strcmp(argv[1], "off") == 0) {
                    CommandId = 0;
                  } else if(strcmp(argv[1], "on") == 0) {
                    CommandId = 1;
                  } else if (strcmp(argv[1], "toggle") == 0) {
                    CommandId = 2;
                  } else {
                    PRINTF("\n *** Missing Second Args : On/Off/Toggle \n");
                  }
				  rt = eOnOff(AddrMode, strtol(argv[0], NULL, 16), 1, 1, CommandId); //default Src&Dst EP =1
				  assert(rt == E_ZCB_OK);
				}
				else
					 goto err;
				break;
				
            case 5: //changed from 6 args to 5 
                if (strcmp(argv[0], "bound") == 0) {
                    AddrMode = E_AM_BOUND;
                } else if (strcmp(argv[0], "group") == 0) {
                    AddrMode = E_AM_GROUP;
                } else if (strcmp(argv[0], "short") == 0) {
                    AddrMode = E_AM_SHORT;
                } else {
                    goto err;
                }
                if(strcmp(argv[4], "off") == 0) {
                    CommandId = 0;
                } else if(strcmp(argv[4], "on") == 0) {
                    CommandId = 1;
                } else if (strcmp(argv[4], "toggle") == 0) {
                    CommandId = 2;
                } else {
                    goto err;
                }
                if (addr_valid(argv[1], 4) && is_a_endpoint(argv[2]) && is_a_endpoint(argv[3])) {
                    rt = eOnOff(AddrMode, strtol(argv[1], NULL, 16), atoi(argv[2]), atoi(argv[3]), CommandId);
                    assert(rt == E_ZCB_OK);
                } else {
                    goto err;
                }
                break;

            default:
                goto err;
                break;
        }

    return CHIP_NO_ERROR;
err:
    return CHIP_ERROR_INVALID_ARGUMENT;
}

extern bool EnumJoinedNodes(void);
CHIP_ERROR enum_nodes(int argc, char **argv)
{
	EnumJoinedNodes();
	return CHIP_NO_ERROR;
}

extern void RestoreJoinedNodes(void);
CHIP_ERROR retrieve_nodes(int argc, char **argv)
{
	RestoreJoinedNodes();
	return CHIP_NO_ERROR;
}

extern void SaveJoinedNodes(void);
CHIP_ERROR save_nodes(int argc, char **argv)
{
	SaveJoinedNodes();
	return CHIP_NO_ERROR;
}

void chip::NXP::App::AppCLIBase::RegisterDefaultCommands(void)
{
    static const chip::Shell::shell_command_t kCommands[] = {
        {
            .cmd_func = commissioningManager,
            .cmd_name = "mattercommissioning",
            .cmd_help = "Open/close the commissioning window. Usage : mattercommissioning [on|off]",
        },
        {
            .cmd_func = cliFactoryReset,
            .cmd_name = "matterfactoryreset",
            .cmd_help = "Perform a factory reset on the device",
        },
        {
            .cmd_func = cliReset,
            .cmd_name = "matterreset",
            .cmd_help = "Reset the device",
        },
		{
			.cmd_func = zb_erasepdm,
			.cmd_name = "zb-erasepdm",
			.cmd_help = "Erase PDM : No Argument",
		},
		{
			.cmd_func = zb_nwk_form,
			.cmd_name = "zb-nwk-form",
			.cmd_help = "Form the network:args with Channel",
		},
		{
			.cmd_func = zb_nwk_pjoin,
			.cmd_name = "zb-nwk-pjoin",
			.cmd_help = "Permit to join:args as Timeout",
		},
		{
			.cmd_func = zb_zdo_leave,
			.cmd_name = "zb-zdo-leave",
			.cmd_help = "Leave node from network",
		},								
		{
			.cmd_func = zb_zcl_onoff,
			.cmd_name = "zb-zcl-onoff",
			.cmd_help = "Turn on/off the light:2 args as ShortAddr On/Off/Toggle",
		},
		{
			.cmd_func = enum_nodes,
			.cmd_name = "EnumNodes",
			.cmd_help = "List Joined Nodes",
		},
		{
			.cmd_func = save_nodes,
			.cmd_name = "SaveNodes",
			.cmd_help = "Save Joined Nodes",
		},			
		{
			.cmd_func = retrieve_nodes,
			.cmd_name = "RetrieveNodes",
			.cmd_help = "Retrieve Joined Nodes",
		},	
    };

    /* Register common shell commands */
    cmd_misc_init();
    cmd_otcli_init();
#if CHIP_SHELL_ENABLE_CMD_SERVER
    cmd_app_server_init();
#endif /* CHIP_SHELL_ENABLE_CMD_SERVER */
    chip::Shell::Engine::Root().RegisterCommands(kCommands, sizeof(kCommands) / sizeof(kCommands[0]));
}
