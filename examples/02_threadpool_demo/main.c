/**
 * @file main.c
 * @brief ThreadPool demonstration example
 *
 * This example shows how to use the CFramework ThreadPool for task dispatching.
 * It demonstrates:
 * - ThreadPool initialization
 * - Submitting tasks with different priorities
 * - Task execution tracking
 * - Resource management
 */

#include "cf.h"

//==============================================================================
// TASK DEFINITIONS
//==============================================================================

/**
 * @brief Simple task that prints a message
 */
void simple_task(void* arg)
{
    const char* message = (const char*)arg;
    CF_LOG_I("Task executing: %s", message);

    // Simulate some work
    cf_task_delay(100);

    CF_LOG_I("Task completed: %s", message);
}

/**
 * @brief Sensor reading task
 */
void sensor_task(void* arg)
{
    uint32_t sensor_id = (uint32_t)(uintptr_t)arg;

    CF_LOG_I("Reading sensor %lu...", sensor_id);
    cf_task_delay(50);

    // Simulate sensor reading
    uint32_t value = sensor_id * 100 + (xTaskGetTickCount() % 100);

    CF_LOG_I("Sensor %lu value: %lu", sensor_id, value);
}

/**
 * @brief Data processing task
 */
void processing_task(void* arg)
{
    uint32_t data_id = (uint32_t)(uintptr_t)arg;

    CF_LOG_I("Processing data batch %lu...", data_id);

    // Simulate processing
    for (int i = 0; i < 10; i++) {
        cf_task_delay(10);
    }

    CF_LOG_I("Data batch %lu processed", data_id);
}

/**
 * @brief Critical task (high priority)
 */
void critical_task(void* arg)
{
    CF_LOG_W("CRITICAL TASK: Immediate action required!");

    // Simulate urgent operation
    cf_task_delay(20);

    CF_LOG_W("CRITICAL TASK: Action completed");
}

//==============================================================================
// UART CONFIGURATION FOR LOGGING
//==============================================================================

// NOTE: This is platform-specific - adjust for your hardware
#if defined(CF_PLATFORM_STM32L4) || defined(CF_PLATFORM_STM32L1)
    extern UART_HandleTypeDef huart2;  // Assuming UART2 is configured
    #define LOG_UART_HANDLE &huart2
#elif defined(CF_PLATFORM_ESP32)
    #define LOG_UART_PORT UART_NUM_0
#endif

//==============================================================================
// MAIN APPLICATION TASK
//==============================================================================

/**
 * @brief Main application task
 */
void app_main_task(void* arg)
{
    CF_LOG_I("=== CFramework ThreadPool Demo ===");
    CF_LOG_I("Framework Version: %s", CF_VERSION_STRING);

    // Initialize ThreadPool with default config
    cf_status_t status = cf_threadpool_init();
    if (status != CF_OK) {
        CF_LOG_E("ThreadPool init failed: %d", status);
        return;
    }

    CF_LOG_I("ThreadPool initialized successfully");

    // Wait a bit
    cf_task_delay(1000);

    //==========================================================================
    // DEMO 1: Submit tasks with different priorities
    //==========================================================================

    CF_LOG_I("--- Demo 1: Priority-based task submission ---");

    // Submit low priority tasks
    cf_threadpool_submit(simple_task, "Low Priority Task 1",
                        CF_THREADPOOL_PRIORITY_LOW, CF_WAIT_FOREVER);
    cf_threadpool_submit(simple_task, "Low Priority Task 2",
                        CF_THREADPOOL_PRIORITY_LOW, CF_WAIT_FOREVER);

    // Submit normal priority tasks
    cf_threadpool_submit(simple_task, "Normal Priority Task 1",
                        CF_THREADPOOL_PRIORITY_NORMAL, CF_WAIT_FOREVER);
    cf_threadpool_submit(simple_task, "Normal Priority Task 2",
                        CF_THREADPOOL_PRIORITY_NORMAL, CF_WAIT_FOREVER);

    // Submit high priority task
    cf_threadpool_submit(simple_task, "High Priority Task",
                        CF_THREADPOOL_PRIORITY_HIGH, CF_WAIT_FOREVER);

    // Wait for completion
    cf_task_delay(2000);

    //==========================================================================
    // DEMO 2: Multiple sensor readings
    //==========================================================================

    CF_LOG_I("--- Demo 2: Multiple sensor readings ---");

    for (uint32_t i = 1; i <= 5; i++) {
        cf_threadpool_submit(sensor_task, (void*)(uintptr_t)i,
                            CF_THREADPOOL_PRIORITY_NORMAL, CF_WAIT_FOREVER);
    }

    // Wait for completion
    cf_task_delay(1500);

    //==========================================================================
    // DEMO 3: Data processing pipeline
    //==========================================================================

    CF_LOG_I("--- Demo 3: Data processing pipeline ---");

    for (uint32_t i = 1; i <= 8; i++) {
        cf_threadpool_submit(processing_task, (void*)(uintptr_t)i,
                            CF_THREADPOOL_PRIORITY_NORMAL, CF_WAIT_FOREVER);
    }

    // Wait a bit
    cf_task_delay(500);

    // Submit critical task (should interrupt low priority tasks)
    cf_threadpool_submit(critical_task, NULL,
                        CF_THREADPOOL_PRIORITY_CRITICAL, CF_WAIT_FOREVER);

    // Wait for all tasks to complete
    CF_LOG_I("Waiting for all tasks to complete...");
    status = cf_threadpool_wait_idle(10000);
    if (status == CF_OK) {
        CF_LOG_I("All tasks completed successfully");
    } else {
        CF_LOG_W("Timeout waiting for tasks");
    }

    //==========================================================================
    // Statistics
    //==========================================================================

    CF_LOG_I("--- ThreadPool Statistics ---");
    CF_LOG_I("Active tasks: %lu", cf_threadpool_get_active_count());
    CF_LOG_I("Pending tasks: %lu", cf_threadpool_get_pending_count());
    CF_LOG_I("Idle: %s", cf_threadpool_is_idle() ? "Yes" : "No");

    // Cleanup
    CF_LOG_I("Shutting down ThreadPool...");
    cf_threadpool_deinit(true);

    CF_LOG_I("=== Demo completed ===");

    // Keep task alive
    while (1) {
        cf_task_delay(1000);
    }
}

//==============================================================================
// MAIN ENTRY POINT
//==============================================================================

int main(void)
{
    // Hardware initialization (platform-specific)
    // NOTE: Initialize clocks, GPIO, UART, etc. using vendor HAL

#if defined(CF_PLATFORM_STM32L4) || defined(CF_PLATFORM_STM32L1)
    HAL_Init();
    SystemClock_Config();  // You need to implement this
    // Initialize UART for logging
    // MX_USART2_UART_Init();  // From CubeMX
#elif defined(CF_PLATFORM_ESP32)
    // ESP-IDF initialization done automatically
#endif

    // Initialize CFramework logger
    cf_log_init();

    // Create UART sink for logging
#if defined(CF_PLATFORM_STM32L4) || defined(CF_PLATFORM_STM32L1)
    cf_log_uart_sink_t uart_sink;
    cf_log_uart_sink_init(&uart_sink, LOG_UART_HANDLE, CF_LOG_DEBUG);
    cf_log_add_sink(&uart_sink.base);
#elif defined(CF_PLATFORM_ESP32)
    cf_log_uart_sink_t uart_sink;
    cf_log_uart_sink_init(&uart_sink, LOG_UART_PORT, CF_LOG_DEBUG);
    cf_log_add_sink(&uart_sink.base);
#endif

    CF_LOG_I("System starting...");

    // Create main application task
    cf_task_config_t task_config;
    cf_task_config_default(&task_config);
    task_config.name = "AppMain";
    task_config.function = app_main_task;
    task_config.argument = NULL;
    task_config.stack_size = 4096;
    task_config.priority = CF_TASK_PRIORITY_NORMAL;

    cf_task_t app_task;
    cf_status_t status = cf_task_create(&app_task, &task_config);
    if (status != CF_OK) {
        CF_LOG_E("Failed to create app task: %d", status);
        while (1);  // Halt
    }

    // Start FreeRTOS scheduler
    CF_LOG_I("Starting RTOS scheduler...");
    cf_task_start_scheduler();

    // Should never reach here
    return 0;
}
