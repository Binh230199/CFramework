/**
 * @file cf_ringbuf.h
 * @brief Ring buffer implementation
 * @version 1.0.0
 * @date 2025-10-29
 * @author CFramework Contributors
 *
 * @copyright Copyright (c) 2025 CFramework
 * Licensed under MIT License
 */

#ifndef CF_RINGBUF_H
#define CF_RINGBUF_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cf_common.h"

//==============================================================================
// TYPE DEFINITIONS
//==============================================================================

/**
 * @brief Ring buffer structure
 */
typedef struct {
    uint8_t* buffer;        /**< Data buffer */
    uint32_t size;          /**< Buffer size */
    uint32_t head;          /**< Write position */
    uint32_t tail;          /**< Read position */
    uint32_t count;         /**< Number of bytes in buffer */
#if CF_RTOS_ENABLED
    void* mutex;            /**< Mutex for thread safety */
#endif
} cf_ringbuf_t;

//==============================================================================
// PUBLIC API
//==============================================================================

/**
 * @brief Initialize ring buffer with static buffer
 *
 * @param[out] rb Ring buffer structure
 * @param[in] buffer Data buffer
 * @param[in] size Buffer size
 *
 * @return CF_OK on success
 * @return CF_ERROR_NULL_POINTER if rb or buffer is NULL
 * @return CF_ERROR_INVALID_PARAM if size is 0
 *
 * @note This function is thread-safe
 */
cf_status_t cf_ringbuf_init(cf_ringbuf_t* rb, uint8_t* buffer, uint32_t size);

/**
 * @brief Deinitialize ring buffer
 *
 * @param[in] rb Ring buffer structure
 *
 * @note This function is thread-safe
 */
void cf_ringbuf_deinit(cf_ringbuf_t* rb);

/**
 * @brief Write data to ring buffer
 *
 * @param[in] rb Ring buffer structure
 * @param[in] data Data to write
 * @param[in] len Length of data
 *
 * @return Number of bytes written
 *
 * @note This function is thread-safe
 */
uint32_t cf_ringbuf_write(cf_ringbuf_t* rb, const uint8_t* data, uint32_t len);

/**
 * @brief Read data from ring buffer
 *
 * @param[in] rb Ring buffer structure
 * @param[out] data Buffer to receive data
 * @param[in] len Maximum length to read
 *
 * @return Number of bytes read
 *
 * @note This function is thread-safe
 */
uint32_t cf_ringbuf_read(cf_ringbuf_t* rb, uint8_t* data, uint32_t len);

/**
 * @brief Peek data without removing from buffer
 *
 * @param[in] rb Ring buffer structure
 * @param[out] data Buffer to receive data
 * @param[in] len Maximum length to peek
 *
 * @return Number of bytes peeked
 *
 * @note This function is thread-safe
 */
uint32_t cf_ringbuf_peek(cf_ringbuf_t* rb, uint8_t* data, uint32_t len);

/**
 * @brief Get number of bytes available in buffer
 *
 * @param[in] rb Ring buffer structure
 *
 * @return Number of bytes available
 *
 * @note This function is thread-safe
 */
uint32_t cf_ringbuf_available(cf_ringbuf_t* rb);

/**
 * @brief Get free space in buffer
 *
 * @param[in] rb Ring buffer structure
 *
 * @return Number of free bytes
 *
 * @note This function is thread-safe
 */
uint32_t cf_ringbuf_free_space(cf_ringbuf_t* rb);

/**
 * @brief Check if buffer is empty
 *
 * @param[in] rb Ring buffer structure
 *
 * @return true if empty, false otherwise
 *
 * @note This function is thread-safe
 */
bool cf_ringbuf_is_empty(cf_ringbuf_t* rb);

/**
 * @brief Check if buffer is full
 *
 * @param[in] rb Ring buffer structure
 *
 * @return true if full, false otherwise
 *
 * @note This function is thread-safe
 */
bool cf_ringbuf_is_full(cf_ringbuf_t* rb);

/**
 * @brief Clear ring buffer
 *
 * @param[in] rb Ring buffer structure
 *
 * @note This function is thread-safe
 */
void cf_ringbuf_clear(cf_ringbuf_t* rb);

#ifdef __cplusplus
}
#endif

#endif /* CF_RINGBUF_H */
