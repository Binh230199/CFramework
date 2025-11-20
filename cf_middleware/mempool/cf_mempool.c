/**
 * @file cf_mempool.c
 * @brief Memory Pool Middleware Implementation
 * @version 1.0.0
 * @date 2025-11-15
 */

#include "cf_mempool.h"
#include "cf_log.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

//==============================================================================
// PRIVATE CONSTANTS
//==============================================================================

#define CF_MEMPOOL_MAGIC            0xDEADBEEF  /**< Magic number for validation */
#define CF_MEMPOOL_NAME_MAX_LEN     16          /**< Maximum pool name length */
#define CF_MEMPOOL_INVALID_INDEX    0xFF        /**< Invalid pool index marker */

//==============================================================================
// PRIVATE TYPES
//==============================================================================

/**
 * @brief Memory pool structure (opaque implementation)
 */
struct cf_mempool_s {
    uint32_t magic;                             /**< Magic number for validation */
    bool active;                                /**< Pool is active */

    // Configuration (read-only after creation)
    uint32_t block_size;                        /**< Size of each block */
    uint32_t block_count;                       /**< Number of blocks */
    char name[CF_MEMPOOL_NAME_MAX_LEN];         /**< Pool name */

    // Memory management
    uint8_t* memory_base;                       /**< Start of pool memory */
    volatile uint32_t free_mask_low;            /**< Free blocks mask (bits 0-31) */
    volatile uint32_t free_mask_high;           /**< Free blocks mask (bits 32-63) */
    volatile uint32_t alloc_hint;               /**< Next allocation hint */

    // Thread safety
    cf_mutex_t mutex;                           /**< Per-pool mutex */

    // Statistics
    volatile uint32_t total_allocations;        /**< Total allocations */
    volatile uint32_t total_deallocations;      /**< Total deallocations */
    volatile uint32_t current_used;             /**< Current blocks in use */
    volatile uint32_t peak_used;                /**< Peak usage */
    volatile uint32_t allocation_failures;      /**< Failed allocations */
    volatile uint32_t fragmentation_count;      /**< Fragmentation events */
};

/**
 * @brief Pool manager structure
 */
typedef struct {
    bool initialized;                           /**< System initialized flag */
    cf_mutex_t global_mutex;                    /**< Global system mutex */

    // Pool storage
    struct cf_mempool_s pools[CF_MEMPOOL_MAX_POOLS];
    uint8_t pool_count;                         /**< Active pool count */

    // Fast size-to-pool lookup table
    uint8_t size_to_pool_map[CF_MEMPOOL_MAX_SIZE + 1];

    // Global statistics
    volatile uint32_t global_allocations;       /**< Global allocation counter */
    volatile uint32_t global_failures;          /**< Global failure counter */
    volatile uint32_t fragmentation_events;     /**< Global fragmentation events */
} cf_mempool_manager_t;

//==============================================================================
// PRIVATE VARIABLES
//==============================================================================

static cf_mempool_manager_t g_pool_manager = {0};

//==============================================================================
// PRIVATE FUNCTION DECLARATIONS
//==============================================================================

static bool validate_config(const cf_mempool_config_t* config);
static bool validate_handle(cf_mempool_handle_t handle);
static cf_status_t find_free_block(struct cf_mempool_s* pool, uint32_t* block_index);
static void mark_block_used(struct cf_mempool_s* pool, uint32_t block_index);
static void mark_block_free(struct cf_mempool_s* pool, uint32_t block_index);
static bool is_block_free(struct cf_mempool_s* pool, uint32_t block_index);
static void* get_block_address(struct cf_mempool_s* pool, uint32_t block_index);
static cf_status_t get_block_index(struct cf_mempool_s* pool, const void* ptr, uint32_t* index);
static void update_size_to_pool_map(void);
static struct cf_mempool_s* find_pool_for_size(size_t size);
static struct cf_mempool_s* find_pool_by_pointer(const void* ptr);

//==============================================================================
// PUBLIC API IMPLEMENTATION - SYSTEM MANAGEMENT
//==============================================================================

cf_status_t cf_mempool_init(void)
{
    if (g_pool_manager.initialized) {
        return CF_ERROR;
    }

    // Initialize global mutex
    cf_status_t status = cf_mutex_create(&g_pool_manager.global_mutex);
    if (status != CF_OK) {
        return status;
    }

    // Initialize pool array
    memset(g_pool_manager.pools, 0, sizeof(g_pool_manager.pools));
    g_pool_manager.pool_count = 0;

    // Initialize lookup table to invalid
    memset(g_pool_manager.size_to_pool_map, CF_MEMPOOL_INVALID_INDEX,
           sizeof(g_pool_manager.size_to_pool_map));

    // Reset global statistics
    g_pool_manager.global_allocations = 0;
    g_pool_manager.global_failures = 0;
    g_pool_manager.fragmentation_events = 0;

    g_pool_manager.initialized = true;

#if CF_LOG_ENABLED
    CF_LOG_I("Memory pool system initialized");
#endif

    return CF_OK;
}

void cf_mempool_deinit(void)
{
    if (!g_pool_manager.initialized) {
        return;
    }

    cf_mutex_lock(g_pool_manager.global_mutex, CF_WAIT_FOREVER);

    // Destroy all active pools
    for (uint8_t i = 0; i < CF_MEMPOOL_MAX_POOLS; i++) {
        if (g_pool_manager.pools[i].active) {
            // Destroy pool mutex
            cf_mutex_destroy(g_pool_manager.pools[i].mutex);

            // Clear pool
            memset(&g_pool_manager.pools[i], 0, sizeof(struct cf_mempool_s));
        }
    }

    g_pool_manager.pool_count = 0;
    g_pool_manager.initialized = false;

    cf_mutex_unlock(g_pool_manager.global_mutex);

    // Destroy global mutex
    cf_mutex_destroy(g_pool_manager.global_mutex);

#if CF_LOG_ENABLED
    CF_LOG_I("Memory pool system deinitialized");
#endif
}

//==============================================================================
// PUBLIC API IMPLEMENTATION - POOL MANAGEMENT
//==============================================================================

cf_status_t cf_mempool_create(cf_mempool_handle_t* handle,
                              const cf_mempool_config_t* config)
{
    CF_PTR_CHECK(handle);
    CF_PTR_CHECK(config);

    if (!g_pool_manager.initialized) {
        return CF_ERROR_NOT_INITIALIZED;
    }

    if (!validate_config(config)) {
        return CF_ERROR_INVALID_PARAM;
    }

    cf_mutex_lock(g_pool_manager.global_mutex, CF_WAIT_FOREVER);

    // Find free pool slot
    struct cf_mempool_s* pool = NULL;
    for (uint8_t i = 0; i < CF_MEMPOOL_MAX_POOLS; i++) {
        if (!g_pool_manager.pools[i].active) {
            pool = &g_pool_manager.pools[i];
            break;
        }
    }

    if (!pool) {
        cf_mutex_unlock(g_pool_manager.global_mutex);
        return CF_ERROR_NO_MEMORY;
    }

    // Calculate total memory needed
    size_t total_memory = config->block_count * config->block_size;

    // Allocate pool memory (using malloc for now - could use static allocation)
    uint8_t* memory = (uint8_t*)malloc(total_memory);
    if (!memory) {
        cf_mutex_unlock(g_pool_manager.global_mutex);
        return CF_ERROR_NO_MEMORY;
    }

    // Initialize pool mutex
    cf_status_t status = cf_mutex_create(&pool->mutex);
    if (status != CF_OK) {
        free(memory);
        cf_mutex_unlock(g_pool_manager.global_mutex);
        return status;
    }

    // Initialize pool structure
    pool->magic = CF_MEMPOOL_MAGIC;
    pool->active = true;
    pool->block_size = config->block_size;
    pool->block_count = config->block_count;
    pool->memory_base = memory;

    // Set pool name
    if (config->name) {
        strncpy(pool->name, config->name, CF_MEMPOOL_NAME_MAX_LEN - 1);
        pool->name[CF_MEMPOOL_NAME_MAX_LEN - 1] = '\0';
    } else {
        snprintf(pool->name, CF_MEMPOOL_NAME_MAX_LEN, "pool_%u", g_pool_manager.pool_count);
    }

    // Initialize free masks (all blocks free)
    if (config->block_count <= 32) {
        pool->free_mask_low = (1UL << config->block_count) - 1;
        pool->free_mask_high = 0;
    } else {
        pool->free_mask_low = 0xFFFFFFFF;
        uint32_t remaining = config->block_count - 32;
        pool->free_mask_high = (remaining >= 32) ? 0xFFFFFFFF : (1UL << remaining) - 1;
    }

    pool->alloc_hint = 0;

    // Initialize statistics
    pool->total_allocations = 0;
    pool->total_deallocations = 0;
    pool->current_used = 0;
    pool->peak_used = 0;
    pool->allocation_failures = 0;
    pool->fragmentation_count = 0;

    // Update pool count
    g_pool_manager.pool_count++;

    // Update size-to-pool lookup table
    update_size_to_pool_map();

    // Return handle
    *handle = pool;

    cf_mutex_unlock(g_pool_manager.global_mutex);

#if CF_LOG_ENABLED
    CF_LOG_I("Created pool '%s': %lu blocks × %lu bytes = %lu bytes total",
             pool->name, config->block_count, config->block_size, total_memory);
#endif

    return CF_OK;
}

cf_status_t cf_mempool_destroy(cf_mempool_handle_t handle)
{
    if (!validate_handle(handle)) {
        return CF_ERROR_INVALID_PARAM;
    }

    struct cf_mempool_s* pool = (struct cf_mempool_s*)handle;

    cf_mutex_lock(g_pool_manager.global_mutex, CF_WAIT_FOREVER);
    cf_mutex_lock(pool->mutex, CF_WAIT_FOREVER);

    // Free pool memory
    free(pool->memory_base);

    // Destroy pool mutex (unlock first)
    cf_mutex_unlock(pool->mutex);
    cf_mutex_destroy(pool->mutex);

    // Clear pool structure
    memset(pool, 0, sizeof(struct cf_mempool_s));

    // Update pool count
    if (g_pool_manager.pool_count > 0) {
        g_pool_manager.pool_count--;
    }

    // Update size-to-pool lookup table
    update_size_to_pool_map();

    cf_mutex_unlock(g_pool_manager.global_mutex);

#if CF_LOG_ENABLED
    CF_LOG_I("Pool destroyed");
#endif

    return CF_OK;
}

//==============================================================================
// PUBLIC API IMPLEMENTATION - MEMORY ALLOCATION
//==============================================================================

void* cf_mempool_alloc_from_pool(cf_mempool_handle_t handle)
{
    if (!validate_handle(handle)) {
        return NULL;
    }

    struct cf_mempool_s* pool = (struct cf_mempool_s*)handle;

    // Quick check without lock
    if (pool->current_used >= pool->block_count) {
        __sync_fetch_and_add(&pool->allocation_failures, 1);
        return NULL;
    }

    // Try to acquire mutex with timeout to avoid blocking
    if (cf_mutex_lock(pool->mutex, 10) != CF_OK) {
        __sync_fetch_and_add(&pool->allocation_failures, 1);
        return NULL;
    }

    // Find free block
    uint32_t block_index;
    cf_status_t status = find_free_block(pool, &block_index);
    if (status != CF_OK) {
        cf_mutex_unlock(pool->mutex);
        __sync_fetch_and_add(&pool->allocation_failures, 1);
        return NULL;
    }

    // Mark block as used
    mark_block_used(pool, block_index);

    // Update statistics
    pool->current_used++;
    pool->total_allocations++;

    if (pool->current_used > pool->peak_used) {
        pool->peak_used = pool->current_used;
    }

    // Update allocation hint
    pool->alloc_hint = (block_index + 1) % pool->block_count;

    cf_mutex_unlock(pool->mutex);

    // Calculate and return block address
    void* ptr = get_block_address(pool, block_index);

    // Update global statistics
    __sync_fetch_and_add(&g_pool_manager.global_allocations, 1);

    return ptr;
}

void* cf_mempool_alloc(size_t size)
{
    if (size == 0 || size > CF_MEMPOOL_MAX_SIZE) {
        return NULL;
    }

    if (!g_pool_manager.initialized) {
        return NULL;
    }

    // Find best-fit pool
    struct cf_mempool_s* best_pool = find_pool_for_size(size);
    if (best_pool) {
        void* ptr = cf_mempool_alloc_from_pool(best_pool);
        if (ptr) {
            // Check if we used a larger pool than needed (fragmentation)
            if (best_pool->block_size > size) {
                __sync_fetch_and_add(&best_pool->fragmentation_count, 1);
                __sync_fetch_and_add(&g_pool_manager.fragmentation_events, 1);
            }
            return ptr;
        }
    }

    // Try fallback to larger pools
    cf_mutex_lock(g_pool_manager.global_mutex, CF_WAIT_FOREVER);

    for (uint8_t i = 0; i < CF_MEMPOOL_MAX_POOLS; i++) {
        struct cf_mempool_s* pool = &g_pool_manager.pools[i];
        if (pool->active && pool->block_size >= size) {
            void* ptr = cf_mempool_alloc_from_pool(pool);
            if (ptr) {
                cf_mutex_unlock(g_pool_manager.global_mutex);

                // Count fragmentation
                if (pool->block_size > size) {
                    __sync_fetch_and_add(&pool->fragmentation_count, 1);
                    __sync_fetch_and_add(&g_pool_manager.fragmentation_events, 1);
                }
                return ptr;
            }
        }
    }

    cf_mutex_unlock(g_pool_manager.global_mutex);

    // All pools exhausted
    __sync_fetch_and_add(&g_pool_manager.global_failures, 1);
    return NULL;
}

cf_status_t cf_mempool_free(void* ptr)
{
    if (!ptr) {
        return CF_OK;  // Safe to free NULL
    }

    // Find pool that owns this pointer
    struct cf_mempool_s* pool = find_pool_by_pointer(ptr);
    if (!pool) {
        return CF_ERROR_INVALID_PARAM;  // Not a pool pointer
    }

    // Get block index
    uint32_t block_index;
    cf_status_t status = get_block_index(pool, ptr, &block_index);
    if (status != CF_OK) {
        return status;
    }

    cf_mutex_lock(pool->mutex, CF_WAIT_FOREVER);

    // Check if block is actually allocated
    if (is_block_free(pool, block_index)) {
        cf_mutex_unlock(pool->mutex);
        return CF_ERROR_INVALID_STATE;  // Double free
    }

    // Mark block as free
    mark_block_free(pool, block_index);

    // Update statistics
    pool->current_used--;
    pool->total_deallocations++;

    cf_mutex_unlock(pool->mutex);

    return CF_OK;
}

//==============================================================================
// PUBLIC API IMPLEMENTATION - STATISTICS
//==============================================================================

cf_status_t cf_mempool_get_stats(cf_mempool_handle_t handle,
                                 cf_mempool_stats_t* stats)
{
    CF_PTR_CHECK(stats);

    if (!validate_handle(handle)) {
        return CF_ERROR_INVALID_PARAM;
    }

    struct cf_mempool_s* pool = (struct cf_mempool_s*)handle;

    cf_mutex_lock(pool->mutex, CF_WAIT_FOREVER);

    stats->total_allocations = pool->total_allocations;
    stats->total_deallocations = pool->total_deallocations;
    stats->current_used = pool->current_used;
    stats->peak_used = pool->peak_used;
    stats->allocation_failures = pool->allocation_failures;
    stats->fragmentation_count = pool->fragmentation_count;
    stats->utilization_percent = (pool->current_used * 100) / pool->block_count;

    cf_mutex_unlock(pool->mutex);

    return CF_OK;
}

cf_status_t cf_mempool_get_global_stats(cf_mempool_global_stats_t* stats)
{
    CF_PTR_CHECK(stats);

    if (!g_pool_manager.initialized) {
        return CF_ERROR_NOT_INITIALIZED;
    }

    cf_mutex_lock(g_pool_manager.global_mutex, CF_WAIT_FOREVER);

    stats->total_pools = g_pool_manager.pool_count;
    stats->global_allocations = g_pool_manager.global_allocations;
    stats->global_failures = g_pool_manager.global_failures;
    stats->fragmentation_events = g_pool_manager.fragmentation_events;

    // Calculate total memory
    stats->total_memory_bytes = 0;
    for (uint8_t i = 0; i < CF_MEMPOOL_MAX_POOLS; i++) {
        if (g_pool_manager.pools[i].active) {
            stats->total_memory_bytes +=
                g_pool_manager.pools[i].block_count * g_pool_manager.pools[i].block_size;
        }
    }

    // Determine overall health
    stats->overall_health = CF_POOL_HEALTH_GOOD;  // Simplified for now

    cf_mutex_unlock(g_pool_manager.global_mutex);

    return CF_OK;
}

cf_pool_health_t cf_mempool_check_health(cf_mempool_handle_t handle)
{
    if (!validate_handle(handle)) {
        return CF_POOL_HEALTH_EMERGENCY;
    }

    struct cf_mempool_s* pool = (struct cf_mempool_s*)handle;

    uint32_t utilization = (pool->current_used * 100) / pool->block_count;

    if (utilization >= 95) {
        return CF_POOL_HEALTH_CRITICAL;
    } else if (utilization >= 80) {
        return CF_POOL_HEALTH_WARNING;
    } else {
        return CF_POOL_HEALTH_GOOD;
    }
}

cf_status_t cf_mempool_reset_stats(cf_mempool_handle_t handle)
{
    if (handle == NULL) {
        // Reset all pools
        if (!g_pool_manager.initialized) {
            return CF_ERROR_NOT_INITIALIZED;
        }

        cf_mutex_lock(g_pool_manager.global_mutex, CF_WAIT_FOREVER);

        for (uint8_t i = 0; i < CF_MEMPOOL_MAX_POOLS; i++) {
            if (g_pool_manager.pools[i].active) {
                struct cf_mempool_s* pool = &g_pool_manager.pools[i];
                cf_mutex_lock(pool->mutex, CF_WAIT_FOREVER);
                pool->total_allocations = 0;
                pool->total_deallocations = 0;
                pool->peak_used = pool->current_used;
                pool->allocation_failures = 0;
                pool->fragmentation_count = 0;
                cf_mutex_unlock(pool->mutex);
            }
        }

        g_pool_manager.global_allocations = 0;
        g_pool_manager.global_failures = 0;
        g_pool_manager.fragmentation_events = 0;

        cf_mutex_unlock(g_pool_manager.global_mutex);
    } else {
        // Reset specific pool
        if (!validate_handle(handle)) {
            return CF_ERROR_INVALID_PARAM;
        }

        struct cf_mempool_s* pool = (struct cf_mempool_s*)handle;

        cf_mutex_lock(pool->mutex, CF_WAIT_FOREVER);
        pool->total_allocations = 0;
        pool->total_deallocations = 0;
        pool->peak_used = pool->current_used;
        pool->allocation_failures = 0;
        pool->fragmentation_count = 0;
        cf_mutex_unlock(pool->mutex);
    }

    return CF_OK;
}

//==============================================================================
// PUBLIC API IMPLEMENTATION - UTILITIES
//==============================================================================

cf_status_t cf_mempool_get_info(cf_mempool_handle_t handle,
                                cf_mempool_config_t* config)
{
    CF_PTR_CHECK(config);

    if (!validate_handle(handle)) {
        return CF_ERROR_INVALID_PARAM;
    }

    struct cf_mempool_s* pool = (struct cf_mempool_s*)handle;

    config->block_size = pool->block_size;
    config->block_count = pool->block_count;
    config->name = pool->name;  // Note: pointer to internal string

    return CF_OK;
}

bool cf_mempool_is_pool_pointer(const void* ptr)
{
    if (!ptr || !g_pool_manager.initialized) {
        return false;
    }

    return find_pool_by_pointer(ptr) != NULL;
}

void cf_mempool_config_default(cf_mempool_config_t* config,
                               uint32_t block_size,
                               uint32_t block_count)
{
    if (!config) {
        return;
    }

    config->block_size = block_size;
    config->block_count = block_count;
    config->name = NULL;  // Will get auto-generated name
}

//==============================================================================
// PRIVATE FUNCTION IMPLEMENTATIONS
//==============================================================================

static bool validate_config(const cf_mempool_config_t* config)
{
    if (!config) {
        return false;
    }

    if (config->block_size == 0 || config->block_size > CF_MEMPOOL_MAX_SIZE) {
        return false;
    }

    if (config->block_count == 0 || config->block_count > 64) {
        return false;  // Limited by bitmask size
    }

    return true;
}

static bool validate_handle(cf_mempool_handle_t handle)
{
    if (!handle || !g_pool_manager.initialized) {
        return false;
    }

    struct cf_mempool_s* pool = (struct cf_mempool_s*)handle;

    return (pool->magic == CF_MEMPOOL_MAGIC && pool->active);
}

static cf_status_t find_free_block(struct cf_mempool_s* pool, uint32_t* block_index)
{
    // Start from allocation hint
    uint32_t start = pool->alloc_hint;

    for (uint32_t i = 0; i < pool->block_count; i++) {
        uint32_t idx = (start + i) % pool->block_count;

        if (is_block_free(pool, idx)) {
            *block_index = idx;
            return CF_OK;
        }
    }

    return CF_ERROR_NO_MEMORY;
}

static void mark_block_used(struct cf_mempool_s* pool, uint32_t block_index)
{
    if (block_index < 32) {
        pool->free_mask_low &= ~(1UL << block_index);
    } else {
        pool->free_mask_high &= ~(1UL << (block_index - 32));
    }
}

static void mark_block_free(struct cf_mempool_s* pool, uint32_t block_index)
{
    if (block_index < 32) {
        pool->free_mask_low |= (1UL << block_index);
    } else {
        pool->free_mask_high |= (1UL << (block_index - 32));
    }
}

static bool is_block_free(struct cf_mempool_s* pool, uint32_t block_index)
{
    if (block_index < 32) {
        return (pool->free_mask_low & (1UL << block_index)) != 0;
    } else {
        return (pool->free_mask_high & (1UL << (block_index - 32))) != 0;
    }
}

static void* get_block_address(struct cf_mempool_s* pool, uint32_t block_index)
{
    return pool->memory_base + (block_index * pool->block_size);
}

static cf_status_t get_block_index(struct cf_mempool_s* pool, const void* ptr, uint32_t* index)
{
    if ((uint8_t*)ptr < pool->memory_base) {
        return CF_ERROR_INVALID_PARAM;
    }

    size_t offset = (uint8_t*)ptr - pool->memory_base;
    uint32_t block_idx = offset / pool->block_size;

    if (block_idx >= pool->block_count) {
        return CF_ERROR_INVALID_PARAM;
    }

    // Check alignment
    if (offset % pool->block_size != 0) {
        return CF_ERROR_INVALID_PARAM;
    }

    *index = block_idx;
    return CF_OK;
}

static void update_size_to_pool_map(void)
{
    // Clear map
    memset(g_pool_manager.size_to_pool_map, CF_MEMPOOL_INVALID_INDEX,
           sizeof(g_pool_manager.size_to_pool_map));

    // Build map for each size
    for (uint32_t size = 1; size <= CF_MEMPOOL_MAX_SIZE; size++) {
        uint32_t best_block_size = UINT32_MAX;
        uint8_t best_pool_idx = CF_MEMPOOL_INVALID_INDEX;

        // Find smallest pool that can fit this size
        for (uint8_t i = 0; i < CF_MEMPOOL_MAX_POOLS; i++) {
            struct cf_mempool_s* pool = &g_pool_manager.pools[i];
            if (pool->active && pool->block_size >= size && pool->block_size < best_block_size) {
                best_block_size = pool->block_size;
                best_pool_idx = i;
            }
        }

        g_pool_manager.size_to_pool_map[size] = best_pool_idx;
    }
}

static struct cf_mempool_s* find_pool_for_size(size_t size)
{
    if (size > CF_MEMPOOL_MAX_SIZE) {
        return NULL;
    }

    uint8_t pool_idx = g_pool_manager.size_to_pool_map[size];
    if (pool_idx == CF_MEMPOOL_INVALID_INDEX) {
        return NULL;
    }

    struct cf_mempool_s* pool = &g_pool_manager.pools[pool_idx];
    return (pool->active) ? pool : NULL;
}

static struct cf_mempool_s* find_pool_by_pointer(const void* ptr)
{
    if (!ptr) {
        return NULL;
    }

    for (uint8_t i = 0; i < CF_MEMPOOL_MAX_POOLS; i++) {
        struct cf_mempool_s* pool = &g_pool_manager.pools[i];
        if (!pool->active) {
            continue;
        }

        uint8_t* base = pool->memory_base;
        size_t total_size = pool->block_count * pool->block_size;

        if ((uint8_t*)ptr >= base && (uint8_t*)ptr < (base + total_size)) {
            return pool;
        }
    }

    return NULL;
}
