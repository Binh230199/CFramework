/**
 * @file cf_task.h
 * @brief Task wrapper for FreeRTOS
 * @version 1.0.0
 * @date 2025-10-29
 * @author CFramework Contributors
 *
 * @copyright Copyright (c) 2025 CFramework
 * Licensed under MIT License
 */

#ifndef CF_TASK_H
#define CF_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cf_common.h"

#if CF_RTOS_ENABLED

//==============================================================================
// TYPE DEFINITIONS
//==============================================================================

/**
 * @brief Opaque task handle
 */
typedef struct cf_task_s* cf_task_t;

/**
 * @brief Task function type
 */
typedef void (*cf_task_func_t)(void* argument);

/**
 * @brief Task priority levels
 */
typedef enum {
    CF_TASK_PRIORITY_IDLE,
    CF_TASK_PRIORITY_LOW,
    CF_TASK_PRIORITY_BELOW_NORMAL,
    CF_TASK_PRIORITY_NORMAL,
    CF_TASK_PRIORITY_ABOVE_NORMAL,
    CF_TASK_PRIORITY_HIGH,
    CF_TASK_PRIORITY_REALTIME
} cf_task_priority_t;

/**
 * @brief Task configuration
 */
typedef struct {
    const char* name;           /**< Task name (for debugging) */
    cf_task_func_t function;    /**< Task function */
    void* argument;             /**< Argument passed to function */
    uint32_t stack_size;        /**< Stack size in bytes */
    cf_task_priority_t priority; /**< Task priority */
} cf_task_config_t;

//==============================================================================
// PUBLIC API
//==============================================================================

/**
 * @brief Create and start a task
 *
 * @param[out] task Pointer to receive task handle
 * @param[in] config Task configuration
 *
 * @return CF_OK on success
 * @return CF_ERROR_NULL_POINTER if task or config is NULL
 * @return CF_ERROR_INVALID_PARAM if config parameters are invalid
 * @return CF_ERROR_NO_MEMORY if creation failed
 *
 * @note This function is thread-safe
 */
cf_status_t cf_task_create(cf_task_t* task, const cf_task_config_t* config);

/**
 * @brief Delete a task
 *
 * @param[in] task Task handle (NULL for current task)
 *
 * @note This function is thread-safe
 * @warning If task is NULL, the calling task will be deleted
 */
void cf_task_delete(cf_task_t task);

/**
 * @brief Delay current task
 *
 * @param[in] delay_ms Delay in milliseconds
 *
 * @note This function is thread-safe
 */
void cf_task_delay(uint32_t delay_ms);

/**
 * @brief Get current task handle
 *
 * @return Current task handle
 *
 * @note This function is thread-safe
 */
cf_task_t cf_task_get_current(void);

/**
 * @brief Get task name
 *
 * @param[in] task Task handle (NULL for current task)
 *
 * @return Task name string (never NULL)
 *
 * @note This function is thread-safe
 */
const char* cf_task_get_name(cf_task_t task);

/**
 * @brief Start the RTOS scheduler
 *
 * @note This function does NOT return
 */
void cf_task_start_scheduler(void);

/**
 * @brief Initialize default task configuration
 *
 * @param[out] config Configuration structure to initialize
 */
void cf_task_config_default(cf_task_config_t* config);

#endif /* CF_RTOS_ENABLED */

#ifdef __cplusplus
}
#endif

#endif /* CF_TASK_H */
