# CHIP RW61x Matter Zigbee Bridge Application

The bridge-app example implements a server which can be accessed by a CHIP
controller and can accept basic cluster commands.

This bridge-app support RW612 as Matter-Zigbee-Bridge functionality 
so that Non-Matter nodes like Zigbee light or sensor devices can be controlled by Matter.

There are  additonal commands to control the external Zigbee Coordinator:
   - zb-erasepdm   (factory reset Zigbee Coordinator by erase all network data)
   - zb-nwk-form   (form Zigbee network)
   - zb-nwk-pjoin  (permit join Zigbee node : 0 to disable, 255 to enable)
   - zb-zdo-leave  (management leave the joined Zigbee node)
   - EnumNodes     (list the joined Zigbee devices)


The example is based on
[Project CHIP](https://github.com/project-chip/connectedhomeip) and the NXP
RW612 SDK, and provides a prototype application that demonstrates device
commissioning and different cluster control.

<hr>

-   [Introduction](#introduction)
-   [Building](#building)
-   [Flashing and debugging](#flashing-and-debugging)
-   [Testing the example](#testing-the-example)
-   [Matter Shell](#testing-the-bridge-application-with-matter-cli-enabled)
-   [OTA Software Update](#ota-software-update)

<hr>

<a name="introduction"></a>

## Introduction

The RW61x bridge application provides a working demonstration of the
RW610/RW612 board integration, built using the Project CHIP codebase and the NXP
RW612 SDK.

The example supports:

-   Matter over Wi-Fi

### Hardware requirements

For Matter over WiFi configuration :

-   [`NXP RD-RW612-BGA`] or [`NXP RD-RW610-BGA`] board
-   BLE antenna (to plug in Ant1)
-   Wi-Fi antenna (to plug in Ant2)

<a name="building"></a>

## Building

In order to build the Project CHIP example, we recommend using a Linux
distribution (the demo-application was compiled on Ubuntu 20.04).

-   Follow instruction in [nxp_examples_freertos_platforms.md](third_party/matter/docs/platforms/nxp/nxp_examples_freertos_platforms.md)
    to setup the environment to be able to build Matter.

-   Download the NXP MCUXpresso git SDK and associated middleware using the west
    tool.

## Manufacturing data

See
[Guide for writing manufacturing data on NXP devices](../../../../../docs/guides/nxp_manufacturing_flow.md)

Other comments:

The bridge app demonstrates the usage of encrypted Matter manufacturing
data storage. Matter manufacturing data should be encrypted using an AES 128
software key before flashing them to the device flash.

Using DAC private key secure usage: Experimental feature, contain some
limitation: potential concurrent access issue during sign with dac key operation
due to the lack of protection between multiple access to `ELS` crypto module.
The argument `chip_enable_secure_dac_private_key_storage=1` must be added to the
_gn gen_ command to enable secure private DAC key usage with S50.
`chip_with_factory_data=1` must have been added to the _gn gen_ command

DAC private key generation: The argument `chip_convert_dac_private_key=1` must
be added to the _gn gen_ command to enable DAC private plain key conversion to
blob with S50. `chip_enable_secure_dac_private_key_storage=1` must have been
added to the _gn gen_ command

`ELS` contain concurrent access risks. They must be fixed before enabling it by
default.

<a name="flashing-and-debugging"></a>

## Flashing and debugging

### Flashing the Bridge-app application

In order to flash the application we recommend using
[MCUXpresso IDE (version >= 11.6.0)](https://www.nxp.com/design/software/development-software/mcuxpresso-software-and-tools-/mcuxpresso-integrated-development-environment-ide:MCUXpresso-IDE).

-   Import the previously downloaded NXP SDK into MCUXpresso IDE.

Right click the empty space in the MCUXpresso IDE "Installed SDKs" tab to show
the menu, select the "Import archive" (or "Import folder" if a folder is used)
menu item.

-   Import the connectedhomeip repo in MCUXpresso IDE as Makefile Project. Use
    _none_ as _Toolchain for Indexer Settings_:

```
File -> Import -> C/C++ -> Existing Code as Makefile Project
```

-   Configure MCU Settings:

```
Right click on the Project -> Properties -> C/C++ Build -> MCU Settings -> Select RW612 -> Apply & Close
```

![MCU_Set](../../../../platform/nxp/rt/rw61x/doc/images/mcu-set.PNG)

-   Configure the toolchain editor:

```
Right click on the Project -> C/C++ Build-> Tool Chain Editor -> NXP MCU Tools -> Apply & Close
```

![toolchain](../../../../platform/nxp/rt/rw61x/doc/images/toolchain.JPG)

-   Create a debug configuration :

```
Right click on the Project -> Debug -> As->SEGGER JLink probes -> OK -> Select elf file
```

(Note : if SDK package is used, a simpler way could be duplicating the debug
configuration from the SDK Hello World example after importing it.)

-   Debug using the newly created configuration file.

<a name="testing-the-example"></a>

## Testing the example

CHIP Tool is a Matter controller which can be used to commission a Matter device
into the network. For more information regarding how to use the CHIP Tool
controller, please refer to the
[CHIP Tool guide](../../../../../docs/guides/chip_tool_guide.md).

To know how to commission a device over BLE, follow the instructions from
[chip-tool's README.md 'Commission a device over
BLE'][readme_ble_commissioning_section].

[readme_ble_commissioning_section]:
    ../../../../chip-tool/README.md#commission-a-device-over-ble

To know how to commissioning a device over IP, follow the instructions from
[chip-tool's README.md 'Pair a device over
IP'][readme_pair_ip_commissioning_section]

[readme_pair_ip_commissioning_section]:
    ../../../../chip-tool/README.md#pair-a-device-over-ip

#### Matter over wifi configuration :

The "ble-wifi" pairing method can be used in order to commission the device.

### NVM

By default the file system used by the application is NVS.

### Testing the bridge application without Matter CLI:

1. Prepare the board with the flashed `Bridge application` (as shown
   above).
2. The Bridge example uses UART1 (`FlexComm3`) to print logs while running
   the server. To view raw UART output, start a terminal emulator like PuTTY and
   connect to the used COM port with the following UART settings:

    - Baud rate: 115200
    - 8 data bits
    - 1 stop bit
    - No parity
    - No flow control

3. Open a terminal connection on the board and watch the printed logs.

4. On the client side, start sending commands using the chip-tool application as
   it is described
   [here](../../../../chip-tool/README.md#using-the-client-to-send-matter-commands).

<a name="testing-the-bridge-application-with-matter-cli-enabled"></a>

### Testing the bridge application with Matter CLI enabled:

The Matter CLI can be enabled with the bridge application.

For more information about the Matter CLI default commands, you can refer to the
dedicated [ReadMe](../../../../shell/README.md).

The Bridge application supports additional commands :

```
> help
[...]
mattercommissioning     Open/close the commissioning window. Usage : mattercommissioning [on|off]
matterfactoryreset      Perform a factory reset on the device
matterreset             Reset the device
```

-   `matterfactoryreset` command erases the file system completely (all Matter
    settings are erased).
-   `matterreset` enables the device to reboot without erasing the settings.

Here are described steps to use the bridge-app with the Matter CLI enabled

1. Prepare the board with the flashed `Bridge application` (as shown
   above).
2. The matter CLI is accessible in UART1. For that, start a terminal emulator
   like PuTTY and connect to the used COM port with the following UART settings:

    - Baud rate: 115200
    - 8 data bits
    - 1 stop bit
    - No parity
    - No flow control

3. The Bridge example uses UART2 (`FlexComm0`) to print logs while running
   the server. To view raw UART output, a pin should be plugged to an USB to
   UART adapter (connector `HD2 pin 03`), then start a terminal emulator like
   PuTTY and connect to the used COM port with the following UART settings:

    - Baud rate: 115200
    - 8 data bits
    - 1 stop bit
    - No parity
    - No flow control

4. On the client side, start sending commands using the chip-tool application as
   it is described
   [here](../../../../chip-tool/README.md#using-the-client-to-send-matter-commands).

<a name="ota-software-update"></a>

## OTA Software Update

Over-The-Air software updates are supported with the RW61x bridge-app example.
The process to follow in order to perform a software update is described in the
dedicated guide
['Matter Over-The-Air Software Update with NXP RW61x example applications'](../../../../../docs/guides/nxp_rw61x_ota_software_update.md).
