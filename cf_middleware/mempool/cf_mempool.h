/**
 * @file cf_mempool.h
 * @brief Memory Pool Middleware for High-Performance Thread-Safe Allocation
 * @version 1.0.0
 * @date 2025-11-15
 * @author CFramework Contributors
 *
 * @copyright Copyright (c) 2025 CFramework
 * Licensed under MIT License
 */

#ifndef CF_MEMPOOL_H
#define CF_MEMPOOL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cf_common.h"
#include "cf_status.h"
#include "cf_mutex.h"
#include "cf_assert.h"

//==============================================================================
// CONFIGURATION
//==============================================================================

#ifndef CF_MEMPOOL_MAX_POOLS
    #define CF_MEMPOOL_MAX_POOLS        8       /**< Maximum number of pools */
#endif

#ifndef CF_MEMPOOL_MAX_SIZE
    #define CF_MEMPOOL_MAX_SIZE         2048    /**< Maximum block size */
#endif

#ifndef CF_MEMPOOL_STATS_ENABLED
    #define CF_MEMPOOL_STATS_ENABLED    1       /**< Enable statistics */
#endif

#ifndef CF_MEMPOOL_HEALTH_CHECK_ENABLED
    #define CF_MEMPOOL_HEALTH_CHECK_ENABLED 1   /**< Enable health monitoring */
#endif

//==============================================================================
// TYPE DEFINITIONS
//==============================================================================

/**
 * @brief Memory pool handle (opaque)
 */
typedef struct cf_mempool_s* cf_mempool_handle_t;

/**
 * @brief Pool configuration structure
 */
typedef struct {
    uint32_t block_size;                        /**< Size of each block in bytes */
    uint32_t block_count;                       /**< Number of blocks in pool */
    const char* name;                           /**< Pool name (for debugging) */
} cf_mempool_config_t;

/**
 * @brief Pool statistics structure
 */
typedef struct {
    uint32_t total_allocations;                 /**< Total allocations performed */
    uint32_t total_deallocations;               /**< Total deallocations performed */
    uint32_t current_used;                      /**< Current blocks in use */
    uint32_t peak_used;                         /**< Peak blocks usage */
    uint32_t allocation_failures;               /**< Failed allocation attempts */
    uint32_t utilization_percent;               /**< Current utilization percentage */
    uint32_t fragmentation_count;               /**< Times larger pool was used */
} cf_mempool_stats_t;

/**
 * @brief Pool health status
 */
typedef enum {
    CF_POOL_HEALTH_GOOD,                        /**< Normal operation */
    CF_POOL_HEALTH_WARNING,                     /**< High usage warning */
    CF_POOL_HEALTH_CRITICAL,                    /**< Critical usage level */
    CF_POOL_HEALTH_EMERGENCY                    /**< Emergency - failure imminent */
} cf_pool_health_t;

/**
 * @brief Global pool manager statistics
 */
typedef struct {
    uint32_t total_pools;                       /**< Number of active pools */
    uint32_t total_memory_bytes;                /**< Total pool memory allocated */
    uint32_t global_allocations;                /**< Global allocation counter */
    uint32_t global_failures;                  /**< Global failure counter */
    uint32_t fragmentation_events;             /**< Total fragmentation events */
    cf_pool_health_t overall_health;            /**< Overall system health */
} cf_mempool_global_stats_t;

//==============================================================================
// PUBLIC API - POOL MANAGEMENT
//==============================================================================

/**
 * @brief Initialize memory pool system
 *
 * Must be called once before using any pool functions.
 *
 * @return CF_OK on success
 * @return CF_ERROR if already initialized
 *
 * @note Thread-safe
 */
cf_status_t cf_mempool_init(void);

/**
 * @brief Deinitialize memory pool system
 *
 * Destroys all pools and frees resources.
 *
 * @note Thread-safe
 * @warning All pool handles become invalid after this call
 */
void cf_mempool_deinit(void);

/**
 * @brief Create a new memory pool
 *
 * @param[out] handle Pointer to receive pool handle
 * @param[in] config Pool configuration
 *
 * @return CF_OK on success
 * @return CF_ERROR_INVALID_PARAM if parameters are invalid
 * @return CF_ERROR_NO_MEMORY if insufficient memory for pool
 * @return CF_ERROR_NOT_INITIALIZED if system not initialized
 *
 * @note Thread-safe
 * @note Pool memory is allocated statically at creation time
 *
 * @code
 * cf_mempool_config_t config = {
 *     .block_size = 64,
 *     .block_count = 20,
 *     .name = "sensor_data"
 * };
 * cf_mempool_handle_t pool;
 * if (cf_mempool_create(&pool, &config) == CF_OK) {
 *     // Use pool...
 * }
 * @endcode
 */
cf_status_t cf_mempool_create(cf_mempool_handle_t* handle,
                              const cf_mempool_config_t* config);

/**
 * @brief Destroy a memory pool
 *
 * @param[in] handle Pool handle to destroy
 *
 * @return CF_OK on success
 * @return CF_ERROR_INVALID_PARAM if handle is invalid
 *
 * @note Thread-safe
 * @warning All pointers allocated from this pool become invalid
 */
cf_status_t cf_mempool_destroy(cf_mempool_handle_t handle);

//==============================================================================
// PUBLIC API - MEMORY ALLOCATION
//==============================================================================

/**
 * @brief Allocate block from specific pool
 *
 * @param[in] handle Pool handle
 *
 * @return Pointer to allocated block on success
 * @return NULL if no free blocks available
 *
 * @note Thread-safe
 * @note Returns blocks of pool's configured block_size
 * @note Allocation time: O(1) average case
 *
 * @code
 * void* ptr = cf_mempool_alloc_from_pool(pool);
 * if (ptr) {
 *     // Use memory...
 *     cf_mempool_free(ptr);
 * }
 * @endcode
 */
void* cf_mempool_alloc_from_pool(cf_mempool_handle_t handle);

/**
 * @brief Smart allocation with best-fit selection
 *
 * Automatically selects the most appropriate pool for the requested size.
 * Uses best-fit algorithm with fallback to larger pools if needed.
 *
 * @param[in] size Requested size in bytes
 *
 * @return Pointer to allocated block on success
 * @return NULL if no suitable pool has free blocks
 *
 * @note Thread-safe
 * @note Prefers exact-fit pools, falls back to larger pools
 * @note May return blocks larger than requested size
 * @note Allocation time: O(pools) worst case, O(1) typical case
 *
 * @code
 * void* ptr = cf_mempool_alloc(28);  // May get 32B block
 * if (ptr) {
 *     // Use memory...
 *     cf_mempool_free(ptr);
 * }
 * @endcode
 */
void* cf_mempool_alloc(size_t size);

/**
 * @brief Free allocated memory block
 *
 * @param[in] ptr Pointer to memory block (from cf_mempool_alloc*)
 *
 * @return CF_OK on success
 * @return CF_ERROR_INVALID_PARAM if ptr is not from a pool
 *
 * @note Thread-safe
 * @note Safe to call with NULL pointer (no operation)
 * @note Automatically determines source pool
 * @note Deallocation time: O(1)
 *
 * @code
 * void* ptr = cf_mempool_alloc(64);
 * // ... use memory ...
 * cf_mempool_free(ptr);  // Always check return for debugging
 * @endcode
 */
cf_status_t cf_mempool_free(void* ptr);

//==============================================================================
// PUBLIC API - STATISTICS AND MONITORING
//==============================================================================

/**
 * @brief Get pool statistics
 *
 * @param[in] handle Pool handle
 * @param[out] stats Pointer to statistics structure
 *
 * @return CF_OK on success
 * @return CF_ERROR_INVALID_PARAM if parameters are invalid
 *
 * @note Thread-safe
 */
cf_status_t cf_mempool_get_stats(cf_mempool_handle_t handle,
                                 cf_mempool_stats_t* stats);

/**
 * @brief Get global pool system statistics
 *
 * @param[out] stats Pointer to global statistics structure
 *
 * @return CF_OK on success
 * @return CF_ERROR_INVALID_PARAM if stats is NULL
 *
 * @note Thread-safe
 */
cf_status_t cf_mempool_get_global_stats(cf_mempool_global_stats_t* stats);

/**
 * @brief Check pool health status
 *
 * @param[in] handle Pool handle
 *
 * @return Health status (CF_POOL_HEALTH_*)
 *
 * @note Thread-safe
 * @note Returns CF_POOL_HEALTH_EMERGENCY for invalid handles
 */
cf_pool_health_t cf_mempool_check_health(cf_mempool_handle_t handle);

/**
 * @brief Reset pool statistics
 *
 * @param[in] handle Pool handle (NULL for all pools)
 *
 * @return CF_OK on success
 *
 * @note Thread-safe
 * @note Does not affect current allocations, only counters
 */
cf_status_t cf_mempool_reset_stats(cf_mempool_handle_t handle);

//==============================================================================
// PUBLIC API - UTILITIES
//==============================================================================

/**
 * @brief Get pool information
 *
 * @param[in] handle Pool handle
 * @param[out] config Pointer to receive pool configuration
 *
 * @return CF_OK on success
 * @return CF_ERROR_INVALID_PARAM if parameters are invalid
 *
 * @note Thread-safe
 */
cf_status_t cf_mempool_get_info(cf_mempool_handle_t handle,
                                cf_mempool_config_t* config);

/**
 * @brief Check if pointer belongs to a pool
 *
 * @param[in] ptr Pointer to check
 *
 * @return true if pointer is from a pool
 * @return false if pointer is not from any pool
 *
 * @note Thread-safe
 * @note Useful for mixed allocation strategies
 */
bool cf_mempool_is_pool_pointer(const void* ptr);

/**
 * @brief Get default configuration
 *
 * @param[out] config Pointer to configuration structure to fill
 * @param[in] block_size Desired block size
 * @param[in] block_count Desired block count
 *
 * @note Helper function to initialize configuration with defaults
 */
void cf_mempool_config_default(cf_mempool_config_t* config,
                               uint32_t block_size,
                               uint32_t block_count);

//==============================================================================
// CONVENIENCE MACROS
//==============================================================================

/**
 * @brief Create pool with default name
 */
#define CF_MEMPOOL_CREATE_DEFAULT(handle, block_size, block_count) \
    do { \
        cf_mempool_config_t _cfg; \
        cf_mempool_config_default(&_cfg, (block_size), (block_count)); \
        cf_mempool_create((handle), &_cfg); \
    } while(0)

/**
 * @brief Safe allocation with NULL check
 */
#define CF_MEMPOOL_ALLOC_SAFE(size, ptr_var) \
    do { \
        (ptr_var) = cf_mempool_alloc(size); \
        if (!(ptr_var)) { \
            CF_LOG_E("Pool allocation failed for size %zu", (size_t)(size)); \
        } \
    } while(0)

/**
 * @brief Safe deallocation with NULL check
 */
#define CF_MEMPOOL_FREE_SAFE(ptr) \
    do { \
        if (ptr) { \
            cf_mempool_free(ptr); \
            (ptr) = NULL; \
        } \
    } while(0)

#ifdef __cplusplus
}
#endif

#endif /* CF_MEMPOOL_H */
