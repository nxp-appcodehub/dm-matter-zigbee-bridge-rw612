#
#   Copyright (c) 2025 Project CHIP Authors
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#

cmake_minimum_required(VERSION 3.30)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Retrieve CHIP_ROOT if provided
if (DEFINED CHIP_ROOT)
    get_filename_component(CHIP_ROOT ${CHIP_ROOT} ABSOLUTE)
else()
    get_filename_component(CHIP_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../../sdk-next/mcuxsdk/middleware/matter REALPATH)
endif()

# Export CHIP_ROOT as an environment variable to be recognized by Kconfig
set(COMMON_KCONFIG_ENV_SETTINGS CHIP_ROOT=${CHIP_ROOT})

get_filename_component(GEN_DIR ${CHIP_ROOT}/zzz_generated/ REALPATH)
get_filename_component(NXP_ZIGBEE_BRIDGE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../zigbee_bridge/rt/rw61x/ZCB REALPATH)
get_filename_component(BRIGE_APP_COMMON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../bridge-common REALPATH)
get_filename_component(NXP_EXAMPLE_DIR ${CMAKE_CURRENT_SOURCE_DIR} REALPATH)
#get_filename_component(NXP_EXAMPLE_ZAP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/zap REALPATH)
get_filename_component(EXAMPLE_PLATFORM_NXP_COMMON_DIR ${CHIP_ROOT}/examples/platform/nxp/common REALPATH)
get_filename_component(NXP_MATTER_SUPPORT_DIR ${CHIP_ROOT}/third_party/nxp/nxp_matter_support REALPATH)

# Add the CHIP stack & NXP port libs as an external module
list(APPEND EXTRA_MCUX_MODULES ${CHIP_ROOT}/config/nxp/chip-cmake-freertos)

# If CONF_FILE_NAME is provided, append its full path to CONF_FILE
if (DEFINED CONF_FILE_NAME)
    set(CONF_FILE ${CHIP_ROOT}/examples/platform/nxp/config/${CONF_FILE_NAME} ${CONF_FILE})
endif()

# Add application prj.conf in front of CONF_FILE
set(CONF_FILE ${CMAKE_CURRENT_SOURCE_DIR}/prj.conf ${CONF_FILE})

set(KCONFIG_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/Kconfig)

# Load the MCUX SDK package
find_package(McuxSDK 3.0.0)

# Get compiler flags and includes and pass them to CHIP target
list(APPEND CONFIG_CHIP_EXTERNAL_TARGETS McuxSDK)

if (NOT DEFINED NXP_SDK_RECONFIG_CMAKE_DIR)
    get_filename_component(NXP_SDK_RECONFIG_CMAKE_DIR ${NXP_MATTER_SUPPORT_DIR}/examples/platform/${CONFIG_CHIP_NXP_PLATFORM_FAMILY} REALPATH)
endif()

# Include the nxp_sdk_reconfig.cmake to customize the SDK for Matter application
include(${NXP_SDK_RECONFIG_CMAKE_DIR}/nxp_sdk_reconfig.cmake)

project(chip-nxp-zigbee-bridge-app-example)

#**********************************************************
# Add Application files to the build
include(${CHIP_ROOT}/src/app/chip_data_model.cmake)

# Add NXP common application files
include(${EXAMPLE_PLATFORM_NXP_COMMON_DIR}/app_common.cmake)

target_include_directories(app
    PRIVATE
    ${CHIP_ROOT}/src/platform/nxp/common
    ${NXP_EXAMPLE_DIR}/common/main/include
    ${NXP_EXAMPLE_DIR}/rt/rw61x/include/config
    ${NXP_EXAMPLE_DIR}/rt/rw61x/main/include
    ${BRIGE_APP_COMMON_DIR}/include
    ${NXP_ZIGBEE_BRIDGE_DIR}
    ${GEN_DIR}/app-common
)

mcux_add_include(
    BASE_PATH ${NXP_ZIGBEE_BRIDGE_DIR}
    INCLUDES .
)

target_compile_definitions(chip INTERFACE
    CHIP_DEVICE_CONFIG_ENABLE_ZIGBEE=1
)
add_compile_definitions(
    CHIP_DEVICE_CONFIG_DYNAMIC_ENDPOINT_COUNT=${CONFIG_DYNAMIC_ENDPOINT_COUNT}
    TRACE_APP=${CONFIG_ZB_COORD_TRACE_APP}
    TRACE_ZCL=${CONFIG_ZB_COORD_TRACE_ZCL}
    ZIGBEE_EVENT_IMPL=1
)
string(REPLACE "-DWIFI_FW_ADDRESS=0" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
string(REPLACE "-DWIFI_FW_ADDRESS=0" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")

target_sources(app
    PRIVATE
    ${BRIGE_APP_COMMON_DIR}/src/binding-handler.cpp
    ${BRIGE_APP_COMMON_DIR}/src/bridged-actions-stub.cpp
    ${BRIGE_APP_COMMON_DIR}/src/smco-stub.cpp
    ${NXP_EXAMPLE_DIR}/common/main/AppTask.cpp
    ${NXP_EXAMPLE_DIR}/common/main/DeviceCallbacks.cpp
    ${NXP_EXAMPLE_DIR}/common/main/main.cpp
    ${NXP_EXAMPLE_DIR}/rt/rw61x/main/src/ClustersCommandHandlers.cpp
    ${NXP_EXAMPLE_DIR}/rt/rw61x/main/src/Bridge.cpp
    ${NXP_EXAMPLE_DIR}/rt/rw61x/main/src/BridgeActions.cpp
    ${NXP_EXAMPLE_DIR}/rt/rw61x/main/src/BridgeMgr.cpp
    ${NXP_EXAMPLE_DIR}/rt/rw61x/main/src/Device.cpp
    ${NXP_EXAMPLE_DIR}/rt/rw61x/main/src/ZigbeeCmds.cpp
    ${NXP_ZIGBEE_BRIDGE_DIR}/app_coordinator.c
    ${NXP_ZIGBEE_BRIDGE_DIR}/bdb_options.h
    ${NXP_ZIGBEE_BRIDGE_DIR}/zcl_options.h
    ${NXP_ZIGBEE_BRIDGE_DIR}/app_main.h
    ${NXP_ZIGBEE_BRIDGE_DIR}/app_main.c
    ${NXP_ZIGBEE_BRIDGE_DIR}/app_start.c
    ${NXP_ZIGBEE_BRIDGE_DIR}/app_zcl_task.c
    ${NXP_ZIGBEE_BRIDGE_DIR}/cmd.c
    ${NXP_ZIGBEE_BRIDGE_DIR}/pdum_apdu.S
    ${NXP_ZIGBEE_BRIDGE_DIR}/pdum_gen.c
    ${NXP_ZIGBEE_BRIDGE_DIR}/pdum_gen.h
    ${NXP_ZIGBEE_BRIDGE_DIR}/shell.c
    ${NXP_ZIGBEE_BRIDGE_DIR}/zcb.c
    ${NXP_ZIGBEE_BRIDGE_DIR}/zigbee_cmd.c
    ${NXP_ZIGBEE_BRIDGE_DIR}/ZigbeeDevices.c
    ${NXP_ZIGBEE_BRIDGE_DIR}/zps_gen.c
    ${NXP_ZIGBEE_BRIDGE_DIR}/zps_gen.h
)

if(CONFIG_CHIP_WIFI AND CONFIG_NET_L2_OPENTHREAD)
    message(FATAL_ERROR "WiFi + Thread (Border Router) configuration is not supported in this application.")
else()
    chip_configure_data_model(app
        INCLUDE_SERVER
        ZAP_FILE ${BRIGE_APP_COMMON_DIR}/bridge-app.zap
    )
endif()