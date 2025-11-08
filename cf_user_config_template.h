/**
 * @file cf_user_config.h
 * @brief User configuration template for CFramework
 *
 * Copy this file to your project directory and customize as needed.
 * DO NOT modify framework source files!
 *
 * Integration steps:
 * 1. Copy this file to your project (e.g., Core/Inc/cf_user_config.h)
 * 2. Define CF_USER_CONFIG in your build system
 * 3. Add CFramework include paths to your project
 *
 * Example Makefile/CMake:
 *   CFLAGS += -DCF_USER_CONFIG
 *   INCLUDES += -I./Core/Inc
 *   INCLUDES += -I./CFramework/cf_core/include
 *   INCLUDES += -I./CFramework/cf_middleware
 */

#ifndef CF_USER_CONFIG_H
#define CF_USER_CONFIG_H

//==============================================================================
// RTOS CONFIGURATION (Required)
//==============================================================================

#define CF_RTOS_ENABLED              1      // Enable RTOS support (required)

//==============================================================================
// DEBUG CONFIGURATION (Optional overrides)
//==============================================================================

// #define CF_DEBUG                     1      // Enable debug features
// #define CF_ASSERT_ENABLED            1      // Enable assertions

//==============================================================================
// LOGGER CONFIGURATION (Optional overrides)
//==============================================================================

#define CF_LOG_ENABLED               1      // Enable logging
// #define CF_LOG_MAX_SINKS             4      // Maximum number of log sinks
// #define CF_LOG_BUFFER_SIZE           512    // Log buffer size in bytes

/**
 * If using UART log sink, define the platform-specific UART handle type.
 * This type is used by cf_log_uart_sink.h
 *
 * For STM32:  UART_HandleTypeDef*
 * For ESP32:  uart_port_t (int)
 * Default:    void*
 */
#define CF_UART_HANDLE_TYPE          UART_HandleTypeDef*  // For STM32

//==============================================================================
// THREADPOOL CONFIGURATION (Optional overrides)
//==============================================================================

#define CF_THREADPOOL_ENABLED        1      // Enable ThreadPool
// #define CF_THREADPOOL_THREAD_COUNT   4      // Number of worker threads
// #define CF_THREADPOOL_QUEUE_SIZE     20     // Task queue size
// #define CF_THREADPOOL_STACK_SIZE     2048   // Stack size per thread

//==============================================================================
// EVENT SYSTEM CONFIGURATION (Optional overrides)
//==============================================================================

#define CF_EVENT_ENABLED             1      // Enable Event System
// #define CF_EVENT_MAX_SUBSCRIBERS     32     // Maximum subscribers per event
// #define CF_EVENT_QUEUE_SIZE          20     // Event queue size

//==============================================================================
// MEMORY CONFIGURATION (Optional overrides)
//==============================================================================

// #define CF_MEMPOOL_ENABLED           1      // Enable memory pool
// #define CF_MEMPOOL_USE_STATIC        1      // Use static allocation

//==============================================================================
// PLATFORM-SPECIFIC NOTES
//==============================================================================

/**
 * CFramework provides OS abstraction and middleware only.
 * For peripheral access (GPIO, UART, SPI, etc.), use your platform's HAL directly:
 *
 * STM32:  Use STM32 HAL (stm32xxxx_hal.h)
 * ESP32:  Use ESP-IDF APIs (driver/*.h)
 *
 * If using features that require platform integration (like UART log sink),
 * you must implement the required functions. See docs/PORTING_GUIDE.md
 */

#endif /* CF_USER_CONFIG_H */
