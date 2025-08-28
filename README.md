# NXP Application Code Hub
[<img src="https://mcuxpresso.nxp.com/static/icon/nxp-logo-color.svg" width="100"/>](https://www.nxp.com)

## Matter Zigbee Bridge FRDM-RW612
This user guide provides instructions on how to bridge non Matter devices such as Legacy Zigbee devices to the Matter ecosystem using NXP Wireless MCU system composed of NXP RW612.

Design infrastructure:

![picture](images/Infrastructure.png)

#### Boards: FRDM-RW612
#### Categories: Wireless Connectivity, Bridge, RTOS
#### Peripherals: UART
#### Toolchains: GCC

## Table of Contents
1. [List of branches](#step1)
1. [Getting started](#step2)
2. [Support](#step3)
3. [Release Notes](#step4)

## 1. List of branches<a name="step1"></a>
The list of branches in this repository include:
* main
  * Contains this README
* [Matter Zigbee Bridge](https://github.com/nxp-appcodehub/dm-matter-zigbee-bridge-rw612/blob/rw612_only/README.md)
  * Contains instructions to run the Matter Zigbee Bridge. This demo consists of the Matter Zigbee bridge application running on NXP FRDM-RW612. The Matter network is available over the Wi-Fi interface and the application bridges the Zigbee devices to the Matter network. 
* [Matter Zigbee Bridge with OpenThread support](https://github.com/nxp-appcodehub/dm-matter-zigbee-bridge-rw612/blob/rw612_with_external_transceiver/README.md)
  * Contains instructions to run the Matter Zigbee Bridge with OpenThread Border Router (OTBR) support. This demo consists of the Matter Zigbee bridge application with OTBR support running on NXP FRDM-RW612 and connected through UART to a Zigbee Control Bridge application running on NXP K32W0x1DK6. The Matter network is available over the Wi-Fi and Thread interfaces and the application bridges the Zigbee devices to the Matter network.

## 2. Getting started<a name="step2"></a>
Each branch has a README file with instructions for getting started.  To review, you can use a browser to view the [Matter Zigbee Bridge branches](https://github.com/nxp-appcodehub/dm-matter-zigbee-bridge-rw612/branches), and find the desired README.


## 3. Support<a name="step3"></a>
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

## 4. Release Notes<a name="step4"></a>
| Version | Description / Update                           | Date                        |
|:-------:|------------------------------------------------|----------------------------:|
| 1.0     | Initial release on Application Code Hub        | November 14<sup>th</sup> 2024 |
| 1.1     | Update to Matter 1.4                           | April 25<sup>th</sup> 2025  |
| 2.0     | Release of RW612 Wi-Fi+Zigbee solution         | September 25<sup>th</sup> 2025  |