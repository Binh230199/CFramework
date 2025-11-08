/**
 * @file cf_critical.h
 * @brief Critical section management for CFramework
 * @version 1.0.0
 * @date 2025-11-04
 * @author CFramework Contributors
 *
 * @copyright Copyright (c) 2025 CFramework
 * Licensed under MIT License
 *
 * @description
 * Platform-independent critical section API.
 * Abstracts FreeRTOS/ESP-IDF critical section functions.
 */

#ifndef CF_CRITICAL_H
#define CF_CRITICAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cf_common.h"

#if CF_RTOS_ENABLED
    #ifdef ESP_PLATFORM
        #include "freertos/FreeRTOS.h"
        #include "freertos/task.h"
    #else
        #include "FreeRTOS.h"
        #include "task.h"
    #endif
#endif

//==============================================================================
// ESP32 SPINLOCK (required for ESP-IDF critical sections)
//==============================================================================

#ifdef ESP_PLATFORM
static portMUX_TYPE cf_critical_mux = portMUX_INITIALIZER_UNLOCKED;
#endif

//==============================================================================
// PUBLIC API
//==============================================================================

/**
 * @brief Enter critical section (disable interrupts)
 *
 * @note Thread-safe, can be nested
 * @note Must be paired with cf_critical_section_exit()
 * @note On FreeRTOS: calls taskENTER_CRITICAL()
 * @note On ESP32: calls portENTER_CRITICAL()
 * @note On bare-metal: calls __disable_irq()
 *
 * @warning Do NOT call blocking functions inside critical section
 * @warning Keep critical section as short as possible
 *
 * Example:
 * ```c
 * cf_critical_section_enter();
 * shared_variable++;  // Atomic operation
 * cf_critical_section_exit();
 * ```
 */
static inline void cf_critical_section_enter(void)
{
#if CF_RTOS_ENABLED
    #ifdef ESP_PLATFORM
        portENTER_CRITICAL(&cf_critical_mux);  /* ESP32 requires spinlock */
    #else
        taskENTER_CRITICAL();
    #endif
#else
    __disable_irq();
#endif
}

/**
 * @brief Exit critical section (re-enable interrupts)
 *
 * @note Must be paired with cf_critical_section_enter()
 * @note On FreeRTOS: calls taskEXIT_CRITICAL()
 * @note On ESP32: calls portEXIT_CRITICAL()
 * @note On bare-metal: calls __enable_irq()
 */
static inline void cf_critical_section_exit(void)
{
#if CF_RTOS_ENABLED
    #ifdef ESP_PLATFORM
        portEXIT_CRITICAL(&cf_critical_mux);
    #else
        taskEXIT_CRITICAL();
    #endif
#else
    __enable_irq();
#endif
}

/**
 * @brief Enter critical section from ISR context
 *
 * @note ISR-safe, can be called from interrupt handlers
 * @note Must be paired with cf_critical_section_exit_from_isr()
 *
 * @warning Some RTOS implementations don't support nested ISR critical sections
 */
static inline void cf_critical_section_enter_from_isr(void)
{
#if CF_RTOS_ENABLED
    #ifdef ESP_PLATFORM
        portENTER_CRITICAL_ISR(&cf_critical_mux);
    #else
        /* FreeRTOS doesn't have separate ISR critical section API */
        taskENTER_CRITICAL();
    #endif
#else
    __disable_irq();
#endif
}

/**
 * @brief Exit critical section from ISR context
 *
 * @note Must be paired with cf_critical_section_enter_from_isr()
 */
static inline void cf_critical_section_exit_from_isr(void)
{
#if CF_RTOS_ENABLED
    #ifdef ESP_PLATFORM
        portEXIT_CRITICAL_ISR(&cf_critical_mux);
    #else
        taskEXIT_CRITICAL();
    #endif
#else
    __enable_irq();
#endif
}

/**
 * @brief RAII-style critical section guard (C++ only)
 *
 * Example:
 * ```cpp
 * {
 *     cf_critical_guard_t guard;
 *     shared_variable++;  // Protected
 * }  // Automatically exits critical section
 * ```
 */
#ifdef __cplusplus
struct cf_critical_guard_t {
    cf_critical_guard_t() { cf_critical_section_enter(); }
    ~cf_critical_guard_t() { cf_critical_section_exit(); }

    // Prevent copy/move
    cf_critical_guard_t(const cf_critical_guard_t&) = delete;
    cf_critical_guard_t& operator=(const cf_critical_guard_t&) = delete;
};
#endif

#ifdef __cplusplus
}
#endif

#endif /* CF_CRITICAL_H */
