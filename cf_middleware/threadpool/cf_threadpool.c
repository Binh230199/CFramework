/**
 * @file cf_threadpool.c
 * @brief ThreadPool implementation
 */

#include "threadpool/cf_threadpool.h"

#if CF_THREADPOOL_ENABLED && CF_RTOS_ENABLED

#include "cf_assert.h"
#include "os/cf_mutex.h"
#include "os/cf_task.h"
#include "os/cf_queue.h"

// Include FreeRTOS queue for ISR operations
#include "FreeRTOS.h"
#include "queue.h"

#if CF_LOG_ENABLED
    #include "utils/cf_log.h"
#endif

#include <string.h>
#include <stdio.h>

//==============================================================================
// PRIVATE TYPES
//==============================================================================

/**
 * @brief Task descriptor
 */
typedef struct {
    cf_threadpool_task_func_t function;
    void* arg;
    cf_threadpool_priority_t priority;
} cf_threadpool_task_t;

/**
 * @brief ThreadPool structure
 */
typedef struct {
    bool initialized;
    cf_threadpool_state_t state;

    // Configuration
    uint32_t thread_count;
    uint32_t stack_size;

    // Worker threads
    cf_task_t* workers;

    // Task queue (separate queues for each priority)
    cf_queue_t queue_critical;
    cf_queue_t queue_high;
    cf_queue_t queue_normal;
    cf_queue_t queue_low;

    // Synchronization
    cf_mutex_t mutex;

    // Statistics
    uint32_t active_tasks;
    uint32_t total_submitted;
    uint32_t total_completed;

} cf_threadpool_t;

//==============================================================================
// PRIVATE VARIABLES
//==============================================================================

static cf_threadpool_t g_threadpool = {0};

//==============================================================================
// PRIVATE FUNCTIONS
//==============================================================================

/**
 * @brief Get queue for priority level
 */
static cf_queue_t get_queue_for_priority(cf_threadpool_priority_t priority)
{
    switch (priority) {
        case CF_THREADPOOL_PRIORITY_CRITICAL:
            return g_threadpool.queue_critical;
        case CF_THREADPOOL_PRIORITY_HIGH:
            return g_threadpool.queue_high;
        case CF_THREADPOOL_PRIORITY_LOW:
            return g_threadpool.queue_low;
        case CF_THREADPOOL_PRIORITY_NORMAL:
        default:
            return g_threadpool.queue_normal;
    }
}

/**
 * @brief Try to get next task from queues (priority order)
 */
static bool get_next_task(cf_threadpool_task_t* task)
{
    // Try critical priority first
    if (cf_queue_receive(g_threadpool.queue_critical, task, 0) == CF_OK) {
        return true;
    }

    // Try high priority
    if (cf_queue_receive(g_threadpool.queue_high, task, 0) == CF_OK) {
        return true;
    }

    // Try normal priority
    if (cf_queue_receive(g_threadpool.queue_normal, task, 0) == CF_OK) {
        return true;
    }

    // Try low priority
    if (cf_queue_receive(g_threadpool.queue_low, task, 0) == CF_OK) {
        return true;
    }

    return false;
}

/**
 * @brief Worker thread function
 */
static void worker_thread(void* arg)
{
    uint32_t worker_id = (uint32_t)(uintptr_t)arg;
    cf_threadpool_task_t task;

#if CF_LOG_ENABLED
    CF_LOG_D("ThreadPool worker %lu started", worker_id);
#endif

    while (g_threadpool.state == CF_THREADPOOL_RUNNING) {
        // Try to get task from queues (blocking with timeout)
        bool got_task = false;

        // Try critical first (non-blocking)
        if (cf_queue_receive(g_threadpool.queue_critical, &task, 0) == CF_OK) {
            got_task = true;
        }
        // Try high
        else if (cf_queue_receive(g_threadpool.queue_high, &task, 0) == CF_OK) {
            got_task = true;
        }
        // Try normal (with timeout to allow checking state)
        else if (cf_queue_receive(g_threadpool.queue_normal, &task, 100) == CF_OK) {
            got_task = true;
        }
        // Try low
        else if (cf_queue_receive(g_threadpool.queue_low, &task, 0) == CF_OK) {
            got_task = true;
        }

        if (got_task && task.function != NULL) {
            // Update active count
            cf_mutex_lock(g_threadpool.mutex, CF_WAIT_FOREVER);
            g_threadpool.active_tasks++;
            cf_mutex_unlock(g_threadpool.mutex);

            // Execute task
            task.function(task.arg);

            // Update statistics
            cf_mutex_lock(g_threadpool.mutex, CF_WAIT_FOREVER);
            g_threadpool.active_tasks--;
            g_threadpool.total_completed++;
            cf_mutex_unlock(g_threadpool.mutex);
        }
    }

#if CF_LOG_ENABLED
    CF_LOG_D("ThreadPool worker %lu stopped", worker_id);
#endif
}

/**
 * @brief Create worker threads
 */
static cf_status_t create_workers(uint32_t count, uint32_t stack_size, cf_task_priority_t priority)
{
    g_threadpool.workers = (cf_task_t*)pvPortMalloc(count * sizeof(cf_task_t));
    if (g_threadpool.workers == NULL) {
        return CF_ERROR_NO_MEMORY;
    }

    memset(g_threadpool.workers, 0, count * sizeof(cf_task_t));

    cf_task_config_t task_config;
    cf_task_config_default(&task_config);
    task_config.function = worker_thread;
    task_config.stack_size = stack_size;
    task_config.priority = priority;

    for (uint32_t i = 0; i < count; i++) {
        char name[16];
        snprintf(name, sizeof(name), "Worker%lu", i);
        task_config.name = name;
        task_config.argument = (void*)(uintptr_t)i;

        cf_status_t status = cf_task_create(&g_threadpool.workers[i], &task_config);
        if (status != CF_OK) {
            // Cleanup previously created workers
            for (uint32_t j = 0; j < i; j++) {
                cf_task_delete(g_threadpool.workers[j]);
            }
            vPortFree(g_threadpool.workers);
            g_threadpool.workers = NULL;
            return status;
        }
    }

    return CF_OK;
}

/**
 * @brief Destroy worker threads
 */
static void destroy_workers(void)
{
    if (g_threadpool.workers == NULL) {
        return;
    }

    // Set state to shutting down
    g_threadpool.state = CF_THREADPOOL_SHUTTING_DOWN;

    // Wait a bit for workers to finish current tasks
    cf_task_delay(100);

    // Delete all workers
    for (uint32_t i = 0; i < g_threadpool.thread_count; i++) {
        if (g_threadpool.workers[i] != NULL) {
            cf_task_delete(g_threadpool.workers[i]);
        }
    }

    vPortFree(g_threadpool.workers);
    g_threadpool.workers = NULL;
}

//==============================================================================
// PUBLIC API IMPLEMENTATION
//==============================================================================

cf_status_t cf_threadpool_init(void)
{
    cf_threadpool_config_t config;
    cf_threadpool_config_default(&config);
    return cf_threadpool_init_with_config(&config);
}

cf_status_t cf_threadpool_init_with_config(const cf_threadpool_config_t* config)
{
    CF_PTR_CHECK(config);

    if (g_threadpool.initialized) {
        return CF_ERROR_ALREADY_INITIALIZED;
    }

    if (config->thread_count == 0 || config->queue_size == 0 || config->stack_size == 0) {
        return CF_ERROR_INVALID_PARAM;
    }

    memset(&g_threadpool, 0, sizeof(cf_threadpool_t));

    // Create mutex
    cf_status_t status = cf_mutex_create(&g_threadpool.mutex);
    if (status != CF_OK) {
        return status;
    }

    // Create queues for each priority
    status = cf_queue_create(&g_threadpool.queue_critical, config->queue_size, sizeof(cf_threadpool_task_t));
    if (status != CF_OK) {
        goto cleanup;
    }

    status = cf_queue_create(&g_threadpool.queue_high, config->queue_size, sizeof(cf_threadpool_task_t));
    if (status != CF_OK) {
        goto cleanup;
    }

    status = cf_queue_create(&g_threadpool.queue_normal, config->queue_size * 2, sizeof(cf_threadpool_task_t));
    if (status != CF_OK) {
        goto cleanup;
    }

    status = cf_queue_create(&g_threadpool.queue_low, config->queue_size, sizeof(cf_threadpool_task_t));
    if (status != CF_OK) {
        goto cleanup;
    }

    // Save configuration
    g_threadpool.thread_count = config->thread_count;
    g_threadpool.stack_size = config->stack_size;
    g_threadpool.state = CF_THREADPOOL_RUNNING;

    // Create worker threads
    status = create_workers(config->thread_count, config->stack_size, config->thread_priority);
    if (status != CF_OK) {
        goto cleanup;
    }

    g_threadpool.initialized = true;

#if CF_LOG_ENABLED
    CF_LOG_I("ThreadPool initialized: %lu workers, queue size %lu",
             config->thread_count, config->queue_size);
#endif

    return CF_OK;

cleanup:
    if (g_threadpool.queue_critical) cf_queue_destroy(g_threadpool.queue_critical);
    if (g_threadpool.queue_high) cf_queue_destroy(g_threadpool.queue_high);
    if (g_threadpool.queue_normal) cf_queue_destroy(g_threadpool.queue_normal);
    if (g_threadpool.queue_low) cf_queue_destroy(g_threadpool.queue_low);
    if (g_threadpool.mutex) cf_mutex_destroy(g_threadpool.mutex);

    memset(&g_threadpool, 0, sizeof(cf_threadpool_t));
    return status;
}

void cf_threadpool_deinit(bool wait_for_tasks)
{
    if (!g_threadpool.initialized) {
        return;
    }

    if (wait_for_tasks) {
        // Wait for all tasks to complete (with timeout)
        cf_threadpool_wait_idle(5000);
    }

    // Destroy workers
    destroy_workers();

    // Destroy queues
    cf_queue_destroy(g_threadpool.queue_critical);
    cf_queue_destroy(g_threadpool.queue_high);
    cf_queue_destroy(g_threadpool.queue_normal);
    cf_queue_destroy(g_threadpool.queue_low);

    // Destroy mutex
    cf_mutex_destroy(g_threadpool.mutex);

    g_threadpool.initialized = false;
    g_threadpool.state = CF_THREADPOOL_STOPPED;

#if CF_LOG_ENABLED
    CF_LOG_I("ThreadPool deinitialized (completed %lu tasks)",
             g_threadpool.total_completed);
#endif
}

cf_status_t cf_threadpool_submit(cf_threadpool_task_func_t function,
                                  void* arg,
                                  cf_threadpool_priority_t priority,
                                  uint32_t timeout_ms)
{
    CF_PTR_CHECK(function);

    if (!g_threadpool.initialized) {
        return CF_ERROR_NOT_INITIALIZED;
    }

    if (g_threadpool.state != CF_THREADPOOL_RUNNING) {
        return CF_ERROR_INVALID_STATE;
    }

    // Create task descriptor
    cf_threadpool_task_t task = {
        .function = function,
        .arg = arg,
        .priority = priority
    };

    // Get queue for this priority
    cf_queue_t queue = get_queue_for_priority(priority);

    // Submit to queue
    cf_status_t status = cf_queue_send(queue, &task, timeout_ms);
    if (status != CF_OK) {
        return status;
    }

    // Update statistics
    cf_mutex_lock(g_threadpool.mutex, CF_WAIT_FOREVER);
    g_threadpool.total_submitted++;
    cf_mutex_unlock(g_threadpool.mutex);

    return CF_OK;
}

cf_status_t cf_threadpool_submit_from_isr(cf_threadpool_task_func_t function,
                                           void* arg,
                                           cf_threadpool_priority_t priority,
                                           uint32_t timeout_ms,
                                           BaseType_t* pxHigherPriorityTaskWoken)
{
    CF_PTR_CHECK(function);

    if (!g_threadpool.initialized) {
        return CF_ERROR_NOT_INITIALIZED;
    }

    if (g_threadpool.state != CF_THREADPOOL_RUNNING) {
        return CF_ERROR_INVALID_STATE;
    }

    // Timeout must be 0 in ISR
    if (timeout_ms != 0) {
        return CF_ERROR_INVALID_PARAM;
    }

    // Create task descriptor
    cf_threadpool_task_t task = {
        .function = function,
        .arg = arg,
        .priority = priority
    };

    // Get queue for this priority
    cf_queue_t queue = get_queue_for_priority(priority);

    // Access underlying FreeRTOS queue handle for ISR-safe operation
    // cf_queue_t is defined in cf_queue.c as: struct cf_queue_s { QueueHandle_t handle; }
    // We need to access the handle directly to use xQueueSendFromISR
    struct cf_queue_s {
        QueueHandle_t handle;
    };
    struct cf_queue_s* q = (struct cf_queue_s*)queue;

    // Submit to queue from ISR (non-blocking)
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    BaseType_t result = xQueueSendFromISR(q->handle, &task, &xHigherPriorityTaskWoken);
    
    if (pxHigherPriorityTaskWoken != NULL) {
        *pxHigherPriorityTaskWoken = xHigherPriorityTaskWoken;
    }

    if (result != pdTRUE) {
        return CF_ERROR_QUEUE_FULL;
    }

    // Note: Cannot update statistics in ISR (mutex not allowed)
    
    return CF_OK;
}

uint32_t cf_threadpool_get_active_count(void)
{
    if (!g_threadpool.initialized) {
        return 0;
    }

    cf_mutex_lock(g_threadpool.mutex, CF_WAIT_FOREVER);
    uint32_t count = g_threadpool.active_tasks;
    cf_mutex_unlock(g_threadpool.mutex);

    return count;
}

uint32_t cf_threadpool_get_pending_count(void)
{
    if (!g_threadpool.initialized) {
        return 0;
    }

    return cf_queue_get_count(g_threadpool.queue_critical) +
           cf_queue_get_count(g_threadpool.queue_high) +
           cf_queue_get_count(g_threadpool.queue_normal) +
           cf_queue_get_count(g_threadpool.queue_low);
}

bool cf_threadpool_is_idle(void)
{
    return cf_threadpool_get_active_count() == 0 &&
           cf_threadpool_get_pending_count() == 0;
}

cf_threadpool_state_t cf_threadpool_get_state(void)
{
    return g_threadpool.state;
}

cf_status_t cf_threadpool_wait_idle(uint32_t timeout_ms)
{
    if (!g_threadpool.initialized) {
        return CF_ERROR_NOT_INITIALIZED;
    }

    uint32_t elapsed = 0;
    uint32_t check_interval = 10; // Check every 10ms

    while (!cf_threadpool_is_idle()) {
        if (timeout_ms != CF_WAIT_FOREVER && elapsed >= timeout_ms) {
            return CF_ERROR_TIMEOUT;
        }

        cf_task_delay(check_interval);
        elapsed += check_interval;
    }

    return CF_OK;
}

void cf_threadpool_config_default(cf_threadpool_config_t* config)
{
    if (config == NULL) {
        return;
    }

    memset(config, 0, sizeof(cf_threadpool_config_t));
    config->thread_count = CF_THREADPOOL_THREAD_COUNT;
    config->queue_size = CF_THREADPOOL_QUEUE_SIZE;
    config->stack_size = CF_THREADPOOL_STACK_SIZE;
    config->thread_priority = CF_TASK_PRIORITY_NORMAL;
}

#endif /* CF_THREADPOOL_ENABLED && CF_RTOS_ENABLED */
