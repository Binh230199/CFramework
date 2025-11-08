/**
 * @file cf_task.c
 * @brief Task wrapper implementation for FreeRTOS
 */

#include "os/cf_task.h"

#if CF_RTOS_ENABLED

#include "cf_assert.h"
#ifdef ESP_PLATFORM
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
#else
    #include "FreeRTOS.h"
    #include "task.h"
#endif
#include <string.h>

//==============================================================================
// CONSTANTS
//==============================================================================

#define CF_TASK_DEFAULT_STACK_SIZE  512
#define CF_TASK_DEFAULT_NAME        "cf_task"

//==============================================================================
// TYPE DEFINITIONS
//==============================================================================

struct cf_task_s {
    TaskHandle_t handle;
};

//==============================================================================
// PRIVATE FUNCTIONS
//==============================================================================

static UBaseType_t priority_to_freertos(cf_task_priority_t priority)
{
    switch (priority) {
        case CF_TASK_PRIORITY_IDLE:          return tskIDLE_PRIORITY;
        case CF_TASK_PRIORITY_LOW:           return tskIDLE_PRIORITY + 1;
        case CF_TASK_PRIORITY_BELOW_NORMAL:  return tskIDLE_PRIORITY + 2;
        case CF_TASK_PRIORITY_NORMAL:        return tskIDLE_PRIORITY + 3;
        case CF_TASK_PRIORITY_ABOVE_NORMAL:  return tskIDLE_PRIORITY + 4;
        case CF_TASK_PRIORITY_HIGH:          return tskIDLE_PRIORITY + 5;
        case CF_TASK_PRIORITY_REALTIME:      return configMAX_PRIORITIES - 1;
        default:                              return tskIDLE_PRIORITY + 3;
    }
}

//==============================================================================
// PUBLIC API IMPLEMENTATION
//==============================================================================

cf_status_t cf_task_create(cf_task_t* task, const cf_task_config_t* config)
{
    CF_PTR_CHECK(task);
    CF_PTR_CHECK(config);
    CF_PTR_CHECK(config->function);

    // Allocate task structure
    struct cf_task_s* tsk = (struct cf_task_s*)pvPortMalloc(sizeof(struct cf_task_s));
    if (tsk == NULL) {
        return CF_ERROR_NO_MEMORY;
    }

    // Get parameters
    const char* name = config->name ? config->name : CF_TASK_DEFAULT_NAME;
    uint32_t stack_size = config->stack_size > 0 ? config->stack_size : CF_TASK_DEFAULT_STACK_SIZE;
    UBaseType_t priority = priority_to_freertos(config->priority);

    // Create FreeRTOS task
    BaseType_t result = xTaskCreate(
        (TaskFunction_t)config->function,
        name,
        stack_size / sizeof(StackType_t),  // Stack size in words
        config->argument,
        priority,
        &tsk->handle
    );

    if (result != pdPASS) {
        vPortFree(tsk);
        return CF_ERROR_NO_MEMORY;
    }

    *task = tsk;
    return CF_OK;
}

void cf_task_delete(cf_task_t task)
{
    if (task == NULL) {
        // Delete current task
        vTaskDelete(NULL);
        return;
    }

    if (task->handle != NULL) {
        vTaskDelete(task->handle);
    }

    vPortFree(task);
}

void cf_task_delay(uint32_t delay_ms)
{
    vTaskDelay(pdMS_TO_TICKS(delay_ms));
}

cf_task_t cf_task_get_current(void)
{
    // Note: This returns a temporary handle
    // For full implementation, we'd need a handle registry
    static struct cf_task_s current_task;
    current_task.handle = xTaskGetCurrentTaskHandle();
    return &current_task;
}

const char* cf_task_get_name(cf_task_t task)
{
    if (task == NULL) {
        return pcTaskGetName(NULL);
    }

    if (task->handle == NULL) {
        return "unknown";
    }

    return pcTaskGetName(task->handle);
}

void cf_task_start_scheduler(void)
{
    vTaskStartScheduler();

    // Should never reach here
    while(1);
}

void cf_task_config_default(cf_task_config_t* config)
{
    if (config == NULL) {
        return;
    }

    memset(config, 0, sizeof(cf_task_config_t));
    config->name = CF_TASK_DEFAULT_NAME;
    config->stack_size = CF_TASK_DEFAULT_STACK_SIZE;
    config->priority = CF_TASK_PRIORITY_NORMAL;
}

#endif /* CF_RTOS_ENABLED */
