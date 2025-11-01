/**
 * @file main.c
 * @brief CFramework example - Blinky with logging
 *
 * This example demonstrates:
 * - GPIO control
 * - Logger with UART sink
 * - FreeRTOS task
 * - Framework initialization
 *
 * Hardware: STM32L4 board with LED on any GPIO pin
 */

#include "cf.h"
#include "utils/cf_log_uart_sink.h"

//==============================================================================
// CONFIGURATION
//==============================================================================

// LED GPIO configuration
#define LED_PORT        0       // GPIOA
#define LED_PIN         5       // PA5

// UART for logging (platform-specific, defined in main.h or by CubeMX)
extern UART_HandleTypeDef huart2;

//==============================================================================
// GLOBAL VARIABLES
//==============================================================================

static cf_gpio_handle_t led_gpio;
static cf_uart_sink_t uart_sink;

//==============================================================================
// TASKS
//==============================================================================

/**
 * @brief LED blink task
 */
void led_task(void* argument)
{
    (void)argument;

    CF_LOG_I("LED task started");

    uint32_t count = 0;

    while (1) {
        // Toggle LED
        cf_gpio_toggle(led_gpio);

        // Log every 5 toggles
        if (count % 5 == 0) {
            CF_LOG_I("LED toggled %lu times", count);
        }

        count++;

        // Delay 500ms
        cf_task_delay(500);
    }
}

//==============================================================================
// INITIALIZATION
//==============================================================================

/**
 * @brief Initialize framework
 */
static cf_status_t framework_init(void)
{
    cf_status_t status;

    // Initialize logger
    status = cf_log_init();
    if (status != CF_OK) {
        return status;
    }

    // Create UART sink for logging
    cf_uart_sink_config_t uart_config = {
        .uart = &huart2,
        .timeout_ms = 1000
    };

    status = cf_uart_sink_create(&uart_sink, &uart_config, CF_LOG_DEBUG);
    if (status != CF_OK) {
        return status;
    }

    // Register sink with logger
    status = cf_log_add_sink(&uart_sink.base);
    if (status != CF_OK) {
        return status;
    }

    CF_LOG_I("CFramework v%s initialized", cf_get_version());

    return CF_OK;
}

/**
 * @brief Initialize LED GPIO
 */
static cf_status_t led_init(void)
{
    // Configure LED as output
    cf_gpio_config_t gpio_config;
    cf_gpio_config_default(&gpio_config);
    gpio_config.port = LED_PORT;
    gpio_config.pin = LED_PIN;
    gpio_config.mode = CF_GPIO_MODE_OUTPUT_PP;
    gpio_config.pull = CF_GPIO_PULL_NONE;
    gpio_config.speed = CF_GPIO_SPEED_LOW;

    cf_status_t status = cf_gpio_init(&led_gpio, &gpio_config);
    if (status != CF_OK) {
        CF_LOG_E("Failed to initialize LED GPIO: %s", cf_status_to_string(status));
        return status;
    }

    CF_LOG_I("LED GPIO initialized on port %lu pin %lu", gpio_config.port, gpio_config.pin);

    // Turn off LED initially
    cf_gpio_write(led_gpio, CF_GPIO_PIN_RESET);

    return CF_OK;
}

//==============================================================================
// MAIN FUNCTION
//==============================================================================

/**
 * @brief Application entry point (called from main after HAL init)
 */
void app_main(void)
{
    cf_status_t status;

    // Initialize framework
    status = framework_init();
    if (status != CF_OK) {
        // Error: framework init failed
        while (1);
    }

    // Initialize LED
    status = led_init();
    if (status != CF_OK) {
        // Error: LED init failed
        while (1);
    }

    // Create LED task
    cf_task_config_t task_config;
    cf_task_config_default(&task_config);
    task_config.name = "LED_Task";
    task_config.function = led_task;
    task_config.stack_size = 512;
    task_config.priority = CF_TASK_PRIORITY_NORMAL;

    cf_task_t led_task_handle;
    status = cf_task_create(&led_task_handle, &task_config);
    if (status != CF_OK) {
        CF_LOG_E("Failed to create LED task: %s", cf_status_to_string(status));
        while (1);
    }

    CF_LOG_I("Application initialized, starting scheduler...");

    // Start FreeRTOS scheduler
    cf_task_start_scheduler();

    // Should never reach here
    while (1);
}
