/**
 * @file cf_timer.c
 * @brief FreeRTOS software timer wrapper implementation
 */

#include "os/cf_timer.h"

#if CF_RTOS_ENABLED

#include "cf_assert.h"
#include <string.h>

//==============================================================================
// PRIVATE TYPES
//==============================================================================

/**
 * @brief Timer context (for callback wrapper)
 */
typedef struct {
    cf_timer_callback_t user_callback;
    void* user_arg;
} cf_timer_context_t;

//==============================================================================
// PRIVATE FUNCTIONS
//==============================================================================

/**
 * @brief FreeRTOS timer callback wrapper
 */
static void timer_callback_wrapper(TimerHandle_t xTimer)
{
    cf_timer_context_t* ctx = (cf_timer_context_t*)pvTimerGetTimerID(xTimer);

    if (ctx != NULL && ctx->user_callback != NULL) {
        ctx->user_callback((cf_timer_t)xTimer, ctx->user_arg);
    }
}

//==============================================================================
// PUBLIC API IMPLEMENTATION
//==============================================================================

cf_status_t cf_timer_create(cf_timer_t* handle, const cf_timer_config_t* config)
{
    CF_PTR_CHECK(handle);
    CF_PTR_CHECK(config);
    CF_PTR_CHECK(config->callback);

    if (config->period_ms == 0) {
        return CF_ERROR_INVALID_PARAM;
    }

    // Allocate context for callback
    cf_timer_context_t* ctx = (cf_timer_context_t*)pvPortMalloc(sizeof(cf_timer_context_t));
    if (ctx == NULL) {
        return CF_ERROR_NO_MEMORY;
    }

    ctx->user_callback = config->callback;
    ctx->user_arg = config->argument;

    // Convert period to ticks
    TickType_t period_ticks = pdMS_TO_TICKS(config->period_ms);
    if (period_ticks == 0) {
        period_ticks = 1;  // Minimum 1 tick
    }

    // Create timer
    TimerHandle_t timer = xTimerCreate(
        config->name ? config->name : "cf_timer",
        period_ticks,
        config->type,
        ctx,  // Timer ID = context pointer
        timer_callback_wrapper
    );

    if (timer == NULL) {
        vPortFree(ctx);
        return CF_ERROR_NO_MEMORY;
    }

    *handle = (cf_timer_t)timer;

    // Auto-start if requested
    if (config->auto_start) {
        BaseType_t result = xTimerStart(timer, 0);
        if (result != pdPASS) {
            xTimerDelete(timer, 0);
            vPortFree(ctx);
            return CF_ERROR;
        }
    }

    return CF_OK;
}

cf_status_t cf_timer_delete(cf_timer_t handle, uint32_t timeout_ms)
{
    CF_PTR_CHECK(handle);

    TimerHandle_t timer = (TimerHandle_t)handle;

    // Get context and free it
    cf_timer_context_t* ctx = (cf_timer_context_t*)pvTimerGetTimerID(timer);
    if (ctx != NULL) {
        vPortFree(ctx);
    }

    // Delete timer
    TickType_t timeout_ticks = (timeout_ms == CF_WAIT_FOREVER) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    BaseType_t result = xTimerDelete(timer, timeout_ticks);

    return (result == pdPASS) ? CF_OK : CF_ERROR_TIMEOUT;
}

cf_status_t cf_timer_start(cf_timer_t handle, uint32_t timeout_ms)
{
    CF_PTR_CHECK(handle);

    TickType_t timeout_ticks = (timeout_ms == CF_WAIT_FOREVER) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    BaseType_t result = xTimerStart((TimerHandle_t)handle, timeout_ticks);

    return (result == pdPASS) ? CF_OK : CF_ERROR_TIMEOUT;
}

cf_status_t cf_timer_stop(cf_timer_t handle, uint32_t timeout_ms)
{
    CF_PTR_CHECK(handle);

    TickType_t timeout_ticks = (timeout_ms == CF_WAIT_FOREVER) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    BaseType_t result = xTimerStop((TimerHandle_t)handle, timeout_ticks);

    return (result == pdPASS) ? CF_OK : CF_ERROR_TIMEOUT;
}

cf_status_t cf_timer_reset(cf_timer_t handle, uint32_t timeout_ms)
{
    CF_PTR_CHECK(handle);

    TickType_t timeout_ticks = (timeout_ms == CF_WAIT_FOREVER) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    BaseType_t result = xTimerReset((TimerHandle_t)handle, timeout_ticks);

    return (result == pdPASS) ? CF_OK : CF_ERROR_TIMEOUT;
}

cf_status_t cf_timer_change_period(cf_timer_t handle,
                                    uint32_t new_period_ms,
                                    uint32_t timeout_ms)
{
    CF_PTR_CHECK(handle);

    if (new_period_ms == 0) {
        return CF_ERROR_INVALID_PARAM;
    }

    TickType_t period_ticks = pdMS_TO_TICKS(new_period_ms);
    if (period_ticks == 0) {
        period_ticks = 1;
    }

    TickType_t timeout_ticks = (timeout_ms == CF_WAIT_FOREVER) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    BaseType_t result = xTimerChangePeriod((TimerHandle_t)handle, period_ticks, timeout_ticks);

    return (result == pdPASS) ? CF_OK : CF_ERROR_TIMEOUT;
}

bool cf_timer_is_active(cf_timer_t handle)
{
    if (handle == NULL) {
        return false;
    }

    return (xTimerIsTimerActive((TimerHandle_t)handle) != pdFALSE);
}

const char* cf_timer_get_name(cf_timer_t handle)
{
    if (handle == NULL) {
        return "Unknown";
    }

    return pcTimerGetName((TimerHandle_t)handle);
}

void cf_timer_config_default(cf_timer_config_t* config)
{
    if (config == NULL) {
        return;
    }

    memset(config, 0, sizeof(cf_timer_config_t));
    config->name = "timer";
    config->period_ms = 1000;
    config->type = CF_TIMER_PERIODIC;
    config->callback = NULL;
    config->argument = NULL;
    config->auto_start = false;
}

#endif /* CF_RTOS_ENABLED */
