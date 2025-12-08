/**
 * @file cf_stats.h
 * @brief System Statistics Module for Development and Debugging
 * @version 1.0.0
 * @date 2025-12-02
 * @author CFramework Contributors
 *
 * @copyright Copyright (c) 2025 CFramework
 * Licensed under MIT License
 *
 * ============================================================================
 * DEVELOPMENT ONLY MODULE
 * ============================================================================
 *
 * This module provides comprehensive system statistics for development and
 * debugging purposes. It aggregates information from all CFramework modules
 * to provide a task-manager-like view of system resources.
 *
 * IMPORTANT: This module should be disabled in production builds by setting
 * CF_STATS_ENABLED to 0 in cf_user_config.h
 */

#ifndef CF_STATS_H
#define CF_STATS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cf_common.h"
#include "cf_types.h"

//==============================================================================
// CONFIGURATION
//==============================================================================

#ifndef CF_STATS_ENABLED
    #define CF_STATS_ENABLED 1  /**< Enable statistics module */
#endif

#ifndef CF_STATS_MAX_TASKS
    #define CF_STATS_MAX_TASKS 16  /**< Maximum tasks to display in stats */
#endif

#if CF_STATS_ENABLED

//==============================================================================
// TYPE DEFINITIONS
//==============================================================================

/**
 * @brief Memory pool details structure
 */
typedef struct {
    const char* name;                      /**< Pool name */
    uint32_t block_size;                   /**< Block size in bytes */
    uint32_t block_count;                  /**< Total blocks */
    uint32_t used_blocks;                  /**< Currently used blocks */
    uint32_t utilization_percent;          /**< Utilization percentage */
    uint32_t allocation_failures;          /**< Allocation failures */
} cf_stats_pool_detail_t;

/**
 * @brief Memory statistics structure (enhanced)
 */
typedef struct {
    uint32_t total_pools;                  /**< Number of active memory pools */
    uint32_t total_memory_bytes;           /**< Total pool memory allocated */
    uint32_t global_allocations;           /**< Global allocation counter */
    uint32_t global_failures;              /**< Global failure counter */
    uint32_t fragmentation_events;         /**< Total fragmentation events */
    uint32_t heap_free_bytes;              /**< Free heap bytes (0 if no heap) */
    uint32_t heap_total_bytes;             /**< Total heap bytes (0 if no heap) */
    cf_stats_pool_detail_t pools[CF_MEMPOOL_MAX_POOLS]; /**< Details for each pool */
} cf_stats_memory_t;

/**
 * @brief Task information structure
 */
typedef struct {
    char name[16];                         /**< Task name (fixed size for safety) */
    uint32_t priority;                     /**< Task priority */
    uint32_t stack_size;                   /**< Free stack space in bytes (high water mark) */
    uint32_t stack_used;                   /**< Stack used in bytes */
    const char* state;                     /**< Task state string */
} cf_stats_task_info_t;

/**
 * @brief ThreadPool statistics structure (enhanced)
 */
typedef struct {
    uint32_t active_tasks;                 /**< Number of currently executing tasks */
    uint32_t pending_tasks;                /**< Number of tasks waiting in queue */
    uint32_t thread_count;                 /**< Number of worker threads */
    uint32_t queue_size;                   /**< Maximum queue size */
    uint32_t stack_size;                   /**< Stack size per thread */
    bool is_idle;                          /**< True if no active or pending tasks */
    const char* state_str;                 /**< Current state as string */
    cf_stats_task_info_t tasks[CF_STATS_MAX_TASKS]; /**< Task details */
} cf_stats_threadpool_t;

/**
 * @brief Queue statistics structure
 */
typedef struct {
    uint32_t queue_count;                  /**< Number of active queues */
    uint32_t total_messages;               /**< Total messages across all queues */
    uint32_t max_queue_length;             /**< Maximum queue length configured */
} cf_stats_queues_t;

/**
 * @brief Event system statistics structure
 */
typedef struct {
    uint32_t total_subscribers;            /**< Total number of event subscribers */
    uint32_t max_subscribers;              /**< Maximum allowed subscribers */
    bool is_initialized;                   /**< True if event system initialized */
} cf_stats_event_t;

/**
 * @brief System statistics structure (enhanced)
 */
typedef struct {
    cf_stats_memory_t memory;              /**< Memory statistics */
    cf_stats_threadpool_t threadpool;      /**< ThreadPool statistics */
    cf_stats_event_t event;                /**< Event system statistics */
    cf_stats_queues_t queues;              /**< Queue statistics */
    uint32_t uptime_seconds;               /**< System uptime in seconds */
    const char* framework_version;         /**< CFramework version string */
    uint32_t total_tasks;                  /**< Total number of tasks in system */
} cf_stats_system_t;

//==============================================================================
// PUBLIC API
//==============================================================================

/**
 * @brief Initialize statistics module
 *
 * @return CF_OK on success
 * @return CF_ERROR_ALREADY_INITIALIZED if already initialized
 *
 * @note Thread-safe
 */
cf_status_t cf_stats_init(void);

/**
 * @brief Deinitialize statistics module
 *
 * @note Thread-safe
 */
void cf_stats_deinit(void);

/**
 * @brief Collect comprehensive system statistics
 *
 * @param[out] stats Pointer to statistics structure to fill
 *
 * @return CF_OK on success
 * @return CF_ERROR_NULL_POINTER if stats is NULL
 * @return CF_ERROR_NOT_INITIALIZED if module not initialized
 *
 * @note Thread-safe
 * @note This function aggregates data from all enabled modules
 */
cf_status_t cf_stats_get_system(cf_stats_system_t* stats);

/**
 * @brief Print system statistics to log
 *
 * @return CF_OK on success
 * @return CF_ERROR_NOT_INITIALIZED if module not initialized
 *
 * @note Thread-safe
 * @note Requires CF_LOG_ENABLED
 */
cf_status_t cf_stats_print(void);

/**
 * @brief Check if statistics module is initialized
 *
 * @return true if initialized, false otherwise
 *
 * @note Thread-safe
 */
bool cf_stats_is_initialized(void);

#endif /* CF_STATS_ENABLED */

#ifdef __cplusplus
}
#endif

#endif /* CF_STATS_H */