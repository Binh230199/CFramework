/**
 * @file cf_queue.c
 * @brief Queue wrapper implementation for FreeRTOS
 */

#include "os/cf_queue.h"

#if CF_RTOS_ENABLED

#include "cf_assert.h"
#include "FreeRTOS.h"
#include "queue.h"

//==============================================================================
// TYPE DEFINITIONS
//==============================================================================

struct cf_queue_s {
    QueueHandle_t handle;
};

//==============================================================================
// PUBLIC API IMPLEMENTATION
//==============================================================================

cf_status_t cf_queue_create(cf_queue_t* queue, uint32_t length, uint32_t item_size)
{
    CF_PTR_CHECK(queue);

    if (length == 0 || item_size == 0) {
        return CF_ERROR_INVALID_PARAM;
    }

    // Allocate queue structure
    struct cf_queue_s* q = (struct cf_queue_s*)pvPortMalloc(sizeof(struct cf_queue_s));
    if (q == NULL) {
        return CF_ERROR_NO_MEMORY;
    }

    // Create FreeRTOS queue
    q->handle = xQueueCreate(length, item_size);
    if (q->handle == NULL) {
        vPortFree(q);
        return CF_ERROR_NO_MEMORY;
    }

    *queue = q;
    return CF_OK;
}

void cf_queue_destroy(cf_queue_t queue)
{
    if (queue == NULL) {
        return;
    }

    if (queue->handle != NULL) {
        vQueueDelete(queue->handle);
    }

    vPortFree(queue);
}

cf_status_t cf_queue_send(cf_queue_t queue, const void* item, uint32_t timeout_ms)
{
    CF_PTR_CHECK(queue);
    CF_PTR_CHECK(queue->handle);
    CF_PTR_CHECK(item);

    TickType_t ticks = (timeout_ms == CF_WAIT_FOREVER) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);

    BaseType_t result = xQueueSend(queue->handle, item, ticks);

    if (result == pdTRUE) {
        return CF_OK;
    }

    return CF_ERROR_TIMEOUT;
}

cf_status_t cf_queue_receive(cf_queue_t queue, void* item, uint32_t timeout_ms)
{
    CF_PTR_CHECK(queue);
    CF_PTR_CHECK(queue->handle);
    CF_PTR_CHECK(item);

    TickType_t ticks = (timeout_ms == CF_WAIT_FOREVER) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);

    BaseType_t result = xQueueReceive(queue->handle, item, ticks);

    if (result == pdTRUE) {
        return CF_OK;
    }

    return CF_ERROR_TIMEOUT;
}

uint32_t cf_queue_get_count(cf_queue_t queue)
{
    if (queue == NULL || queue->handle == NULL) {
        return 0;
    }

    return (uint32_t)uxQueueMessagesWaiting(queue->handle);
}

uint32_t cf_queue_get_available(cf_queue_t queue)
{
    if (queue == NULL || queue->handle == NULL) {
        return 0;
    }

    return (uint32_t)uxQueueSpacesAvailable(queue->handle);
}

bool cf_queue_is_empty(cf_queue_t queue)
{
    return cf_queue_get_count(queue) == 0;
}

bool cf_queue_is_full(cf_queue_t queue)
{
    return cf_queue_get_available(queue) == 0;
}

cf_status_t cf_queue_reset(cf_queue_t queue)
{
    CF_PTR_CHECK(queue);
    CF_PTR_CHECK(queue->handle);

    BaseType_t result = xQueueReset(queue->handle);

    if (result == pdTRUE) {
        return CF_OK;
    }

    return CF_ERROR;
}

#endif /* CF_RTOS_ENABLED */
