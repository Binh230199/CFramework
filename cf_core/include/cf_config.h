/**
 * @file cf_config.h
 * @brief Framework configuration with user override support
 * @version 1.0.0
 * @date 2025-10-29
 * @author CFramework Contributors
 *
 * @copyright Copyright (c) 2025 CFramework
 * Licensed under MIT License
 *
 * ============================================================================
 * IMPORTANT: DO NOT MODIFY THIS FILE DIRECTLY!
 * ============================================================================
 *
 * This file contains framework defaults. To customize configuration:
 *
 * 1. Create cf_user_config.h in YOUR project directory
 * 2. Define CF_USER_CONFIG in your build system
 * 3. Override any settings you need
 *
 * Example Makefile:
 *   CFLAGS += -DCF_USER_CONFIG
 *   INCLUDES += -I./config
 *
 * Example cf_user_config.h:
 *   #define CF_PLATFORM_STM32F4
 *   #define CF_THREADPOOL_THREAD_COUNT 8
 */

#ifndef CF_CONFIG_H
#define CF_CONFIG_H

//==============================================================================
// USER CONFIGURATION OVERRIDE
//==============================================================================

#ifdef CF_USER_CONFIG
    #include "cf_user_config.h"
#endif

//==============================================================================
// PLATFORM SELECTION (User MUST define)
//==============================================================================

#if !defined(CF_PLATFORM_STM32F1) && \
    !defined(CF_PLATFORM_STM32F4) && \
    !defined(CF_PLATFORM_STM32L1) && \
    !defined(CF_PLATFORM_STM32L4) && \
    !defined(CF_PLATFORM_ESP32)
    #error "Platform not defined! Define CF_PLATFORM_xxx in cf_user_config.h or build flags"
#endif

//==============================================================================
// RTOS CONFIGURATION
//==============================================================================

#ifndef CF_RTOS_ENABLED
    #define CF_RTOS_ENABLED              1
#endif

#ifndef CF_RTOS_FREERTOS
    #define CF_RTOS_FREERTOS             1
#endif

//==============================================================================
// DEBUG CONFIGURATION
//==============================================================================

#ifndef CF_DEBUG
    #define CF_DEBUG                     1
#endif

#ifndef CF_ASSERT_ENABLED
    #define CF_ASSERT_ENABLED            CF_DEBUG
#endif

//==============================================================================
// LOGGER CONFIGURATION
//==============================================================================

#ifndef CF_LOG_ENABLED
    #define CF_LOG_ENABLED               1
#endif

#ifndef CF_LOG_MAX_SINKS
    #define CF_LOG_MAX_SINKS             4
#endif

#ifndef CF_LOG_BUFFER_SIZE
    #define CF_LOG_BUFFER_SIZE           512
#endif

//==============================================================================
// HAL CONFIGURATION
//==============================================================================

#ifndef CF_HAL_GPIO_MAX_HANDLES
    #define CF_HAL_GPIO_MAX_HANDLES      16
#endif

#ifndef CF_HAL_UART_MAX_HANDLES
    #define CF_HAL_UART_MAX_HANDLES      4
#endif

//==============================================================================
// THREADPOOL CONFIGURATION
//==============================================================================

#ifndef CF_THREADPOOL_ENABLED
    #define CF_THREADPOOL_ENABLED        1
#endif

#ifndef CF_THREADPOOL_THREAD_COUNT
    #define CF_THREADPOOL_THREAD_COUNT   4
#endif

#ifndef CF_THREADPOOL_QUEUE_SIZE
    #define CF_THREADPOOL_QUEUE_SIZE     20
#endif

#ifndef CF_THREADPOOL_STACK_SIZE
    #define CF_THREADPOOL_STACK_SIZE     2048
#endif

//==============================================================================
// EVENT SYSTEM CONFIGURATION
//==============================================================================

#ifndef CF_EVENT_ENABLED
    #define CF_EVENT_ENABLED             1
#endif

#ifndef CF_EVENT_MAX_SUBSCRIBERS
    #define CF_EVENT_MAX_SUBSCRIBERS     32
#endif

//==============================================================================
// MEMORY CONFIGURATION
//==============================================================================

#ifndef CF_MEMPOOL_ENABLED
    #define CF_MEMPOOL_ENABLED           1
#endif

#ifndef CF_MEMPOOL_USE_STATIC
    #define CF_MEMPOOL_USE_STATIC        1
#endif

//==============================================================================
// CONFIGURATION VALIDATION
//==============================================================================

#if CF_LOG_MAX_SINKS > 8
    #error "CF_LOG_MAX_SINKS too large (max 8)"
#endif

#if CF_LOG_BUFFER_SIZE < 128
    #error "CF_LOG_BUFFER_SIZE too small (min 128)"
#endif

#if CF_THREADPOOL_THREAD_COUNT > 16
    #error "CF_THREADPOOL_THREAD_COUNT too large (max 16)"
#endif

#if CF_THREADPOOL_THREAD_COUNT < 1
    #error "CF_THREADPOOL_THREAD_COUNT too small (min 1)"
#endif

#if CF_EVENT_MAX_SUBSCRIBERS > 64
    #error "CF_EVENT_MAX_SUBSCRIBERS too large (max 64)"
#endif

#if CF_EVENT_MAX_SUBSCRIBERS < 4
    #error "CF_EVENT_MAX_SUBSCRIBERS too small (min 4)"
#endif

#endif /* CF_CONFIG_H */
