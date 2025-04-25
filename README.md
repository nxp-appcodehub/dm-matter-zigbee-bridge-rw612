# NXP Application Code Hub
[<img src="https://mcuxpresso.nxp.com/static/icon/nxp-logo-color.svg" width="100"/>](https://www.nxp.com)

## Matter Zigbee Bridge FRDM-RW612
This user guide provides instructions on how to bridge non Matter devices such as Legacy Zigbee devices to the Matter ecosystem using NXP Wireless MCU system composed of NXP RW612 (with OpenThread Border Router support - Openthread + WiFi) and NXP K32W0x1 (with Zigbee support).

Design infrastructure:

![picture](images/Infrastructure.png)

Hardware connections:

![picture](images/HW-Connection.png)

#### Boards: FRDM-RW612
#### Categories: Wireless Connectivity, Bridge, RTOS
#### Peripherals: UART
#### Toolchains: GCC

## Table of Contents
1. [Software](#step1)
2. [Hardware](#step2)
3. [Setup](#step3)
4. [Running the demo](#step4)
4. [Results](#step5)
5. [Support](#step6)
6. [Release Notes](#step7)

## 1. Software requirements<a name="step1"></a>
-	Ubuntu 22.04 as standalone PC or installed in the Virtual Machine like VirtualBox
-	JLink version (> v.792f)
-	K32W061 SDK_2_6_16
-	DK6Programmer.exe (SDK_2_6_16_K32W061DK6\tools\JN-SW-4407-DK6-Flash-Programmer\JN-SW-4407 DK6 Production Flash Programmer v4564.exe)
- [JN-AN-1247](https://www.nxp.com/webapp/Download?colCode=JN-AN-1247&appType=license) - Zigbee Control Bridge:
 JN-AN-1247\Binaries\ControlBridge_Full_GpProxy_1000000\ControlBridge_Full_GpProxy_1000000.bin (build with baud-rate set to 1000000bps) 
- [JN-AN-1244](https://www.nxp.com/webapp/Download?colCode=JN-AN-1244&appType=license) - Zigbee Color Light:
 JN-AN-1244\Binaries\ExtendedColorLight_GpProxy_OM15081\ExtendedColorLight_GpProxy_OM15081.bin (Prebuild)
-	[JN-AN-1246](https://www.nxp.com/webapp/Download?colCode=JN-AN-1246&appType=license) - Zigbee Temperature Sensor :
 JN-AN-1246\Binaries \LTOSensor_NtagIcode_Ota_OM15081R2\LTOSensor_NtagIcode_Ota_OM15081R2_V1.bin (Prebuild)
-	FRDM-RW612 Matter-Zigbee-Bridge: /examples/bridge-app/nxp/rt/rw61x/out/debug/chip-rw61x-bridge-example.srec
-	MatterOverThread Light: /examples/lighting-app/nxp/mcxw71/out/debug/chip-mcxw71-light-example.srec


## 2. Hardware requirements<a name="step2"></a>
-  Matter controller based on Matter 1.4 or newer
-	FRDM-RW612 board
-	K32W061-DK6 (MEZZANINE module + OM15076-3 Carrier Board) as Zigbee Coordinator 
	-	FRDM-RW612 J1 Pin 2 (GPIO_9) FC1_UART_RX <=>  K32W061-DK6 J3 Pin 15 USART0_TX
	-	FRDM-RW612 J1 Pin 4 (GPIO_8) FC1_UART_TX <=>  K32W061-DK6 J3 Pin 16 USART0_RX
	-	FRDM-RW612 J6 Pin 2 (GPIO_19) RST        <=>  K32W061-DK6 J3 Pin 30 RSTN
-	USB-UART Converter as Matter information Logging
	-	FRDM-RW612 J5 Pin 4 (GPIO_03) FC0_UART_TXD <=>  Converter RX
	-	FRDM-RW612 J5 Pin 8 (GND)                  <=>  Converter GND
-	FRDM-MCXW71 as MatterOverThread Light
-	K32W061 as Zigbee ColorLight
-	K32W061 as Zigbee LightTemperatureOccupancy(LTO) Sensor

## 3. Setup<a name="step3"></a>
1. Creating the build environment:
   - Initialize the Matter repo as submodule inside *bridge-app/nxp/rt/rw61x/third_party*:

         git submodule update --init
   
   - Copy the patches from *patches* folder to *bridge-app/nxp/rt/rw61x/third_party*:

         cp patches/* bridge-app/nxp/rt/rw61x/third_party/
   
   - Copy the *zigbee_bridge* folder to *bridge-app/nxp/rt/rw61x/third_party/matter/third_party/nxp*:

         cp -r zigbee_bridge/ bridge-app/nxp/rt/rw61x/third_party/matter/third_party/nxp/

   - Go to the matter root folder:

         cd bridge-app/nxp/rt/rw61x/third_party/matter/

   - Install matter required packages:

         sudo apt-get install git gcc g++ pkg-config libssl-dev libdbus-1-dev libglib2.0-dev
         libavahi-client-dev ninja-build python3-venv python3-dev python3-pip unzip
         libgirepository1.0-dev libcairo2-dev libreadline-dev

   - Checkout NXP specific Matter submodules:

         scripts/checkout_submodules.py --shallow --platform nxp --recursive

   - Run the bootstrap script to initialize the Matter build environment

         source scripts/bootstrap.sh

   - If bootstrap was already done the environment activation can be done by using the activate script: 

         source scripts/activate.sh
   
   **Note:** The environment will be active only in the terminal from which the bootstrap/activate script was executed.       

   - Apply the following patches for the */src* and */example* folders:
         
         git apply ../example-changes.patch

         git apply ../src-changes.patch

   - Initialize the NXP SDK:      
         
         third_party/nxp/nxp_matter_support/scripts/update_nxp_sdk.py --platform common

   - Go the the *nxp_matter_support* folder:

         cd third_party/nxp/nxp_matter_support/

   - Apply the following patch to modify the board files and enable the UART communication with the Zigbee coordinator:

         git apply ../../../../sdk-changes.patch
   
2. Build the FRDM-RW612 Matter-Zigbee bridge app
   
   - Go back to the *bridge-app/nxp/rt/rw61x* folder

         cd ../../../../../

   - Generate the build files for the FRDM-RW612 board and with the Thread BR option:

         gn gen --args="chip_enable_wifi=true chip_enable_openthread=true nxp_enable_matter_cli=true board_version=\"frdm\"" out/debug

   - Build the example:

         ninja -C out/debug/
    
   - After the build finishes, the application binary (.srec file) can be found in the out/debug/ folder under the name *chip-rw61x-bridge-example.srec*

   - Flash the application binary on the board by following [these instructions](https://github.com/NXP/ot-nxp/tree/release/v1.4.0/src/rw/rw612#flash-binaries)

3. Build the FRDM-MCXW71 Matter On/Off light app

   - Go back to the root Matter folder
         
         cd third_party/matter

   - Build and flash the Matter On/Off light app for FRDM-MCXW71 following the instructions provided in the dedicated [readme file](https://github.com/NXP/matter/blob/release/v1.4.0/examples/lighting-app/nxp/mcxw71/README.md)

4. Flash the two K32W061 boards with the Zigbee control bridge and Zigbee color light binaries

   - Flash the binaries on the boards using the [following instructions](https://github.com/NXP/ot-nxp/tree/release/v1.4.0/src/k32w0/k32w061#flash-binaries) 

## 4. Running the demo<a name="step4"></a>

1. Join FRDM-RW612 to Matter network via Wi-fi   
   boot up FRDM-RW612 and wait until message “CHIPoBLE advertising started” appears in its logs then run:
   
        chip-tool pairing ble-wifi 1 SSID Passwd 20202021 3840

   wait until message “Device commissioning completed with success” visible on Raspberry Pi which indicates this FRDM-RW612 has been successfully joined the Matter controller as MatterOverWifi device.
2. Setup OpenThread Border Router (OTBR) on FRDM-RW612 by following commands on its CLI
   -  otcli dataset init new
   -  otcli dataset panid 0xabcd     --- 0xabcd can be changed to other value
   -  otcli dataset channel 25       --- 25 can be changed between 11~26
   -  otcli dataset commit active
   -  otcli ifconfig up
   -  otcli thread start
   -  otcli state                    --- must wait until “leader” state appears
   -  otcli dataset active –x        --- thread dataset used in chip-tool similar to following : 0e08000000000001000035060004001fffe002088711152e77458a490708fdcbf744a91020cb05100c208752e1bd2586f0a87ed481890312030f4f70656e5468726561642d633130640410d60d95cb5db1044086f7813e66de19020c0402a0f7f80102abcd0003000019

3. Join FRDM-MCXW71 MatterOverThread Light App to FRDM-RW612 OTBR
   Press SW2 on Factory-New FRDM-MCXW71 flashed with chip-mcxw71-light-example.srec and “Started BLE Advertising” visible on its UART console then run:
   
      chip-tool pairing ble-thread 2 hex: 0e08000000000001000035060004001fffe002088711152e77458a490708fdcbf744a91020cb05100c208752e1bd2586f0a87ed481890312030f4f70656e5468726561642d633130640410d60d95cb5db1044086f7813e66de19020c0402a0f7f80102abcd0003000019  20202021 3840

   wait until message “Device commissioning completed with success” appears on the Matter controller's logs, which confirms that this K32W148 has been successfully joined Matter network through FRDM-RW612 OTBR as MatterOverThread device.

4. Set up Matter ZB Bridge
   run following commands on FRDM-RW612 CLI to form Zigbee network on external K32W061 MEZZANINE module that flashed with ControlBridge_Full_GpProxy_115200.bin then permit other Zigbee nodes to join it:
   -  zb-erasepdm         --- erase currently used Zigbee channel, skip this if want to continue on existing channel
   -  zb-nwk-form 11      --- can be any value between 11~26 as valid Zigbee channel
   -  zb-nwk-pjoin 255    --- 255 to enable and 0 to disable permit join
   
   Power on factory-new Color Light K32W061 flashed with ExtendedColorLight_GpProxy_OM15081.bin, following messages will dump on FRDM-RW612 console:
   
        Add Color Light
        Node Type=3,Short=0x18d4,MAC=0x158d00031f1742,EP=12928
		
	(Here EP=12928 is the dynamic allocated endpoint that map the Non-Matter Zigbee node as Matter device, if more Zigbee nodes joined the FRDM-RW612 Matter network, then increased
    dynamic endpoint as 12929,12930 … will be assigned to these additional nodes individually.)
	Power on factory-new Temperature Sensor K32W061 flashed with LTOSensor_NtagIcode_Ota_OM15081R2_V1.bin, similar message visible on FRDM-RW612 console.
   

## 5. Results<a name="step5"></a>
Run following commands through Matter controller console and RGB on Color Light K32W061 will be changed accordingly:
   -  chip-tool onoff toggle 1 12928                                         --- RGB alternately On and Off
   -  chip-tool levelcontrol move-to-level 2 1 1 1 1 12928                   --- RGB bright level changed to 2
   -  chip-tool colorcontrol move-to-hue 32 1 1 1 1 1 12928                  --- RGB hue changed to 32
   -  chip-tool colorcontrol move-to-saturation 64 1 1 1 1 12928             --- RGB saturation changed to 64
   -  chip-tool colorcontrol move-to-color-temperature 128 1 1 1 1 12928     --- RGB temperature changed to 128
   -  chip-tool colorcontrol move-to-color 30000 60000 1 1 1 1 12928         --- RGB ColorX and ColorY changed to 30000 and 60000 respectively


## 6. Support<a name="step6"></a>
If you need help, please contact FAE or create a ticket to [NXP Community](https://community.nxp.com/).

#### Project Metadata

<!----- Boards ----->
[![Board badge](https://img.shields.io/badge/Board-FRDM&ndash;RW612-blue)]()

<!----- Categories ----->
[![Category badge](https://img.shields.io/badge/Category-WIRELESS%20CONNECTIVITY-yellowgreen)](https://github.com/search?q=org%3Anxp-appcodehub+wireless_connectivity+in%3Areadme&type=Repositories)
[![Category badge](https://img.shields.io/badge/Category-BRIDGE-yellowgreen)](https://github.com/search?q=org%3Anxp-appcodehub+bridge+in%3Areadme&type=Repositories)
[![Category badge](https://img.shields.io/badge/Category-RTOS-yellowgreen)](https://github.com/search?q=org%3Anxp-appcodehub+rtos+in%3Areadme&type=Repositories)

<!----- Peripherals ----->
[![Peripheral badge](https://img.shields.io/badge/Peripheral-UART-yellow)](https://github.com/search?q=org%3Anxp-appcodehub+uart+in%3Areadme&type=Repositories)

<!----- Toolchains ----->
[![Toolchain badge](https://img.shields.io/badge/Toolchain-GCC-orange)](https://github.com/search?q=org%3Anxp-appcodehub+gcc+in%3Areadme&type=Repositories)

Questions regarding the content/correctness of this example can be entered as Issues within this GitHub repository.

>**Warning**: For more general technical questions regarding NXP Microcontrollers and the difference in expected functionality, enter your questions on the [NXP Community Forum](https://community.nxp.com/)

[![Follow us on Youtube](https://img.shields.io/badge/Youtube-Follow%20us%20on%20Youtube-red.svg)](https://www.youtube.com/NXP_Semiconductors)
[![Follow us on LinkedIn](https://img.shields.io/badge/LinkedIn-Follow%20us%20on%20LinkedIn-blue.svg)](https://www.linkedin.com/company/nxp-semiconductors)
[![Follow us on Facebook](https://img.shields.io/badge/Facebook-Follow%20us%20on%20Facebook-blue.svg)](https://www.facebook.com/nxpsemi/)
[![Follow us on Twitter](https://img.shields.io/badge/X-Follow%20us%20on%20X-black.svg)](https://x.com/NXP)

## 7. Release Notes<a name="step7"></a>
| Version | Description / Update                           | Date                        |
|:-------:|------------------------------------------------|----------------------------:|
| 1.0     | Initial release on Application Code Hub        | November 14<sup>th</sup> 2024 |
| 1.1     | Update to Matter 1.4                           | April 25<sup>th</sup> 2025  |