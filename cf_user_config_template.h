/**
 * @file cf_user_config.h
 * @brief User configuration template for CFramework
 *
 * Copy this file to your project directory and customize as needed.
 * DO NOT modify framework source files!
 *
 * Integration steps:
 * 1. Copy this file to your project (e.g., config/cf_user_config.h)
 * 2. Define CF_USER_CONFIG in your build system
 * 3. Add include path to your config directory
 *
 * Example Makefile:
 *   CFLAGS += -DCF_USER_CONFIG
 *   INCLUDES += -I./config
 *   INCLUDES += -I./lib/CFramework/cf_core/include
 */

#ifndef CF_USER_CONFIG_H
#define CF_USER_CONFIG_H

//==============================================================================
// PLATFORM SELECTION (Required - define ONE)
//==============================================================================

// Uncomment the platform you're using:
// #define CF_PLATFORM_STM32F1
// #define CF_PLATFORM_STM32F4
// #define CF_PLATFORM_STM32L1
#define CF_PLATFORM_STM32L4
// #define CF_PLATFORM_ESP32

//==============================================================================
// RTOS CONFIGURATION (Optional overrides)
//==============================================================================

// #define CF_RTOS_ENABLED              1

//==============================================================================
// DEBUG CONFIGURATION (Optional overrides)
//==============================================================================

// #define CF_DEBUG                     1      // Enable debug features
// #define CF_ASSERT_ENABLED            1      // Enable assertions

//==============================================================================
// LOGGER CONFIGURATION (Optional overrides)
//==============================================================================

// #define CF_LOG_ENABLED               1      // Enable logging
// #define CF_LOG_MAX_SINKS             4      // Maximum number of log sinks
// #define CF_LOG_BUFFER_SIZE           512    // Log buffer size in bytes

//==============================================================================
// HAL CONFIGURATION (Optional overrides)
//==============================================================================

// #define CF_HAL_GPIO_MAX_HANDLES      16     // Maximum GPIO handles
// #define CF_HAL_UART_MAX_HANDLES      4      // Maximum UART handles

//==============================================================================
// THREADPOOL CONFIGURATION (Optional overrides)
//==============================================================================

// #define CF_THREADPOOL_ENABLED        1      // Enable ThreadPool
// #define CF_THREADPOOL_THREAD_COUNT   4      // Number of worker threads
// #define CF_THREADPOOL_QUEUE_SIZE     20     // Task queue size
// #define CF_THREADPOOL_STACK_SIZE     2048   // Stack size per thread

//==============================================================================
// MEMORY CONFIGURATION (Optional overrides)
//==============================================================================

// #define CF_MEMPOOL_ENABLED           1      // Enable memory pool
// #define CF_MEMPOOL_USE_STATIC        1      // Use static allocation

#endif /* CF_USER_CONFIG_H */
