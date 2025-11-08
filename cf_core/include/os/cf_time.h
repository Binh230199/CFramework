/**
 * @file cf_time.h
 * @brief Time utilities for CFramework
 * @version 1.0.0
 * @date 2025-11-04
 * @author CFramework Contributors
 *
 * @copyright Copyright (c) 2025 CFramework
 * Licensed under MIT License
 *
 * @description
 * Platform-independent time utilities for tick conversion and delays.
 * Abstracts FreeRTOS/ESP-IDF tick functions for cross-platform compatibility.
 */

#ifndef CF_TIME_H
#define CF_TIME_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cf_common.h"

//==============================================================================
// PUBLIC API
//==============================================================================

/**
 * @brief Get current tick count
 *
 * @return Current system tick count
 *
 * @note Thread-safe, can be called from task context
 * @note Tick frequency is typically 1000 Hz (1ms per tick) on most platforms
 */
static inline uint32_t cf_time_get_tick_count(void)
{
#if CF_RTOS_ENABLED
    #ifdef ESP_PLATFORM
        return xTaskGetTickCount();
    #else
        extern uint32_t cf_task_get_tick_count(void);
        return cf_task_get_tick_count();
    #endif
#else
    return HAL_GetTick();
#endif
}

/**
 * @brief Get tick count from ISR context
 *
 * @return Current system tick count
 *
 * @note ISR-safe, can be called from interrupt handlers
 */
static inline uint32_t cf_time_get_tick_count_from_isr(void)
{
#if CF_RTOS_ENABLED
    #ifdef ESP_PLATFORM
        return xTaskGetTickCountFromISR();
    #else
        extern uint32_t cf_task_get_tick_count_from_isr(void);
        return cf_task_get_tick_count_from_isr();
    #endif
#else
    return HAL_GetTick();
#endif
}

/**
 * @brief Convert milliseconds to ticks
 *
 * @param[in] ms Time in milliseconds
 * @return Equivalent tick count
 *
 * @note Formula: ticks = (ms * tick_rate_hz) / 1000
 */
static inline uint32_t cf_time_ms_to_ticks(uint32_t ms)
{
#if CF_RTOS_ENABLED
    #ifdef ESP_PLATFORM
        return pdMS_TO_TICKS(ms);
    #else
        return (ms * configTICK_RATE_HZ) / 1000;
    #endif
#else
    return ms;  /* Bare-metal: assume 1ms per tick */
#endif
}

/**
 * @brief Convert ticks to milliseconds
 *
 * @param[in] ticks Tick count
 * @return Equivalent time in milliseconds
 *
 * @note Formula: ms = (ticks * 1000) / tick_rate_hz
 */
static inline uint32_t cf_time_ticks_to_ms(uint32_t ticks)
{
#if CF_RTOS_ENABLED
    #ifdef ESP_PLATFORM
        return (ticks * 1000) / configTICK_RATE_HZ;
    #else
        return (ticks * 1000) / configTICK_RATE_HZ;
    #endif
#else
    return ticks;  /* Bare-metal: assume 1ms per tick */
#endif
}

/**
 * @brief Delay for specified milliseconds
 *
 * @param[in] ms Delay time in milliseconds
 *
 * @note Thread-safe, blocks calling task
 * @note Calls cf_task_delay() internally
 */
static inline void cf_time_delay_ms(uint32_t ms)
{
#if CF_RTOS_ENABLED
    extern void cf_task_delay(uint32_t delay_ms);
    cf_task_delay(ms);
#else
    HAL_Delay(ms);
#endif
}

/**
 * @brief Get elapsed time since a previous tick count
 *
 * @param[in] start_tick Starting tick count
 * @return Elapsed ticks (handles overflow correctly)
 *
 * @note This function handles 32-bit tick counter overflow
 */
static inline uint32_t cf_time_elapsed_ticks(uint32_t start_tick)
{
    return cf_time_get_tick_count() - start_tick;
}

/**
 * @brief Get elapsed time in milliseconds since a previous tick count
 *
 * @param[in] start_tick Starting tick count
 * @return Elapsed time in milliseconds
 */
static inline uint32_t cf_time_elapsed_ms(uint32_t start_tick)
{
    return cf_time_ticks_to_ms(cf_time_elapsed_ticks(start_tick));
}

/**
 * @brief Check if timeout has occurred
 *
 * @param[in] start_tick Starting tick count
 * @param[in] timeout_ms Timeout in milliseconds
 * @return true if timeout occurred, false otherwise
 */
static inline bool cf_time_is_timeout(uint32_t start_tick, uint32_t timeout_ms)
{
    return cf_time_elapsed_ms(start_tick) >= timeout_ms;
}

#ifdef __cplusplus
}
#endif

#endif /* CF_TIME_H */
