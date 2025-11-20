/**
 * @file cf_event.c
 * @brief Event System implementation
 */

#include "event/cf_event.h"

#if CF_EVENT_ENABLED && CF_RTOS_ENABLED

#include "cf_assert.h"
#include "os/cf_mutex.h"
#include "threadpool/cf_threadpool.h"

#if CF_MEMPOOL_ENABLED
    #include "mempool/cf_mempool.h"
#endif

#if CF_LOG_ENABLED
    #include "utils/cf_log.h"
#endif

#ifdef ESP_PLATFORM
    #include "freertos/FreeRTOS.h"
#else
    #include "FreeRTOS.h"
#endif
#include <string.h>

//==============================================================================
// PRIVATE TYPES
//==============================================================================

/**
 * @brief Subscriber structure
 */
typedef struct cf_event_subscriber_s {
    bool active;
    cf_event_id_t event_id;          /**< Event to listen to (0 = all) */
    cf_event_callback_t callback;
    void* user_data;
    cf_event_mode_t mode;
} cf_event_subscriber_s;

/**
 * @brief Event dispatch context (for async delivery)
 */
typedef struct {
    cf_event_id_t event_id;
    cf_event_callback_t callback;
    void* user_data;
    void* data;
    size_t data_size;
} cf_event_dispatch_ctx_t;

/**
 * @brief Event system structure
 */
typedef struct {
    bool initialized;
    cf_mutex_t mutex;
    cf_event_subscriber_s subscribers[CF_EVENT_MAX_SUBSCRIBERS];
    uint32_t subscriber_count;
    uint32_t total_published;
} cf_event_system_t;

//==============================================================================
// PRIVATE VARIABLES
//==============================================================================

static cf_event_system_t g_event_system = {0};

#if CF_MEMPOOL_ENABLED
// Event system memory pools
static cf_mempool_handle_t g_event_ctx_pool = NULL;     // For dispatch contexts
static cf_mempool_handle_t g_event_data_pools[5] = {0}; // For different data sizes
static bool g_event_pools_initialized = false;
#endif

//==============================================================================
// PRIVATE FUNCTIONS
//==============================================================================

/**
 * @brief Find free subscriber slot
 */
static int32_t find_free_subscriber_slot(void)
{
    for (uint32_t i = 0; i < CF_EVENT_MAX_SUBSCRIBERS; i++) {
        if (!g_event_system.subscribers[i].active) {
            return (int32_t)i;
        }
    }
    return -1;
}

#if CF_MEMPOOL_ENABLED
/**
 * @brief Initialize event system memory pools
 * @note Called during cf_event_init() - safe to fail
 */
static cf_status_t init_event_pools(void)
{
    if (g_event_pools_initialized) {
        return CF_OK;
    }

    // Initialize mempool system if not done
    cf_status_t status = cf_mempool_init();
    if (status != CF_OK && status != CF_ERROR) {
        // CF_ERROR means already initialized - that's OK
        return status;
    }

    // Create context pool (64B blocks for dispatch contexts)
    cf_mempool_config_t ctx_config = {
        .block_size = 64,  // sizeof(cf_event_dispatch_ctx_t) + alignment
        .block_count = 30, // Support 30 concurrent async events
        .name = "event_ctx"
    };

    status = cf_mempool_create(&g_event_ctx_pool, &ctx_config);
    if (status != CF_OK) {
#if CF_LOG_ENABLED
        CF_LOG_W("Failed to create event context pool, using heap fallback");
#endif
        return status; // Non-fatal - will use heap fallback
    }

    // Create data pools for different sizes
    cf_mempool_config_t data_configs[] = {
        { .block_size = 64,   .block_count = 20, .name = "event_64" },
        { .block_size = 128,  .block_count = 15, .name = "event_128" },
        { .block_size = 256,  .block_count = 10, .name = "event_256" },
        { .block_size = 512,  .block_count = 5,  .name = "event_512" },
        { .block_size = 1024, .block_count = 2,  .name = "event_1k" }
    };

    for (int i = 0; i < 5; i++) {
        status = cf_mempool_create(&g_event_data_pools[i], &data_configs[i]);
        if (status != CF_OK) {
#if CF_LOG_ENABLED
            CF_LOG_W("Failed to create event data pool %d, using heap fallback", i);
#endif
            // Continue - non-fatal
        }
    }

    g_event_pools_initialized = true;

#if CF_LOG_ENABLED
    CF_LOG_I("Event system memory pools initialized");
#endif

    return CF_OK;
}

/**
 * @brief Smart allocation for event data with heap fallback
 * @param size Requested size
 * @return Allocated pointer or NULL
 */
static void* event_smart_alloc(size_t size)
{
    void* ptr = NULL;

    if (g_event_pools_initialized) {
        // Try mempool allocation first
        ptr = cf_mempool_alloc(size);
        if (ptr) {
            return ptr;
        }
        // Pool allocation failed - continue to heap fallback
    }

    // Heap fallback (original behavior)
    ptr = pvPortMalloc(size);

#if CF_LOG_ENABLED
    if (ptr && g_event_pools_initialized) {
        CF_LOG_D("Event alloc fallback to heap: size=%zu", size);
    }
#endif

    return ptr;
}

/**
 * @brief Smart deallocation for event data
 * @param ptr Pointer to free
 * @note Safe to call with NULL pointer
 */
static void event_smart_free(void* ptr)
{
    if (!ptr) {
        return;
    }

    if (g_event_pools_initialized) {
        // Try mempool free first
        if (cf_mempool_free(ptr) == CF_OK) {
            return; // Successfully freed from pool
        }
        // Not a pool pointer - continue to heap free
    }

    // Heap free (original behavior)
    vPortFree(ptr);
}
#endif /* CF_MEMPOOL_ENABLED */

/**
 * @brief Async event dispatch task
 */
static void event_dispatch_task(void* arg)
{
    cf_event_dispatch_ctx_t* ctx = (cf_event_dispatch_ctx_t*)arg;

    if (ctx == NULL) {
        return;
    }

    // Invoke callback
    if (ctx->callback != NULL) {
        ctx->callback(ctx->event_id, ctx->data, ctx->data_size, ctx->user_data);
    }

    // Free data if allocated
    if (ctx->data != NULL) {
#if CF_MEMPOOL_ENABLED
        event_smart_free(ctx->data);
#else
        vPortFree(ctx->data);
#endif
    }

    // Free context
#if CF_MEMPOOL_ENABLED
    event_smart_free(ctx);
#else
    vPortFree(ctx);
#endif
}

/**
 * @brief Deliver event to single subscriber
 */
static void deliver_to_subscriber(const cf_event_subscriber_s* sub,
                                   cf_event_id_t event_id,
                                   const void* data,
                                   size_t data_size)
{
    if (sub->mode == CF_EVENT_SYNC) {
        // Synchronous - call immediately
        sub->callback(event_id, data, data_size, sub->user_data);
    } else {
        // Asynchronous - dispatch to ThreadPool
#if CF_MEMPOOL_ENABLED
        cf_event_dispatch_ctx_t* ctx = (cf_event_dispatch_ctx_t*)event_smart_alloc(sizeof(cf_event_dispatch_ctx_t));
#else
        cf_event_dispatch_ctx_t* ctx = (cf_event_dispatch_ctx_t*)pvPortMalloc(sizeof(cf_event_dispatch_ctx_t));
#endif
        if (ctx == NULL) {
#if CF_LOG_ENABLED
            CF_LOG_E("Failed to allocate dispatch context");
#endif
            return;
        }

        ctx->event_id = event_id;
        ctx->callback = sub->callback;
        ctx->user_data = sub->user_data;
        ctx->data_size = data_size;

        // Copy data if present
        if (data != NULL && data_size > 0) {
#if CF_MEMPOOL_ENABLED
            ctx->data = event_smart_alloc(data_size);
#else
            ctx->data = pvPortMalloc(data_size);
#endif
            if (ctx->data == NULL) {
#if CF_LOG_ENABLED
                CF_LOG_E("Failed to allocate event data");
#endif
#if CF_MEMPOOL_ENABLED
                event_smart_free(ctx);
#else
                vPortFree(ctx);
#endif
                return;
            }
            memcpy(ctx->data, data, data_size);
        } else {
            ctx->data = NULL;
        }

        // Submit to ThreadPool
        cf_status_t status = cf_threadpool_submit(event_dispatch_task, ctx,
                                                  CF_THREADPOOL_PRIORITY_NORMAL,
                                                  100);
        if (status != CF_OK) {
#if CF_LOG_ENABLED
            CF_LOG_E("Failed to submit async event: %d", status);
#endif
#if CF_MEMPOOL_ENABLED
            if (ctx->data) event_smart_free(ctx->data);
            event_smart_free(ctx);
#else
            if (ctx->data) vPortFree(ctx->data);
            vPortFree(ctx);
#endif
        }
    }
}

//==============================================================================
// PUBLIC API IMPLEMENTATION
//==============================================================================

cf_status_t cf_event_init(void)
{
    if (g_event_system.initialized) {
        return CF_ERROR_ALREADY_INITIALIZED;
    }

    // Create global mutex
    cf_status_t status = cf_mutex_create(&g_event_system.mutex);
    if (status != CF_OK) {
        return status;
    }

    // Clear subscriber array
    memset(g_event_system.subscribers, 0, sizeof(g_event_system.subscribers));
    g_event_system.subscriber_count = 0;
    g_event_system.total_published = 0;

#if CF_MEMPOOL_ENABLED
    // Initialize event system memory pools (non-fatal if fails)
    init_event_pools();
#endif

    g_event_system.initialized = true;

#if CF_LOG_ENABLED
    CF_LOG_I("Event system initialized");
#endif

    return CF_OK;
}

void cf_event_deinit(void)
{
    if (!g_event_system.initialized) {
        return;
    }

    cf_mutex_lock(g_event_system.mutex, CF_WAIT_FOREVER);

    // Clear all subscribers
    memset(g_event_system.subscribers, 0, sizeof(g_event_system.subscribers));
    g_event_system.subscriber_count = 0;

    cf_mutex_unlock(g_event_system.mutex);

    // Destroy mutex
    cf_mutex_destroy(g_event_system.mutex);

    g_event_system.initialized = false;

#if CF_LOG_ENABLED
    CF_LOG_I("Event system deinitialized (published %lu events)",
             g_event_system.total_published);
#endif
}

cf_status_t cf_event_subscribe(cf_event_id_t event_id,
                                cf_event_callback_t callback,
                                void* user_data,
                                cf_event_mode_t mode,
                                cf_event_subscriber_t* handle)
{
    CF_PTR_CHECK(callback);

    if (!g_event_system.initialized) {
        return CF_ERROR_NOT_INITIALIZED;
    }

    cf_mutex_lock(g_event_system.mutex, CF_WAIT_FOREVER);

    // Find free slot
    int32_t slot = find_free_subscriber_slot();
    if (slot < 0) {
        cf_mutex_unlock(g_event_system.mutex);
        return CF_ERROR_NO_MEMORY;
    }

    // Register subscriber
    g_event_system.subscribers[slot].active = true;
    g_event_system.subscribers[slot].event_id = event_id;
    g_event_system.subscribers[slot].callback = callback;
    g_event_system.subscribers[slot].user_data = user_data;
    g_event_system.subscribers[slot].mode = mode;

    g_event_system.subscriber_count++;

    // Return handle if requested
    if (handle) {
        *handle = (cf_event_subscriber_t)&g_event_system.subscribers[slot];
    }

    cf_mutex_unlock(g_event_system.mutex);

#if CF_LOG_ENABLED
    CF_LOG_D("Subscribed to event 0x%08lX (mode: %s)",
             event_id, mode == CF_EVENT_SYNC ? "SYNC" : "ASYNC");
#endif

    return CF_OK;
}

cf_status_t cf_event_unsubscribe(cf_event_subscriber_t handle)
{
    CF_PTR_CHECK(handle);

    if (!g_event_system.initialized) {
        return CF_ERROR_NOT_INITIALIZED;
    }

    cf_mutex_lock(g_event_system.mutex, CF_WAIT_FOREVER);

    // Validate handle is within our array
    cf_event_subscriber_s* sub = (cf_event_subscriber_s*)handle;
    if (sub < g_event_system.subscribers ||
        sub >= &g_event_system.subscribers[CF_EVENT_MAX_SUBSCRIBERS]) {
        cf_mutex_unlock(g_event_system.mutex);
        return CF_ERROR_INVALID_PARAM;
    }

    if (!sub->active) {
        cf_mutex_unlock(g_event_system.mutex);
        return CF_ERROR_NOT_FOUND;
    }

    // Deactivate subscriber
    sub->active = false;
    g_event_system.subscriber_count--;

    cf_mutex_unlock(g_event_system.mutex);

#if CF_LOG_ENABLED
    CF_LOG_D("Unsubscribed from event 0x%08lX", sub->event_id);
#endif

    return CF_OK;
}

uint32_t cf_event_unsubscribe_all(cf_event_id_t event_id)
{
    if (!g_event_system.initialized) {
        return 0;
    }

    uint32_t count = 0;

    cf_mutex_lock(g_event_system.mutex, CF_WAIT_FOREVER);

    for (uint32_t i = 0; i < CF_EVENT_MAX_SUBSCRIBERS; i++) {
        if (g_event_system.subscribers[i].active &&
            g_event_system.subscribers[i].event_id == event_id) {
            g_event_system.subscribers[i].active = false;
            g_event_system.subscriber_count--;
            count++;
        }
    }

    cf_mutex_unlock(g_event_system.mutex);

    return count;
}

cf_status_t cf_event_publish(cf_event_id_t event_id)
{
    return cf_event_publish_data(event_id, NULL, 0);
}

cf_status_t cf_event_publish_data(cf_event_id_t event_id,
                                   const void* data,
                                   size_t data_size)
{
    if (!g_event_system.initialized) {
        return CF_ERROR_NOT_INITIALIZED;
    }

    if (data_size > 0 && data == NULL) {
        return CF_ERROR_NULL_POINTER;
    }

    cf_mutex_lock(g_event_system.mutex, CF_WAIT_FOREVER);

    g_event_system.total_published++;

    // Deliver to all matching subscribers
    for (uint32_t i = 0; i < CF_EVENT_MAX_SUBSCRIBERS; i++) {
        cf_event_subscriber_s* sub = &g_event_system.subscribers[i];

        if (!sub->active) {
            continue;
        }

        // Match specific event or wildcard (event_id=0)
        if (sub->event_id == event_id || sub->event_id == 0) {
            deliver_to_subscriber(sub, event_id, data, data_size);
        }
    }

    cf_mutex_unlock(g_event_system.mutex);

    return CF_OK;
}

uint32_t cf_event_get_subscriber_count(void)
{
    if (!g_event_system.initialized) {
        return 0;
    }

    cf_mutex_lock(g_event_system.mutex, CF_WAIT_FOREVER);
    uint32_t count = g_event_system.subscriber_count;
    cf_mutex_unlock(g_event_system.mutex);

    return count;
}

uint32_t cf_event_get_event_subscriber_count(cf_event_id_t event_id)
{
    if (!g_event_system.initialized) {
        return 0;
    }

    uint32_t count = 0;

    cf_mutex_lock(g_event_system.mutex, CF_WAIT_FOREVER);

    for (uint32_t i = 0; i < CF_EVENT_MAX_SUBSCRIBERS; i++) {
        if (g_event_system.subscribers[i].active &&
            (g_event_system.subscribers[i].event_id == event_id ||
             g_event_system.subscribers[i].event_id == 0)) {
            count++;
        }
    }

    cf_mutex_unlock(g_event_system.mutex);

    return count;
}

bool cf_event_is_initialized(void)
{
    return g_event_system.initialized;
}

#endif /* CF_EVENT_ENABLED && CF_RTOS_ENABLED */
