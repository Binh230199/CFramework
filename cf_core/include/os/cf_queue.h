/**
 * @file cf_queue.h
 * @brief Queue wrapper for FreeRTOS
 * @version 1.0.0
 * @date 2025-10-29
 * @author CFramework Contributors
 *
 * @copyright Copyright (c) 2025 CFramework
 * Licensed under MIT License
 */

#ifndef CF_QUEUE_H
#define CF_QUEUE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cf_common.h"

#if CF_RTOS_ENABLED

//==============================================================================
// TYPE DEFINITIONS
//==============================================================================

/**
 * @brief Opaque queue handle
 */
typedef struct cf_queue_s* cf_queue_t;

//==============================================================================
// PUBLIC API
//==============================================================================

/**
 * @brief Create a queue
 *
 * @param[out] queue Pointer to receive queue handle
 * @param[in] length Maximum number of items in queue
 * @param[in] item_size Size of each item in bytes
 *
 * @return CF_OK on success
 * @return CF_ERROR_NULL_POINTER if queue is NULL
 * @return CF_ERROR_INVALID_PARAM if length or item_size is 0
 * @return CF_ERROR_NO_MEMORY if creation failed
 *
 * @note This function is thread-safe
 */
cf_status_t cf_queue_create(cf_queue_t* queue, uint32_t length, uint32_t item_size);

/**
 * @brief Destroy a queue
 *
 * @param[in] queue Queue handle to destroy
 *
 * @note This function is thread-safe
 */
void cf_queue_destroy(cf_queue_t queue);

/**
 * @brief Send item to queue
 *
 * @param[in] queue Queue handle
 * @param[in] item Pointer to item to send
 * @param[in] timeout_ms Timeout in milliseconds
 *
 * @return CF_OK on success
 * @return CF_ERROR_NULL_POINTER if queue or item is NULL
 * @return CF_ERROR_TIMEOUT if timeout occurred
 * @return CF_ERROR_QUEUE_FULL if queue is full
 *
 * @note This function is thread-safe
 */
cf_status_t cf_queue_send(cf_queue_t queue, const void* item, uint32_t timeout_ms);

/**
 * @brief Receive item from queue
 *
 * @param[in] queue Queue handle
 * @param[out] item Pointer to buffer to receive item
 * @param[in] timeout_ms Timeout in milliseconds
 *
 * @return CF_OK on success
 * @return CF_ERROR_NULL_POINTER if queue or item is NULL
 * @return CF_ERROR_TIMEOUT if timeout occurred
 * @return CF_ERROR_QUEUE_EMPTY if queue is empty
 *
 * @note This function is thread-safe
 */
cf_status_t cf_queue_receive(cf_queue_t queue, void* item, uint32_t timeout_ms);

/**
 * @brief Get number of items in queue
 *
 * @param[in] queue Queue handle
 *
 * @return Number of items (0 if queue is NULL)
 *
 * @note This function is thread-safe
 */
uint32_t cf_queue_get_count(cf_queue_t queue);

/**
 * @brief Get available spaces in queue
 *
 * @param[in] queue Queue handle
 *
 * @return Number of available spaces (0 if queue is NULL)
 *
 * @note This function is thread-safe
 */
uint32_t cf_queue_get_available(cf_queue_t queue);

/**
 * @brief Check if queue is empty
 *
 * @param[in] queue Queue handle
 *
 * @return true if empty, false otherwise
 *
 * @note This function is thread-safe
 */
bool cf_queue_is_empty(cf_queue_t queue);

/**
 * @brief Check if queue is full
 *
 * @param[in] queue Queue handle
 *
 * @return true if full, false otherwise
 *
 * @note This function is thread-safe
 */
bool cf_queue_is_full(cf_queue_t queue);

/**
 * @brief Reset queue (clear all items)
 *
 * @param[in] queue Queue handle
 *
 * @return CF_OK on success
 * @return CF_ERROR_NULL_POINTER if queue is NULL
 *
 * @note This function is thread-safe
 */
cf_status_t cf_queue_reset(cf_queue_t queue);

#endif /* CF_RTOS_ENABLED */

#ifdef __cplusplus
}
#endif

#endif /* CF_QUEUE_H */
