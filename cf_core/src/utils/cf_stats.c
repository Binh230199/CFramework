/**
 * @file cf_stats.c
 * @brief System Statistics Module Implementation
 * @version 1.0.0
 * @date 2025-12-02
 * @author CFramework Contributors
 *
 * @copyright Copyright (c) 2025 CFramework
 * Licensed under MIT License
 */

#include "utils/cf_stats.h"

#if CF_STATS_ENABLED

#include "cf_assert.h"
#include "cf_mutex.h"
#include "cf.h"

#if CF_MEMPOOL_ENABLED
#include "mempool/cf_mempool.h"
#endif

#if CF_THREADPOOL_ENABLED
#include "threadpool/cf_threadpool.h"
#endif

#if CF_EVENT_ENABLED
#include "event/cf_event.h"
#endif

#if CF_LOG_ENABLED
#include "utils/cf_log.h"
#endif

#include <string.h>

// Include FreeRTOS headers for advanced statistics (development only)
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

//==============================================================================
// PRIVATE DEFINITIONS
//==============================================================================

/**
 * @brief Statistics module state
 */
static struct {
    bool initialized;                   /**< Module initialization flag */
    cf_mutex_t mutex;                   /**< Mutex for thread safety */
    uint32_t start_tick;                /**< Tick count when stats module started */
} g_stats_state = {
    .initialized = false,
    .mutex = NULL,
    .start_tick = 0
};

/**
 * @brief Convert ThreadPool state to string
 */
static const char* cf_stats_threadpool_state_to_string(cf_threadpool_state_t state)
{
    switch (state) {
        case CF_THREADPOOL_STOPPED:      return "STOPPED";
        case CF_THREADPOOL_RUNNING:      return "RUNNING";
        case CF_THREADPOOL_SHUTTING_DOWN: return "SHUTTING_DOWN";
        default:                         return "UNKNOWN";
    }
}

//==============================================================================
// PUBLIC API IMPLEMENTATION
//==============================================================================

cf_status_t cf_stats_init(void)
{
    cf_status_t status = CF_OK;

    // Check if already initialized
    if (g_stats_state.initialized) {
        return CF_ERROR_ALREADY_INITIALIZED;
    }

    // Create mutex for thread safety
    status = cf_mutex_create(&g_stats_state.mutex);
    if (status != CF_OK) {
        return status;
    }

    // Mark as initialized
    g_stats_state.initialized = true;

    // Record start time for uptime calculation
    g_stats_state.start_tick = xTaskGetTickCount();

    return CF_OK;
}

void cf_stats_deinit(void)
{
    if (!g_stats_state.initialized) {
        return;
    }

    // Destroy mutex
    if (g_stats_state.mutex != NULL) {
        cf_mutex_destroy(g_stats_state.mutex);
        g_stats_state.mutex = NULL;
    }

    // Mark as deinitialized
    g_stats_state.initialized = false;
}

cf_status_t cf_stats_get_system(cf_stats_system_t* stats)
{
    cf_status_t status = CF_OK;

    // Validate parameters
    if (stats == NULL) {
        return CF_ERROR_NULL_POINTER;
    }

    // Check initialization
    if (!g_stats_state.initialized) {
        return CF_ERROR_NOT_INITIALIZED;
    }

    // Lock mutex for thread safety
    cf_mutex_lock(g_stats_state.mutex, CF_WAIT_FOREVER);

    // Clear stats structure
    memset(stats, 0, sizeof(cf_stats_system_t));

    // Set framework version
    stats->framework_version = cf_get_version();

    // Calculate uptime
    uint32_t current_tick = xTaskGetTickCount();
    uint32_t elapsed_ticks = current_tick - g_stats_state.start_tick;
    stats->uptime_seconds = elapsed_ticks / configTICK_RATE_HZ;

    // Collect memory statistics
    #if CF_MEMPOOL_ENABLED
    {
        cf_mempool_global_stats_t mempool_stats;
        if (cf_mempool_get_global_stats(&mempool_stats) == CF_OK) {
            stats->memory.total_pools = mempool_stats.total_pools;
            stats->memory.total_memory_bytes = mempool_stats.total_memory_bytes;
            stats->memory.global_allocations = mempool_stats.global_allocations;
            stats->memory.global_failures = mempool_stats.global_failures;
            stats->memory.fragmentation_events = mempool_stats.fragmentation_events;
        }

        // Get detailed pool information using enumeration
        uint32_t pool_count = 0;
        cf_status_t enum_status = cf_mempool_enumerate_pools(NULL, &pool_count);

        if (enum_status == CF_OK && pool_count > 0) {
            cf_mempool_handle_t* pool_handles = (cf_mempool_handle_t*)pvPortMalloc(pool_count * sizeof(cf_mempool_handle_t));
            if (pool_handles != NULL) {
                enum_status = cf_mempool_enumerate_pools(pool_handles, &pool_count);
                if (enum_status == CF_OK) {
                    for (uint32_t i = 0; i < pool_count && i < CF_MEMPOOL_MAX_POOLS; i++) {
                        cf_mempool_handle_t pool = pool_handles[i];

                        // Get pool configuration
                        cf_mempool_config_t config;
                        if (cf_mempool_get_info(pool, &config) == CF_OK) {
                            stats->memory.pools[i].name = config.name;
                            stats->memory.pools[i].block_size = config.block_size;
                            stats->memory.pools[i].block_count = config.block_count;
                        } else {
                            stats->memory.pools[i].name = "Unknown";
                            stats->memory.pools[i].block_size = 0;
                            stats->memory.pools[i].block_count = 0;
                        }

                        // Get pool statistics
                        cf_mempool_stats_t pool_stats;
                        if (cf_mempool_get_stats(pool, &pool_stats) == CF_OK) {
                            stats->memory.pools[i].used_blocks = pool_stats.current_used;
                            stats->memory.pools[i].utilization_percent = pool_stats.utilization_percent;
                            stats->memory.pools[i].allocation_failures = pool_stats.allocation_failures;
                        } else {
                            stats->memory.pools[i].used_blocks = 0;
                            stats->memory.pools[i].utilization_percent = 0;
                            stats->memory.pools[i].allocation_failures = 0;
                        }
                    }
                }
                vPortFree(pool_handles);
            }
        }

        // Heap stats - since we don't use heap, set to 0
        stats->memory.heap_free_bytes = 0;
        stats->memory.heap_total_bytes = 0;
    }
    #else
    {
        // No memory pool system
        stats->memory.total_pools = 0;
        stats->memory.total_memory_bytes = 0;
        stats->memory.global_allocations = 0;
        stats->memory.global_failures = 0;
        stats->memory.fragmentation_events = 0;
        stats->memory.heap_free_bytes = 0;
        stats->memory.heap_total_bytes = 0;
    }
    #endif

    // Collect ThreadPool statistics
    #if CF_THREADPOOL_ENABLED
    {
        stats->threadpool.active_tasks = cf_threadpool_get_active_count();
        stats->threadpool.pending_tasks = cf_threadpool_get_pending_count();
        stats->threadpool.is_idle = cf_threadpool_is_idle();
        stats->threadpool.state_str = cf_stats_threadpool_state_to_string(cf_threadpool_get_state());

        // Get configuration (if available)
        cf_threadpool_config_t config;
        cf_threadpool_config_default(&config);
        stats->threadpool.thread_count = config.thread_count;
        stats->threadpool.queue_size = config.queue_size;
        // Note: Actual stack size is set in freertos.cpp (4096 * 2 = 8192)
        stats->threadpool.stack_size = 4096 * 2;

        // Get task information using FreeRTOS APIs (development only)
        // This is acceptable for development tools
        uint32_t task_count = 0;
        TaskStatus_t* task_status_array = NULL;
        UBaseType_t task_array_size = uxTaskGetNumberOfTasks();

        if (task_array_size > 0) {
            task_status_array = (TaskStatus_t*)pvPortMalloc(task_array_size * sizeof(TaskStatus_t));
            if (task_status_array != NULL) {
                UBaseType_t actual_task_count = uxTaskGetSystemState(task_status_array, task_array_size, NULL);

                // Collect all tasks (not just ThreadPool tasks)
                for (UBaseType_t i = 0; i < actual_task_count && task_count < CF_STATS_MAX_TASKS; i++) {
                    // Safely copy task name (limit to 15 chars + null terminator)
                    if (task_status_array[i].pcTaskName != NULL &&
                        task_status_array[i].pcTaskName[0] != '\0' &&
                        strlen(task_status_array[i].pcTaskName) > 0) {
                        strncpy(stats->threadpool.tasks[task_count].name,
                               task_status_array[i].pcTaskName,
                               sizeof(stats->threadpool.tasks[task_count].name) - 1);
                        stats->threadpool.tasks[task_count].name[sizeof(stats->threadpool.tasks[task_count].name) - 1] = '\0';

                        // Additional safety check - ensure no control characters
                        for (size_t j = 0; j < sizeof(stats->threadpool.tasks[task_count].name); j++) {
                            if (stats->threadpool.tasks[task_count].name[j] == '\0') break;
                            if (stats->threadpool.tasks[task_count].name[j] < 32 ||
                                stats->threadpool.tasks[task_count].name[j] > 126) {
                                strcpy(stats->threadpool.tasks[task_count].name, "Invalid");
                                break;
                            }
                        }
                    } else {
                        strcpy(stats->threadpool.tasks[task_count].name, "Unknown");
                    }

                    // Validate priority (avoid unreasonable values)
                    uint32_t priority = task_status_array[i].uxCurrentPriority;
                    if (priority <= 10) {  // Reasonable priority range
                        stats->threadpool.tasks[task_count].priority = priority;
                    } else {
                        stats->threadpool.tasks[task_count].priority = 0;
                    }

                    // Validate free stack space (high water mark, avoid unreasonable values)
                    uint32_t free_stack = task_status_array[i].usStackHighWaterMark * 4;
                    if (free_stack > 0 && free_stack < 100000) {  // Reasonable range
                        stats->threadpool.tasks[task_count].stack_size = free_stack;
                    } else {
                        stats->threadpool.tasks[task_count].stack_size = 0;
                    }

                    stats->threadpool.tasks[task_count].stack_used = 0; // Not easily available from FreeRTOS

                    // Convert task state to string
                    switch (task_status_array[i].eCurrentState) {
                        case eRunning: stats->threadpool.tasks[task_count].state = "RUNNING"; break;
                        case eReady: stats->threadpool.tasks[task_count].state = "READY"; break;
                        case eBlocked: stats->threadpool.tasks[task_count].state = "BLOCKED"; break;
                        case eSuspended: stats->threadpool.tasks[task_count].state = "SUSPENDED"; break;
                        case eDeleted: stats->threadpool.tasks[task_count].state = "DELETED"; break;
                        default: stats->threadpool.tasks[task_count].state = "UNKNOWN"; break;
                    }
                    task_count++;
                }

                vPortFree(task_status_array);
            }
        }

        // Fill remaining task slots with empty data
        for (uint32_t i = task_count; i < CF_STATS_MAX_TASKS; i++) {
            strcpy(stats->threadpool.tasks[i].name, "N/A");
            stats->threadpool.tasks[i].priority = 0;
            stats->threadpool.tasks[i].stack_size = 0;
            stats->threadpool.tasks[i].stack_used = 0;
            stats->threadpool.tasks[i].state = "N/A";
        }

        // Set total tasks to the number we actually collected (limited by array size)
        stats->total_tasks = task_count;
    }
    #else
    {
        stats->threadpool.active_tasks = 0;
        stats->threadpool.pending_tasks = 0;
        stats->threadpool.thread_count = 0;
        stats->threadpool.queue_size = 0;
        stats->threadpool.is_idle = true;
        stats->threadpool.state_str = "DISABLED";

        // Clear task information
        for (uint32_t i = 0; i < CF_STATS_MAX_TASKS; i++) {
            strcpy(stats->threadpool.tasks[i].name, "N/A");
            stats->threadpool.tasks[i].priority = 0;
            stats->threadpool.tasks[i].stack_size = 0;
            stats->threadpool.tasks[i].stack_used = 0;
            stats->threadpool.tasks[i].state = "N/A";
        }

        stats->total_tasks = uxTaskGetNumberOfTasks();
    }
    #endif

    // Collect event system statistics
    #if CF_EVENT_ENABLED
    {
        stats->event.total_subscribers = cf_event_get_subscriber_count();
        stats->event.max_subscribers = CF_EVENT_MAX_SUBSCRIBERS;
        stats->event.is_initialized = cf_event_is_initialized();
    }
    #else
    {
        stats->event.total_subscribers = 0;
        stats->event.max_subscribers = 0;
        stats->event.is_initialized = false;
    }
    #endif

    // Collect queue statistics
    // Note: This is simplified since we don't have direct access to all queues
    // In a real system, we might need to track queues globally
    stats->queues.queue_count = 0;  // TODO: Implement queue enumeration
    stats->queues.total_messages = 0;
    stats->queues.max_queue_length = 0;

    // Get total task count (already set above)
    // stats->total_tasks is already set in threadpool section
    stats->total_tasks = uxTaskGetNumberOfTasks();

    // Unlock mutex
    cf_mutex_unlock(g_stats_state.mutex);

    return status;
}

cf_status_t cf_stats_print(void)
{
    cf_status_t status = CF_OK;
    cf_stats_system_t stats;

    // Check initialization
    if (!g_stats_state.initialized) {
        return CF_ERROR_NOT_INITIALIZED;
    }

    #if CF_LOG_ENABLED
    // Get statistics
    status = cf_stats_get_system(&stats);
    if (status != CF_OK) {
        return status;
    }

    // Print header
    CF_LOG_I("=== CFramework System Statistics ===");
    CF_LOG_I("Framework Version: %s", stats.framework_version);
    CF_LOG_I("Uptime: %u seconds", stats.uptime_seconds);
    CF_LOG_I("Total Tasks: %u", stats.total_tasks);

    // Print memory statistics
    CF_LOG_I("");
    CF_LOG_I("--- Memory Statistics ---");
    CF_LOG_I("Memory Pools: %u", stats.memory.total_pools);
    CF_LOG_I("Total Pool Memory: %u bytes", stats.memory.total_memory_bytes);
    CF_LOG_I("Global Allocations: %u", stats.memory.global_allocations);
    CF_LOG_I("Global Failures: %u", stats.memory.global_failures);
    CF_LOG_I("Fragmentation Events: %u", stats.memory.fragmentation_events);
    CF_LOG_I("Heap Free: %u bytes", stats.memory.heap_free_bytes);
    CF_LOG_I("Heap Total: %u bytes", stats.memory.heap_total_bytes);

    // Print detailed pool information
    if (stats.memory.total_pools > 0) {
        CF_LOG_I("Pool Details:");
        for (uint32_t i = 0; i < stats.memory.total_pools && i < CF_MEMPOOL_MAX_POOLS; i++) {
            if (stats.memory.pools[i].name != NULL && strlen(stats.memory.pools[i].name) > 0) {
                CF_LOG_I("  %-12s: %u/%u blocks (%u%%), %u failures",
                        stats.memory.pools[i].name,
                        stats.memory.pools[i].used_blocks,
                        stats.memory.pools[i].block_count,
                        stats.memory.pools[i].utilization_percent,
                        stats.memory.pools[i].allocation_failures);
            }
        }
    }

    // Print ThreadPool statistics
    CF_LOG_I("");
    CF_LOG_I("--- ThreadPool Statistics ---");
    CF_LOG_I("Active Tasks: %u", stats.threadpool.active_tasks);
    CF_LOG_I("Pending Tasks: %u", stats.threadpool.pending_tasks);
    CF_LOG_I("Worker Threads: %u", stats.threadpool.thread_count);
    CF_LOG_I("Queue Size: %u", stats.threadpool.queue_size);
    CF_LOG_I("Is Idle: %s", stats.threadpool.is_idle ? "YES" : "NO");
    CF_LOG_I("State: %s", stats.threadpool.state_str);

    // Print all task details (limit to meaningful tasks)
    uint32_t displayed_tasks = 0;
    if (stats.total_tasks > 0) {
        CF_LOG_I("All Tasks:");
        for (uint32_t i = 0; i < stats.total_tasks && displayed_tasks < 8; i++) {  // Show max 8 tasks
            if (stats.threadpool.tasks[i].name[0] != '\0' &&
                strcmp(stats.threadpool.tasks[i].name, "N/A") != 0) {
                CF_LOG_I("  %-12s: Prio %u, Stack %u bytes, %s",
                        stats.threadpool.tasks[i].name,
                        stats.threadpool.tasks[i].priority,
                        stats.threadpool.tasks[i].stack_size,
                        stats.threadpool.tasks[i].state);
                displayed_tasks++;
            }
        }
        if (stats.total_tasks > displayed_tasks) {
            CF_LOG_I("  ... and %u more tasks", stats.total_tasks - displayed_tasks);
        }
    }

    // Print event system statistics
    CF_LOG_I("");
    CF_LOG_I("--- Event System Statistics ---");
    CF_LOG_I("Total Subscribers: %u", stats.event.total_subscribers);
    CF_LOG_I("Max Subscribers: %u", stats.event.max_subscribers);
    CF_LOG_I("Is Initialized: %s", stats.event.is_initialized ? "YES" : "NO");

    // Print queue statistics
    CF_LOG_I("");
    CF_LOG_I("--- Queue Statistics ---");
    CF_LOG_I("Active Queues: %u", stats.queues.queue_count);
    CF_LOG_I("Total Messages: %u", stats.queues.total_messages);
    CF_LOG_I("Max Queue Length: %u", stats.queues.max_queue_length);

    // Print system summary
    CF_LOG_I("");
    CF_LOG_I("--- System Summary ---");
    CF_LOG_I("Total Tasks: %u", stats.total_tasks);

    CF_LOG_I("=== End Statistics ===");

    #else
    // Logging disabled, return error
    status = CF_ERROR_NOT_SUPPORTED;
    #endif

    return status;
}

bool cf_stats_is_initialized(void)
{
    return g_stats_state.initialized;
}

#endif /* CF_STATS_ENABLED */