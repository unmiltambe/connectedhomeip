/*
 *    Copyright (c) 2020 Project CHIP Authors
 *    Copyright (c) 2020 Google LLC.
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

/**
 *    @file
 *          Example project configuration file for CHIP.
 *
 *          This is a place to put application or project-specific overrides
 *          to the default configuration values for general CHIP features.
 *
 */

#pragma once

// Security and Authentication disabled for development build.
// For convenience, enable CHIP Security Test Mode and disable the requirement for
// authentication in various protocols.
// WARNING: These options make it possible to circumvent basic CHIP security functionality,
// including message encryption. Because of this they MUST NEVER BE ENABLED IN PRODUCTION BUILDS.
#define CHIP_CONFIG_SECURITY_TEST_MODE 0

// Use hard-coded test certificates already embedded in generic chip code => set it to 0
// Use real/development certificates => set it to 1 + file the provisioning section from
//                                      the internal flash
#ifndef CONFIG_CHIP_PLAT_LOAD_REAL_FACTORY_DATA
#define CONFIG_CHIP_PLAT_LOAD_REAL_FACTORY_DATA 0
#endif

#if CONFIG_CHIP_PLAT_LOAD_REAL_FACTORY_DATA

// VID/PID for product => will be used by Basic Information Cluster
#define CHIP_DEVICE_CONFIG_DEVICE_VENDOR_ID 0x1037
#define CHIP_DEVICE_CONFIG_DEVICE_PRODUCT_ID 0xA401

// Set the following define to use the Certification Declaration from below and not use it stored in factory data section
#ifndef CHIP_USE_DEVICE_CONFIG_CERTIFICATION_DECLARATION
#define CHIP_USE_DEVICE_CONFIG_CERTIFICATION_DECLARATION 0
#endif

#ifndef CHIP_DEVICE_CONFIG_CERTIFICATION_DECLARATION
//-> format_version = 1
//-> vendor_id = 0x1037
//-> product_id_array = [ 0xA401 ]
//-> device_type_id = 0x0100
//-> certificate_id = "ZIG20142ZB330003-24"
//-> security_level = 0
//-> security_information = 0
//-> version_number = 0x2694
//-> certification_type = 1
//-> dac_origin_vendor_id is not present
//-> dac_origin_product_id is not present
#define CHIP_DEVICE_CONFIG_CERTIFICATION_DECLARATION                                                                               \
    {                                                                                                                              \
        0x30, 0x81, 0xe8, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x07, 0x02, 0xa0, 0x81, 0xda, 0x30, 0x81, 0xd7,    \
            0x02, 0x01, 0x03, 0x31, 0x0d, 0x30, 0x0b, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01, 0x30,      \
            0x45, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x07, 0x01, 0xa0, 0x38, 0x04, 0x36, 0x15, 0x24, 0x00,      \
            0x01, 0x25, 0x01, 0x37, 0x10, 0x36, 0x02, 0x05, 0x01, 0xa4, 0x18, 0x25, 0x03, 0x00, 0x01, 0x2c, 0x04, 0x13, 0x5a,      \
            0x49, 0x47, 0x32, 0x30, 0x31, 0x34, 0x32, 0x5a, 0x42, 0x33, 0x33, 0x30, 0x30, 0x30, 0x33, 0x2d, 0x32, 0x34, 0x24,      \
            0x05, 0x00, 0x24, 0x06, 0x00, 0x25, 0x07, 0x76, 0x98, 0x24, 0x08, 0x01, 0x18, 0x31, 0x7c, 0x30, 0x7a, 0x02, 0x01,      \
            0x03, 0x80, 0x14, 0x62, 0xfa, 0x82, 0x33, 0x59, 0xac, 0xfa, 0xa9, 0x96, 0x3e, 0x1c, 0xfa, 0x14, 0x0a, 0xdd, 0xf5,      \
            0x04, 0xf3, 0x71, 0x60, 0x30, 0x0b, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01, 0x30, 0x0a,      \
            0x06, 0x08, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x04, 0x03, 0x02, 0x04, 0x46, 0x30, 0x44, 0x02, 0x20, 0x11, 0xc4, 0xe4,      \
            0x54, 0xcc, 0xdb, 0x09, 0xa9, 0x31, 0xd7, 0xbd, 0x6e, 0x28, 0x95, 0x9b, 0xab, 0x3e, 0xec, 0x76, 0x09, 0x8c, 0x39,      \
            0x93, 0x43, 0x6e, 0x89, 0x07, 0x7d, 0x8b, 0xe9, 0x3a, 0x0a, 0x02, 0x20, 0x71, 0xc6, 0xac, 0x09, 0x11, 0x7b, 0x1a,      \
            0x61, 0x5e, 0x3a, 0xc6, 0x4a, 0x4f, 0xf3, 0xd4, 0x3b, 0x62, 0x54, 0x3a, 0xf3, 0x60, 0xeb, 0x47, 0xd4, 0x8f, 0x18,      \
            0x0d, 0xa3, 0xd1, 0xef, 0xd0, 0x70                                                                                     \
    }

// All remaining data will be pulled from the provisioning region of flash.
#endif

#else

/**
 * CHIP_DEVICE_CONFIG_DEVICE_VENDOR_ID
 *
 * 0xFFF1: Test vendor.
 */
#define CHIP_DEVICE_CONFIG_DEVICE_VENDOR_ID 0xFFF1

/**
 * CHIP_DEVICE_CONFIG_DEVICE_PRODUCT_ID
 *
 * 0x8005: example lighting-app
 */
#define CHIP_DEVICE_CONFIG_DEVICE_PRODUCT_ID 0x8005

// Use a default setup PIN code if one hasn't been provisioned in flash.
#ifndef CHIP_DEVICE_CONFIG_USE_TEST_SETUP_PIN_CODE
#define CHIP_DEVICE_CONFIG_USE_TEST_SETUP_PIN_CODE 20202021
#endif

#ifndef CHIP_DEVICE_CONFIG_USE_TEST_SETUP_DISCRIMINATOR
#define CHIP_DEVICE_CONFIG_USE_TEST_SETUP_DISCRIMINATOR 0xF00
#endif

// Use a default pairing code if one hasn't been provisioned in flash.
#define CHIP_DEVICE_CONFIG_USE_TEST_PAIRING_CODE "CHIPUS"

/**
 * CHIP_DEVICE_CONFIG_TEST_SERIAL_NUMBER
 *
 * Enables the use of a hard-coded default serial number if none
 * is found in CHIP NV storage.
 */
#define CHIP_DEVICE_CONFIG_TEST_SERIAL_NUMBER "TEST_SN"

#endif // CONFIG_CHIP_PLAT_LOAD_REAL_FACTORY_DATA

/**
 * CHIP_DEVICE_CONFIG_DEVICE_HARDWARE_VERSION
 *
 * The hardware version number assigned to device or product by the device vendor.  This
 * number is scoped to the device product id, and typically corresponds to a revision of the
 * physical device, a change to its packaging, and/or a change to its marketing presentation.
 * This value is generally *not* incremented for device software versions.
 */
#define CHIP_DEVICE_CONFIG_DEVICE_HARDWARE_VERSION 100

#ifndef CHIP_DEVICE_CONFIG_DEFAULT_DEVICE_HARDWARE_VERSION_STRING
#define CHIP_DEVICE_CONFIG_DEFAULT_DEVICE_HARDWARE_VERSION_STRING "v0.1.0"
#endif

/**
 * CHIP_DEVICE_CONFIG_DEVICE_SOFTWARE_VERSION_STRING
 *
 * A string identifying the software version running on the device.
 * CHIP currently expects the software version to be in the format
 * {MAJOR_VERSION}.0d{MINOR_VERSION}
 */
#ifndef CHIP_DEVICE_CONFIG_DEVICE_SOFTWARE_VERSION_STRING
#define CHIP_DEVICE_CONFIG_DEVICE_SOFTWARE_VERSION_STRING NXP_CONFIG_DEVICE_SOFTWARE_VERSION_STRING
#endif

#ifndef CHIP_DEVICE_CONFIG_DEVICE_SOFTWARE_VERSION
#define CHIP_DEVICE_CONFIG_DEVICE_SOFTWARE_VERSION NXP_CONFIG_DEVICE_SOFTWARE_VERSION
#endif

#ifndef CHIP_DEVICE_CONFIG_DEVICE_VENDOR_NAME
#define CHIP_DEVICE_CONFIG_DEVICE_VENDOR_NAME "NXP Semiconductors"
#endif

#ifndef CHIP_DEVICE_CONFIG_DEVICE_PRODUCT_NAME
#define CHIP_DEVICE_CONFIG_DEVICE_PRODUCT_NAME "NXP Demo App"
#endif

/**
 * CHIP_DEVICE_CONFIG_ENABLE_CHIP_TIME_SERVICE_TIME_SYNC
 *
 * Enables synchronizing the device's real time clock with a remote CHIP Time service
 * using the CHIP Time Sync protocol.
 */
// #define CHIP_DEVICE_CONFIG_ENABLE_CHIP_TIME_SERVICE_TIME_SYNC 1

/**
 * CHIP_DEVICE_CONFIG_BLE_FAST_ADVERTISING_TIMEOUT
 *
 * The amount of time in miliseconds after which BLE should change his advertisements
 * from fast interval to slow interval.
 *
 * 30000 (30 secondes).
 */
#define CHIP_DEVICE_CONFIG_BLE_FAST_ADVERTISING_TIMEOUT (30 * 1000)

/**
 * CHIP_DEVICE_CONFIG_BLE_ADVERTISING_TIMEOUT
 *
 * The amount of time in miliseconds after which BLE advertisement should be disabled, counting
 * from the moment of slow advertisement commencement.
 *
 * Defaults to 9000000 (15 minutes).
 */
#define CHIP_DEVICE_CONFIG_BLE_ADVERTISING_TIMEOUT (15 * 60 * 1000)

/**
 * CONFIG_CHIP_NFC_ONBOARDING_PAYLOAD, CHIP_DEVICE_CONFIG_ENABLE_NFC_ONBOARDING_PAYLOAD
 *
 * NFC onboarding payload is not supported
 */
#define CONFIG_CHIP_NFC_ONBOARDING_PAYLOAD 0
#define CHIP_DEVICE_CONFIG_ENABLE_NFC_ONBOARDING_PAYLOAD 0

/**
 *  @def CHIP_CONFIG_MAX_FABRICS
 *
 *  @brief
 *    Maximum number of fabrics the device can participate in.  Each fabric can
 *    provision the device with its unique operational credentials and manage
 *    its own access control lists.
 */
#define CHIP_CONFIG_MAX_FABRICS 5 // 5 is the minimum number of supported fabrics

/**
 * @def CHIP_IM_MAX_NUM_COMMAND_HANDLER
 *
 * @brief Defines the maximum number of CommandHandler, limits the number of active commands transactions on server.
 */
#define CHIP_IM_MAX_NUM_COMMAND_HANDLER 2

/**
 * @def CHIP_IM_MAX_NUM_WRITE_HANDLER
 *
 * @brief Defines the maximum number of WriteHandler, limits the number of active write transactions on server.
 */
#define CHIP_IM_MAX_NUM_WRITE_HANDLER 2

/**
 * CHIP_CONFIG_EVENT_LOGGING_DEFAULT_IMPORTANCE
 *
 * For a development build, set the default importance of events to be logged as Debug.
 * Since debug is the lowest importance level, this means all standard, critical, info and
 * debug importance level vi events get logged.
 */
#if BUILD_RELEASE
#define CHIP_CONFIG_EVENT_LOGGING_DEFAULT_IMPORTANCE chip::Profiles::DataManagement::Production
#else
#define CHIP_CONFIG_EVENT_LOGGING_DEFAULT_IMPORTANCE chip::Profiles::DataManagement::Debug
#endif // BUILD_RELEASE

#define CHIP_DEVICE_CONFIG_ENABLE_EXTENDED_DISCOVERY 1
