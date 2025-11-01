/**
 * @file cf_mutex.c
 * @brief Mutex wrapper implementation for FreeRTOS
 */

#include "os/cf_mutex.h"

#if CF_RTOS_ENABLED

#include "cf_assert.h"
#include "FreeRTOS.h"
#include "semphr.h"

//==============================================================================
// TYPE DEFINITIONS
//==============================================================================

struct cf_mutex_s {
    SemaphoreHandle_t handle;
};

//==============================================================================
// PUBLIC API IMPLEMENTATION
//==============================================================================

cf_status_t cf_mutex_create(cf_mutex_t* mutex)
{
    CF_PTR_CHECK(mutex);

    // Allocate mutex structure
    struct cf_mutex_s* mtx = (struct cf_mutex_s*)pvPortMalloc(sizeof(struct cf_mutex_s));
    if (mtx == NULL) {
        return CF_ERROR_NO_MEMORY;
    }

    // Create FreeRTOS mutex
    mtx->handle = xSemaphoreCreateMutex();
    if (mtx->handle == NULL) {
        vPortFree(mtx);
        return CF_ERROR_NO_MEMORY;
    }

    *mutex = mtx;
    return CF_OK;
}

void cf_mutex_destroy(cf_mutex_t mutex)
{
    if (mutex == NULL) {
        return;
    }

    if (mutex->handle != NULL) {
        vSemaphoreDelete(mutex->handle);
    }

    vPortFree(mutex);
}

cf_status_t cf_mutex_lock(cf_mutex_t mutex, uint32_t timeout_ms)
{
    CF_PTR_CHECK(mutex);
    CF_PTR_CHECK(mutex->handle);

    TickType_t ticks = (timeout_ms == CF_WAIT_FOREVER) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);

    BaseType_t result = xSemaphoreTake(mutex->handle, ticks);

    if (result == pdTRUE) {
        return CF_OK;
    }

    return CF_ERROR_TIMEOUT;
}

cf_status_t cf_mutex_unlock(cf_mutex_t mutex)
{
    CF_PTR_CHECK(mutex);
    CF_PTR_CHECK(mutex->handle);

    BaseType_t result = xSemaphoreGive(mutex->handle);

    if (result == pdTRUE) {
        return CF_OK;
    }

    return CF_ERROR_MUTEX;
}

#endif /* CF_RTOS_ENABLED */
