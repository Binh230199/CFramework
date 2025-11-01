/**
 * @file cf_ringbuf.c
 * @brief Ring buffer implementation
 */

#include "utils/cf_ringbuf.h"

#if CF_RTOS_ENABLED
    #include "os/cf_mutex.h"
#endif

#include <string.h>

//==============================================================================
// PRIVATE MACROS
//==============================================================================

#define MIN(a, b) ((a) < (b) ? (a) : (b))

//==============================================================================
// PUBLIC API IMPLEMENTATION
//==============================================================================

cf_status_t cf_ringbuf_init(cf_ringbuf_t* rb, uint8_t* buffer, uint32_t size)
{
    CF_PTR_CHECK(rb);
    CF_PTR_CHECK(buffer);

    if (size == 0) {
        return CF_ERROR_INVALID_PARAM;
    }

    memset(rb, 0, sizeof(cf_ringbuf_t));
    rb->buffer = buffer;
    rb->size = size;
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;

#if CF_RTOS_ENABLED
    cf_mutex_t mutex;
    cf_status_t status = cf_mutex_create(&mutex);
    if (status != CF_OK) {
        return status;
    }
    rb->mutex = mutex;
#endif

    return CF_OK;
}

void cf_ringbuf_deinit(cf_ringbuf_t* rb)
{
    if (rb == NULL) {
        return;
    }

#if CF_RTOS_ENABLED
    if (rb->mutex != NULL) {
        cf_mutex_destroy((cf_mutex_t)rb->mutex);
        rb->mutex = NULL;
    }
#endif

    memset(rb, 0, sizeof(cf_ringbuf_t));
}

uint32_t cf_ringbuf_write(cf_ringbuf_t* rb, const uint8_t* data, uint32_t len)
{
    if (rb == NULL || data == NULL || len == 0) {
        return 0;
    }

#if CF_RTOS_ENABLED
    cf_mutex_lock((cf_mutex_t)rb->mutex, CF_WAIT_FOREVER);
#endif

    // Calculate how much we can write
    uint32_t free = rb->size - rb->count;
    uint32_t to_write = MIN(len, free);

    if (to_write == 0) {
#if CF_RTOS_ENABLED
        cf_mutex_unlock((cf_mutex_t)rb->mutex);
#endif
        return 0;
    }

    // Write data
    for (uint32_t i = 0; i < to_write; i++) {
        rb->buffer[rb->head] = data[i];
        rb->head = (rb->head + 1) % rb->size;
    }

    rb->count += to_write;

#if CF_RTOS_ENABLED
    cf_mutex_unlock((cf_mutex_t)rb->mutex);
#endif

    return to_write;
}

uint32_t cf_ringbuf_read(cf_ringbuf_t* rb, uint8_t* data, uint32_t len)
{
    if (rb == NULL || data == NULL || len == 0) {
        return 0;
    }

#if CF_RTOS_ENABLED
    cf_mutex_lock((cf_mutex_t)rb->mutex, CF_WAIT_FOREVER);
#endif

    // Calculate how much we can read
    uint32_t to_read = MIN(len, rb->count);

    if (to_read == 0) {
#if CF_RTOS_ENABLED
        cf_mutex_unlock((cf_mutex_t)rb->mutex);
#endif
        return 0;
    }

    // Read data
    for (uint32_t i = 0; i < to_read; i++) {
        data[i] = rb->buffer[rb->tail];
        rb->tail = (rb->tail + 1) % rb->size;
    }

    rb->count -= to_read;

#if CF_RTOS_ENABLED
    cf_mutex_unlock((cf_mutex_t)rb->mutex);
#endif

    return to_read;
}

uint32_t cf_ringbuf_peek(cf_ringbuf_t* rb, uint8_t* data, uint32_t len)
{
    if (rb == NULL || data == NULL || len == 0) {
        return 0;
    }

#if CF_RTOS_ENABLED
    cf_mutex_lock((cf_mutex_t)rb->mutex, CF_WAIT_FOREVER);
#endif

    // Calculate how much we can peek
    uint32_t to_peek = MIN(len, rb->count);

    if (to_peek == 0) {
#if CF_RTOS_ENABLED
        cf_mutex_unlock((cf_mutex_t)rb->mutex);
#endif
        return 0;
    }

    // Peek data without moving tail
    uint32_t pos = rb->tail;
    for (uint32_t i = 0; i < to_peek; i++) {
        data[i] = rb->buffer[pos];
        pos = (pos + 1) % rb->size;
    }

#if CF_RTOS_ENABLED
    cf_mutex_unlock((cf_mutex_t)rb->mutex);
#endif

    return to_peek;
}

uint32_t cf_ringbuf_available(cf_ringbuf_t* rb)
{
    if (rb == NULL) {
        return 0;
    }

    return rb->count;
}

uint32_t cf_ringbuf_free_space(cf_ringbuf_t* rb)
{
    if (rb == NULL) {
        return 0;
    }

    return rb->size - rb->count;
}

bool cf_ringbuf_is_empty(cf_ringbuf_t* rb)
{
    if (rb == NULL) {
        return true;
    }

    return rb->count == 0;
}

bool cf_ringbuf_is_full(cf_ringbuf_t* rb)
{
    if (rb == NULL) {
        return false;
    }

    return rb->count == rb->size;
}

void cf_ringbuf_clear(cf_ringbuf_t* rb)
{
    if (rb == NULL) {
        return;
    }

#if CF_RTOS_ENABLED
    cf_mutex_lock((cf_mutex_t)rb->mutex, CF_WAIT_FOREVER);
#endif

    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;

#if CF_RTOS_ENABLED
    cf_mutex_unlock((cf_mutex_t)rb->mutex);
#endif
}
