/**
 * @file cf_event.h
 * @brief Event System - Publish-Subscribe pattern
 * @version 1.0.0
 * @date 2025-10-31
 * @author CFramework Contributors
 *
 * @copyright Copyright (c) 2025 CFramework
 * Licensed under MIT License
 */

#ifndef CF_EVENT_H
#define CF_EVENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cf_common.h"
#include "cf_event_types.h"  /* Include common types and utilities */

#if CF_EVENT_ENABLED && CF_RTOS_ENABLED

//==============================================================================
// TYPE DEFINITIONS
//==============================================================================

/**
 * @brief Event ID type
 */
typedef uint32_t cf_event_id_t;

/**
 * @brief Event callback function
 *
 * @param[in] event_id Event identifier
 * @param[in] data Event data (can be NULL)
 * @param[in] data_size Size of event data
 * @param[in] user_data User data passed during subscription
 */
typedef void (*cf_event_callback_t)(cf_event_id_t event_id,
                                     const void* data,
                                     size_t data_size,
                                     void* user_data);

/**
 * @brief Event delivery mode
 */
typedef enum {
    CF_EVENT_SYNC,      /**< Synchronous - callback executed immediately */
    CF_EVENT_ASYNC      /**< Asynchronous - callback dispatched to ThreadPool */
} cf_event_mode_t;

/**
 * @brief Subscriber handle
 */
typedef struct cf_event_subscriber_s* cf_event_subscriber_t;

//==============================================================================
// PUBLIC API
//==============================================================================

/**
 * @brief Initialize event system
 *
 * @return CF_OK on success
 * @return CF_ERROR_ALREADY_INITIALIZED if already initialized
 * @return CF_ERROR_NO_MEMORY if initialization failed
 *
 * @note This function is thread-safe
 * @note Must be called after cf_threadpool_init() for async events
 */
cf_status_t cf_event_init(void);

/**
 * @brief Deinitialize event system
 *
 * @note This function is thread-safe
 * @note All subscribers will be automatically unsubscribed
 */
void cf_event_deinit(void);

/**
 * @brief Subscribe to event
 *
 * @param[in] event_id Event to subscribe to (0 = subscribe to all events)
 * @param[in] callback Callback function
 * @param[in] user_data User data passed to callback
 * @param[in] mode Delivery mode (sync or async)
 * @param[out] handle Pointer to receive subscriber handle (optional)
 *
 * @return CF_OK on success
 * @return CF_ERROR_NULL_POINTER if callback is NULL
 * @return CF_ERROR_NOT_INITIALIZED if event system not initialized
 * @return CF_ERROR_NO_MEMORY if max subscribers reached
 *
 * @note This function is thread-safe
 * @note event_id=0 subscribes to ALL events (wildcard)
 */
cf_status_t cf_event_subscribe(cf_event_id_t event_id,
                                cf_event_callback_t callback,
                                void* user_data,
                                cf_event_mode_t mode,
                                cf_event_subscriber_t* handle);

/**
 * @brief Unsubscribe from event
 *
 * @param[in] handle Subscriber handle
 *
 * @return CF_OK on success
 * @return CF_ERROR_NULL_POINTER if handle is NULL
 * @return CF_ERROR_NOT_FOUND if subscriber not found
 *
 * @note This function is thread-safe
 */
cf_status_t cf_event_unsubscribe(cf_event_subscriber_t handle);

/**
 * @brief Unsubscribe all callbacks for specific event
 *
 * @param[in] event_id Event ID
 *
 * @return Number of subscribers unsubscribed
 *
 * @note This function is thread-safe
 */
uint32_t cf_event_unsubscribe_all(cf_event_id_t event_id);

/**
 * @brief Publish event (no data)
 *
 * @param[in] event_id Event identifier
 *
 * @return CF_OK on success
 * @return CF_ERROR_NOT_INITIALIZED if event system not initialized
 *
 * @note This function is thread-safe
 * @note Sync callbacks execute immediately in caller's context
 * @note Async callbacks are dispatched to ThreadPool
 */
cf_status_t cf_event_publish(cf_event_id_t event_id);

/**
 * @brief Publish event with data
 *
 * @param[in] event_id Event identifier
 * @param[in] data Event data (will be copied)
 * @param[in] data_size Size of data
 *
 * @return CF_OK on success
 * @return CF_ERROR_NULL_POINTER if data is NULL but size > 0
 * @return CF_ERROR_NOT_INITIALIZED if event system not initialized
 * @return CF_ERROR_NO_MEMORY if data copy failed
 *
 * @note This function is thread-safe
 * @note Data is copied internally for async delivery
 */
cf_status_t cf_event_publish_data(cf_event_id_t event_id,
                                   const void* data,
                                   size_t data_size);

/**
 * @brief Get number of active subscribers
 *
 * @return Number of subscribers
 *
 * @note This function is thread-safe
 */
uint32_t cf_event_get_subscriber_count(void);

/**
 * @brief Get number of subscribers for specific event
 *
 * @param[in] event_id Event ID
 *
 * @return Number of subscribers for this event
 *
 * @note This function is thread-safe
 */
uint32_t cf_event_get_event_subscriber_count(cf_event_id_t event_id);

/**
 * @brief Check if event system is initialized
 *
 * @return true if initialized, false otherwise
 *
 * @note This function is thread-safe
 */
bool cf_event_is_initialized(void);

//==============================================================================
// CONVENIENCE MACROS
//==============================================================================

/**
 * @brief Subscribe to event (sync mode, no handle)
 */
#define CF_EVENT_SUBSCRIBE(event_id, callback, user_data) \
    cf_event_subscribe(event_id, callback, user_data, CF_EVENT_SYNC, NULL)

/**
 * @brief Subscribe to event (async mode, no handle)
 */
#define CF_EVENT_SUBSCRIBE_ASYNC(event_id, callback, user_data) \
    cf_event_subscribe(event_id, callback, user_data, CF_EVENT_ASYNC, NULL)

/**
 * @brief Publish event with typed data
 */
#define CF_EVENT_PUBLISH_TYPE(event_id, data_ptr, data_type) \
    cf_event_publish_data(event_id, data_ptr, sizeof(data_type))

#endif /* CF_EVENT_ENABLED && CF_RTOS_ENABLED */

#ifdef __cplusplus
}
#endif

#endif /* CF_EVENT_H */
