/*
 *
 *    Copyright (c) 2022-2024 Project CHIP Authors
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
#include <lib/shell/SubShellCommand.h>
#include <cstring>
#include <lib/shell/Engine.h>
#include <platform/CHIPDeviceLayer.h>

#include "cmd.h"
#include "zcb.h"
#include "shell.h"
#include "zigbee_cmd.h"
#include "ZigbeeDevices.h"

#define MATTER_CLI_LOG(message) (chip::Shell::streamer_printf(chip::Shell::streamer_get(), message))

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
        MATTER_CLI_LOG("\n ### No Args: Must Has Channel\n ");
		goto err;
		break;
    case 1:
			ChannelMask = atoi(argv[0]);
			if ((ChannelMask <= 26) && (ChannelMask >= 11))
			{
                rt = eSetChannelMask(ChannelMask);
                assert(rt == E_ZCB_OK);
                MATTER_CLI_LOG("\n Channel mask set\n ");
                rt = eStartNetwork();
                assert(rt == E_ZCB_OK);
                MATTER_CLI_LOG("\n Network started\n ");
			}
		    else {
                MATTER_CLI_LOG("\n ### Channel must between 11 ~ 26\n ");
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

CHIP_ERROR zb_nwk_steer(int argc, char *argv[])
{
    teZcbStatus rt = E_ZCB_ERROR;
    rt = eStartSteer();
    assert(rt == E_ZCB_OK);

    return CHIP_NO_ERROR;
}

CHIP_ERROR zb_nwk_find(int argc, char *argv[])
{
    teZcbStatus rt = E_ZCB_ERROR;
    rt = eStartFindAndBind();
    assert(rt == E_ZCB_OK);
    return CHIP_NO_ERROR;
}

CHIP_ERROR zb_zdo_leave(int argc, char *argv[])
{
    teZcbStatus rt = E_ZCB_ERROR;

    switch (argc)
    {
	    case 0:
            MATTER_CLI_LOG("\n ### No Args , Must be : ShortAddr MACAddr\n ");
			goto err;
			break;
        case 1:
            if (strcmp(argv[0], HELP_STRING) == 0) {
              ;
            } else {
				MATTER_CLI_LOG("\n ### Incorrect Args , Must be : ShortAddr MACAddr\n ");
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
				MATTER_CLI_LOG("\n ### Incorrect Args , Must be ShortAddr MACAddr\n ");
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
    rt = eOnOff();
    assert(rt == E_ZCB_OK);
    return CHIP_NO_ERROR;
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

void RegisterZigbeeCommands()
{
    static constexpr chip::Shell::Command subCommands[] = {
        { &zb_erasepdm, "zb-erasepdm", "Erase PDM : No Argument" },
        { &zb_nwk_form, "zb-nwk-form", "Form the network:args with Channel" },
        { &zb_nwk_steer, "zb-nwk-steer", "Steer the network: No Argument" },
        { &zb_nwk_find, "zb-nwk-find", "Find and Bind: No Argument" },
        { &zb_zdo_leave, "zb-zdo-leave", "Leave node from network" },
        { &zb_zcl_onoff, "zb-zcl-onoff", "Turn on/off the light: On/Off/Toggle" },
        { &enum_nodes, "EnumNodes", "List Joined Nodes" },
        { &save_nodes, "SaveNodes", "Save Joined Nodes" },
        { &retrieve_nodes, "RetrieveNodes", "Retrieve Joined Nodes" },
    };

    static constexpr chip::Shell::Command zigbeeCommands = { &chip::Shell::SubShellCommand<MATTER_ARRAY_SIZE(subCommands), subCommands>, "zb",
                                             "Zigbee commands" };

    chip::Shell::Engine::Root().RegisterCommands(&zigbeeCommands, 1);
}

