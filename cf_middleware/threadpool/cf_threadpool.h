/**
 * @file cf_threadpool.h
 * @brief ThreadPool - Task dispatcher core
 * @version 1.0.0
 * @date 2025-10-30
 * @author CFramework Contributors
 *
 * @copyright Copyright (c) 2025 CFramework
 * Licensed under MIT License
 */

#ifndef CF_THREADPOOL_H
#define CF_THREADPOOL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cf_common.h"

#include "os/cf_task.h"

#if CF_THREADPOOL_ENABLED && CF_RTOS_ENABLED

//==============================================================================
// TYPE DEFINITIONS
//==============================================================================

/**
 * @brief Task function type
 */
typedef void (*cf_threadpool_task_func_t)(void* arg);

/**
 * @brief ThreadPool priority levels
 */
typedef enum {
    CF_THREADPOOL_PRIORITY_LOW,
    CF_THREADPOOL_PRIORITY_NORMAL,
    CF_THREADPOOL_PRIORITY_HIGH,
    CF_THREADPOOL_PRIORITY_CRITICAL
} cf_threadpool_priority_t;

/**
 * @brief ThreadPool configuration
 */
typedef struct {
    uint32_t thread_count;              /**< Number of worker threads */
    uint32_t queue_size;                /**< Task queue size */
    uint32_t stack_size;                /**< Stack size per thread */
    cf_task_priority_t thread_priority; /**< Worker thread priority */
} cf_threadpool_config_t;

/**
 * @brief ThreadPool state
 */
typedef enum {
    CF_THREADPOOL_STOPPED,
    CF_THREADPOOL_RUNNING,
    CF_THREADPOOL_SHUTTING_DOWN
} cf_threadpool_state_t;

//==============================================================================
// PUBLIC API
//==============================================================================

/**
 * @brief Initialize ThreadPool with default configuration
 *
 * @return CF_OK on success
 * @return CF_ERROR_ALREADY_INITIALIZED if already initialized
 * @return CF_ERROR_NO_MEMORY if creation failed
 *
 * @note This function is thread-safe
 */
cf_status_t cf_threadpool_init(void);

/**
 * @brief Initialize ThreadPool with custom configuration
 *
 * @param[in] config Custom configuration
 *
 * @return CF_OK on success
 * @return CF_ERROR_NULL_POINTER if config is NULL
 * @return CF_ERROR_INVALID_PARAM if config parameters are invalid
 * @return CF_ERROR_ALREADY_INITIALIZED if already initialized
 * @return CF_ERROR_NO_MEMORY if creation failed
 *
 * @note This function is thread-safe
 */
cf_status_t cf_threadpool_init_with_config(const cf_threadpool_config_t* config);

/**
 * @brief Deinitialize ThreadPool
 *
 * @param[in] wait_for_tasks true to wait for running tasks to complete
 *
 * @note This function is thread-safe
 */
void cf_threadpool_deinit(bool wait_for_tasks);

/**
 * @brief Submit task to ThreadPool
 *
 * @param[in] function Task function to execute
 * @param[in] arg Argument to pass to function
 * @param[in] priority Task priority (for queue ordering)
 * @param[in] timeout_ms Timeout in milliseconds (0 = no wait)
 *
 * @return CF_OK on success
 * @return CF_ERROR_NULL_POINTER if function is NULL
 * @return CF_ERROR_NOT_INITIALIZED if ThreadPool not initialized
 * @return CF_ERROR_INVALID_STATE if ThreadPool is shutting down
 * @return CF_ERROR_TIMEOUT if timeout occurred
 * @return CF_ERROR_QUEUE_FULL if queue is full
 *
 * @note This function is thread-safe
 */
cf_status_t cf_threadpool_submit(cf_threadpool_task_func_t function,
                                  void* arg,
                                  cf_threadpool_priority_t priority,
                                  uint32_t timeout_ms);

/**
 * @brief Get number of active tasks
 *
 * @return Number of active tasks (0 if not initialized)
 *
 * @note This function is thread-safe
 */
uint32_t cf_threadpool_get_active_count(void);

/**
 * @brief Get number of pending tasks in queue
 *
 * @return Number of pending tasks (0 if not initialized)
 *
 * @note This function is thread-safe
 */
uint32_t cf_threadpool_get_pending_count(void);

/**
 * @brief Check if ThreadPool is idle (no active or pending tasks)
 *
 * @return true if idle, false otherwise
 *
 * @note This function is thread-safe
 */
bool cf_threadpool_is_idle(void);

/**
 * @brief Get ThreadPool state
 *
 * @return Current state
 *
 * @note This function is thread-safe
 */
cf_threadpool_state_t cf_threadpool_get_state(void);

/**
 * @brief Wait for all tasks to complete
 *
 * @param[in] timeout_ms Timeout in milliseconds (CF_WAIT_FOREVER for infinite)
 *
 * @return CF_OK if all tasks completed
 * @return CF_ERROR_TIMEOUT if timeout occurred
 * @return CF_ERROR_NOT_INITIALIZED if not initialized
 *
 * @note This function is thread-safe
 */
cf_status_t cf_threadpool_wait_idle(uint32_t timeout_ms);

/**
 * @brief Get default ThreadPool configuration
 *
 * @param[out] config Configuration structure to fill
 */
void cf_threadpool_config_default(cf_threadpool_config_t* config);

#endif /* CF_THREADPOOL_ENABLED && CF_RTOS_ENABLED */

#ifdef __cplusplus
}
#endif

#endif /* CF_THREADPOOL_H */
