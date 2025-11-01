/**
 * @file cf_mutex.h
 * @brief Mutex wrapper for FreeRTOS
 * @version 1.0.0
 * @date 2025-10-29
 * @author CFramework Contributors
 *
 * @copyright Copyright (c) 2025 CFramework
 * Licensed under MIT License
 */

#ifndef CF_MUTEX_H
#define CF_MUTEX_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cf_common.h"

#if CF_RTOS_ENABLED

//==============================================================================
// TYPE DEFINITIONS
//==============================================================================

/**
 * @brief Opaque mutex handle
 */
typedef struct cf_mutex_s* cf_mutex_t;

//==============================================================================
// PUBLIC API
//==============================================================================

/**
 * @brief Create a mutex
 *
 * @param[out] mutex Pointer to receive mutex handle
 *
 * @return CF_OK on success
 * @return CF_ERROR_NULL_POINTER if mutex is NULL
 * @return CF_ERROR_NO_MEMORY if creation failed
 *
 * @note This function is thread-safe
 * @note Created mutex must be destroyed with cf_mutex_destroy()
 */
cf_status_t cf_mutex_create(cf_mutex_t* mutex);

/**
 * @brief Destroy a mutex
 *
 * @param[in] mutex Mutex handle to destroy
 *
 * @note This function is thread-safe
 * @warning Do not destroy a locked mutex
 */
void cf_mutex_destroy(cf_mutex_t mutex);

/**
 * @brief Lock a mutex
 *
 * @param[in] mutex Mutex handle
 * @param[in] timeout_ms Timeout in milliseconds (CF_WAIT_FOREVER for infinite)
 *
 * @return CF_OK on success
 * @return CF_ERROR_NULL_POINTER if mutex is NULL
 * @return CF_ERROR_TIMEOUT if timeout occurred
 * @return CF_ERROR_MUTEX on other mutex errors
 *
 * @note This function is thread-safe
 * @note If timeout is CF_WAIT_FOREVER, this will wait indefinitely
 */
cf_status_t cf_mutex_lock(cf_mutex_t mutex, uint32_t timeout_ms);

/**
 * @brief Unlock a mutex
 *
 * @param[in] mutex Mutex handle
 *
 * @return CF_OK on success
 * @return CF_ERROR_NULL_POINTER if mutex is NULL
 * @return CF_ERROR_MUTEX on mutex errors
 *
 * @note This function is thread-safe
 * @note Mutex must be locked before unlocking
 */
cf_status_t cf_mutex_unlock(cf_mutex_t mutex);

//==============================================================================
// HELPER MACROS
//==============================================================================

/**
 * @brief Lock mutex and return on error
 */
#define CF_MUTEX_LOCK(mutex, timeout) \
    do { \
        cf_status_t _status = cf_mutex_lock((mutex), (timeout)); \
        if (_status != CF_OK) { \
            return _status; \
        } \
    } while(0)

/**
 * @brief Unlock mutex and return on error
 */
#define CF_MUTEX_UNLOCK(mutex) \
    do { \
        cf_status_t _status = cf_mutex_unlock(mutex); \
        if (_status != CF_OK) { \
            return _status; \
        } \
    } while(0)

/**
 * @brief Lock mutex and return custom value on error
 */
#define CF_MUTEX_LOCK_RET(mutex, timeout, ret) \
    do { \
        cf_status_t _status = cf_mutex_lock((mutex), (timeout)); \
        if (_status != CF_OK) { \
            return (ret); \
        } \
    } while(0)

/**
 * @brief Unlock mutex and return custom value on error
 */
#define CF_MUTEX_UNLOCK_RET(mutex, ret) \
    do { \
        cf_status_t _status = cf_mutex_unlock(mutex); \
        if (_status != CF_OK) { \
            return (ret); \
        } \
    } while(0)

#endif /* CF_RTOS_ENABLED */

#ifdef __cplusplus
}
#endif

#endif /* CF_MUTEX_H */
