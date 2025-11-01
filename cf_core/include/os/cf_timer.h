/**
 * @file cf_timer.h
 * @brief FreeRTOS software timer wrapper
 * @version 1.0.0
 * @date 2025-10-31
 * @author CFramework Contributors
 *
 * @copyright Copyright (c) 2025 CFramework
 * Licensed under MIT License
 */

#ifndef CF_TIMER_H
#define CF_TIMER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cf_common.h"

#if CF_RTOS_ENABLED

#include "FreeRTOS.h"
#include "timers.h"

//==============================================================================
// TYPE DEFINITIONS
//==============================================================================

/**
 * @brief Timer handle (maps to FreeRTOS TimerHandle_t)
 */
typedef TimerHandle_t cf_timer_t;

/**
 * @brief Timer callback function
 */
typedef void (*cf_timer_callback_t)(cf_timer_t timer, void* arg);

/**
 * @brief Timer type
 */
typedef enum {
    CF_TIMER_ONE_SHOT = pdFALSE,    /**< Timer fires once */
    CF_TIMER_PERIODIC = pdTRUE       /**< Timer auto-reloads */
} cf_timer_type_t;

/**
 * @brief Timer configuration
 */
typedef struct {
    const char* name;                /**< Timer name (for debugging) */
    uint32_t period_ms;              /**< Timer period in milliseconds */
    cf_timer_type_t type;            /**< One-shot or periodic */
    cf_timer_callback_t callback;    /**< Callback function */
    void* argument;                  /**< Argument passed to callback */
    bool auto_start;                 /**< Start timer immediately after creation */
} cf_timer_config_t;

//==============================================================================
// PUBLIC API
//==============================================================================

/**
 * @brief Create software timer
 *
 * @param[out] handle Pointer to receive timer handle
 * @param[in] config Timer configuration
 *
 * @return CF_OK on success
 * @return CF_ERROR_NULL_POINTER if parameters are NULL
 * @return CF_ERROR_INVALID_PARAM if config is invalid
 * @return CF_ERROR_NO_MEMORY if creation failed
 *
 * @note This function is thread-safe
 */
cf_status_t cf_timer_create(cf_timer_t* handle, const cf_timer_config_t* config);

/**
 * @brief Delete timer
 *
 * @param[in] handle Timer handle
 * @param[in] timeout_ms Timeout waiting for deletion (0 = no wait)
 *
 * @return CF_OK on success
 * @return CF_ERROR_NULL_POINTER if handle is NULL
 * @return CF_ERROR_TIMEOUT if timeout occurred
 *
 * @note This function is thread-safe
 */
cf_status_t cf_timer_delete(cf_timer_t handle, uint32_t timeout_ms);

/**
 * @brief Start timer
 *
 * @param[in] handle Timer handle
 * @param[in] timeout_ms Timeout for command to be processed
 *
 * @return CF_OK on success
 * @return CF_ERROR_NULL_POINTER if handle is NULL
 * @return CF_ERROR_TIMEOUT if timeout occurred
 *
 * @note This function is thread-safe
 */
cf_status_t cf_timer_start(cf_timer_t handle, uint32_t timeout_ms);

/**
 * @brief Stop timer
 *
 * @param[in] handle Timer handle
 * @param[in] timeout_ms Timeout for command to be processed
 *
 * @return CF_OK on success
 * @return CF_ERROR_NULL_POINTER if handle is NULL
 * @return CF_ERROR_TIMEOUT if timeout occurred
 *
 * @note This function is thread-safe
 */
cf_status_t cf_timer_stop(cf_timer_t handle, uint32_t timeout_ms);

/**
 * @brief Reset timer (restart from beginning)
 *
 * @param[in] handle Timer handle
 * @param[in] timeout_ms Timeout for command to be processed
 *
 * @return CF_OK on success
 * @return CF_ERROR_NULL_POINTER if handle is NULL
 * @return CF_ERROR_TIMEOUT if timeout occurred
 *
 * @note This function is thread-safe
 */
cf_status_t cf_timer_reset(cf_timer_t handle, uint32_t timeout_ms);

/**
 * @brief Change timer period
 *
 * @param[in] handle Timer handle
 * @param[in] new_period_ms New period in milliseconds
 * @param[in] timeout_ms Timeout for command to be processed
 *
 * @return CF_OK on success
 * @return CF_ERROR_NULL_POINTER if handle is NULL
 * @return CF_ERROR_TIMEOUT if timeout occurred
 *
 * @note This function is thread-safe
 * @note Timer is restarted with new period
 */
cf_status_t cf_timer_change_period(cf_timer_t handle,
                                    uint32_t new_period_ms,
                                    uint32_t timeout_ms);

/**
 * @brief Check if timer is active
 *
 * @param[in] handle Timer handle
 *
 * @return true if active, false otherwise
 *
 * @note This function is thread-safe
 */
bool cf_timer_is_active(cf_timer_t handle);

/**
 * @brief Get timer name
 *
 * @param[in] handle Timer handle
 *
 * @return Timer name or "Unknown" if not found
 */
const char* cf_timer_get_name(cf_timer_t handle);

/**
 * @brief Get default timer configuration
 *
 * @param[out] config Configuration structure to fill
 */
void cf_timer_config_default(cf_timer_config_t* config);

#endif /* CF_RTOS_ENABLED */

#ifdef __cplusplus
}
#endif

#endif /* CF_TIMER_H */
