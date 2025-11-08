/**
 * @file cf_gpio_esp32.c
 * @brief CFramework GPIO platform implementation for ESP32
 * @version 1.0.0
 * @date 2025-11-04
 *
 * @copyright Copyright (c) 2025
 *
 * Port CFramework GPIO driver to ESP32 (ESP-IDF)
 * - Supports all GPIO modes including EXTI interrupts
 * - Static memory allocation (no heap)
 * - Auto NVIC/ISR configuration
 */

#include "hal/cf_gpio.h"
#include "hal/cf_gpio_port.h"
#include "utils/cf_log.h"
#include "driver/gpio.h"
#include <string.h>/* ============================================================
 * Platform-specific data structure
 * ============================================================ */
typedef struct {
    gpio_num_t gpio_num;              // ESP32 GPIO number (0-39)
    cf_gpio_irq_callback_t irq_callback;  // User callback
    void* irq_user_data;              // User data for callback
} esp32_gpio_data_t;

/* Static pool for GPIO platform data (no heap allocation) */
#define MAX_GPIO_HANDLES 16
static esp32_gpio_data_t g_platform_data_pool[MAX_GPIO_HANDLES];
static uint8_t g_platform_data_used[MAX_GPIO_HANDLES] = {0};

/* ISR lookup table: maps GPIO number to platform data */
static esp32_gpio_data_t* g_irq_table[GPIO_NUM_MAX] = {NULL};

/* Global flag to track if ISR service is installed */
static bool g_isr_service_installed = false;

/* ============================================================
 * Helper Functions
 * ============================================================ */

/**
 * @brief Allocate platform data from static pool
 */
static esp32_gpio_data_t* alloc_platform_data(void)
{
    for (int i = 0; i < MAX_GPIO_HANDLES; i++) {
        if (g_platform_data_used[i] == 0) {
            g_platform_data_used[i] = 1;
            memset(&g_platform_data_pool[i], 0, sizeof(esp32_gpio_data_t));
            return &g_platform_data_pool[i];
        }
    }
    return NULL;  // Pool exhausted
}

/**
 * @brief Free platform data back to pool
 */
static void free_platform_data(esp32_gpio_data_t* pdata)
{
    if (pdata == NULL) return;

    for (int i = 0; i < MAX_GPIO_HANDLES; i++) {
        if (&g_platform_data_pool[i] == pdata) {
            g_platform_data_used[i] = 0;
            memset(pdata, 0, sizeof(esp32_gpio_data_t));
            return;
        }
    }
}

/**
 * @brief Convert CFramework GPIO mode to ESP32 gpio_mode_t
 */
static gpio_mode_t convert_mode(cf_gpio_mode_t mode)
{
    switch (mode) {
        case CF_GPIO_MODE_INPUT:
            return GPIO_MODE_INPUT;
        case CF_GPIO_MODE_OUTPUT_PP:
        case CF_GPIO_MODE_OUTPUT_OD:
            return GPIO_MODE_OUTPUT;
        case CF_GPIO_MODE_IT_RISING:
        case CF_GPIO_MODE_IT_FALLING:
        case CF_GPIO_MODE_IT_RISING_FALLING:
            return GPIO_MODE_INPUT;  // Interrupt is input mode
        default:
            return GPIO_MODE_DISABLE;
    }
}

/**
 * @brief Convert CFramework GPIO mode to ESP32 gpio_int_type_t
 */
static gpio_int_type_t convert_interrupt_type(cf_gpio_mode_t mode)
{
    switch (mode) {
        case CF_GPIO_MODE_IT_RISING:
            return GPIO_INTR_POSEDGE;
        case CF_GPIO_MODE_IT_FALLING:
            return GPIO_INTR_NEGEDGE;
        case CF_GPIO_MODE_IT_RISING_FALLING:
            return GPIO_INTR_ANYEDGE;
        default:
            return GPIO_INTR_DISABLE;
    }
}

/**
 * @brief Convert CFramework pull mode to ESP32 gpio_pull_mode_t
 */
static gpio_pull_mode_t convert_pull(cf_gpio_pull_t pull)
{
    switch (pull) {
        case CF_GPIO_PULL_UP:
            return GPIO_PULLUP_ONLY;
        case CF_GPIO_PULL_DOWN:
            return GPIO_PULLDOWN_ONLY;
        case CF_GPIO_PULL_NONE:
        default:
            return GPIO_FLOATING;
    }
}

/**
 * @brief ISR handler called by ESP-IDF GPIO driver
 * NOTE: Must have IRAM_ATTR for ISR context
 */
static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    esp32_gpio_data_t* pdata = (esp32_gpio_data_t*)arg;

    if (pdata != NULL && pdata->irq_callback != NULL) {
        // Call user callback with NULL handle (ISR context)
        pdata->irq_callback(NULL, pdata->irq_user_data);
    }
}

/* ============================================================
 * Platform Interface Implementation
 * ============================================================ */

cf_status_t cf_gpio_port_init(cf_gpio_handle_t handle, const cf_gpio_config_t* config)
{
    if (handle == NULL || config == NULL) {
        return CF_ERROR_INVALID_PARAM;
    }

    // Allocate platform data from static pool
    esp32_gpio_data_t* pdata = alloc_platform_data();
    if (pdata == NULL) {
        CF_LOG_E("Platform data pool exhausted");
        return CF_ERROR_NO_MEMORY;
    }

    // Calculate GPIO number from port and pin
    // ESP32: GPIO0-39 (single port, no GPIOA/GPIOB concept)
    // For compatibility, we treat port as group: port 0 = GPIO0-15, port 1 = GPIO16-31
    gpio_num_t gpio_num = (gpio_num_t)(config->port * 16 + config->pin);

    if (gpio_num >= GPIO_NUM_MAX) {
        free_platform_data(pdata);
        CF_LOG_E("Invalid GPIO number: %d", gpio_num);
        return CF_ERROR_INVALID_PARAM;
    }    // Store GPIO number and callbacks
    pdata->gpio_num = gpio_num;
    pdata->irq_callback = config->irq_callback;
    pdata->irq_user_data = config->irq_user_data;

    // Convert CFramework config to ESP32 gpio_config_t
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << gpio_num),
        .mode = convert_mode(config->mode),
        .pull_up_en = (convert_pull(config->pull) == GPIO_PULLUP_ONLY) ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
        .pull_down_en = (convert_pull(config->pull) == GPIO_PULLDOWN_ONLY) ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE,
        .intr_type = convert_interrupt_type(config->mode)
    };

    // Configure GPIO
    esp_err_t err = gpio_config(&io_conf);
    if (err != ESP_OK) {
        free_platform_data(pdata);
        CF_LOG_E("gpio_config failed: %d", err);
        return CF_ERROR_HARDWARE;
    }

    // If interrupt mode, install ISR service and hook handler
    if (config->mode >= CF_GPIO_MODE_IT_RISING) {
        // Install ISR service once (global)
        if (!g_isr_service_installed) {
            err = gpio_install_isr_service(0);
            if (err != ESP_OK) {
                free_platform_data(pdata);
                CF_LOG_E("gpio_install_isr_service failed: %d", err);
                return CF_ERROR_HARDWARE;
            }
            g_isr_service_installed = true;
        }

        // Hook ISR handler for this GPIO
        err = gpio_isr_handler_add(gpio_num, gpio_isr_handler, pdata);
        if (err != ESP_OK) {
            free_platform_data(pdata);
            CF_LOG_E("gpio_isr_handler_add failed: %d", err);
            return CF_ERROR_HARDWARE;
        }        // Register in IRQ table
        g_irq_table[gpio_num] = pdata;
    }

    // Store platform data in handle
    handle->platform_data = pdata;

    return CF_OK;
}

cf_status_t cf_gpio_port_deinit(cf_gpio_handle_t handle)
{
    if (handle == NULL || handle->platform_data == NULL) {
        return CF_ERROR_INVALID_PARAM;
    }

    esp32_gpio_data_t* pdata = (esp32_gpio_data_t*)handle->platform_data;

    // Remove ISR handler if registered
    if (g_irq_table[pdata->gpio_num] == pdata) {
        gpio_isr_handler_remove(pdata->gpio_num);
        g_irq_table[pdata->gpio_num] = NULL;
    }

    // Reset GPIO to default state
    gpio_reset_pin(pdata->gpio_num);

    // Free platform data
    free_platform_data(pdata);
    handle->platform_data = NULL;

    return CF_OK;
}

cf_status_t cf_gpio_port_write(cf_gpio_handle_t handle, cf_gpio_pin_state_t state)
{
    if (handle == NULL || handle->platform_data == NULL) {
        return CF_ERROR_INVALID_PARAM;
    }

    esp32_gpio_data_t* pdata = (esp32_gpio_data_t*)handle->platform_data;

    esp_err_t err = gpio_set_level(pdata->gpio_num, (state == CF_GPIO_PIN_SET) ? 1 : 0);
    if (err != ESP_OK) {
        return CF_ERROR_HARDWARE;
    }

    return CF_OK;
}

cf_status_t cf_gpio_port_read(cf_gpio_handle_t handle, cf_gpio_pin_state_t* state)
{
    if (handle == NULL || handle->platform_data == NULL || state == NULL) {
        return CF_ERROR_INVALID_PARAM;
    }

    esp32_gpio_data_t* pdata = (esp32_gpio_data_t*)handle->platform_data;

    int level = gpio_get_level(pdata->gpio_num);
    *state = (level == 1) ? CF_GPIO_PIN_SET : CF_GPIO_PIN_RESET;

    return CF_OK;
}

cf_status_t cf_gpio_port_toggle(cf_gpio_handle_t handle)
{
    if (handle == NULL || handle->platform_data == NULL) {
        return CF_ERROR_INVALID_PARAM;
    }

    cf_gpio_pin_state_t current_state;
    cf_status_t status = cf_gpio_port_read(handle, &current_state);
    if (status != CF_OK) {
        return status;
    }

    cf_gpio_pin_state_t new_state = (current_state == CF_GPIO_PIN_SET) ?
                                     CF_GPIO_PIN_RESET : CF_GPIO_PIN_SET;

    return cf_gpio_port_write(handle, new_state);
}

/* ============================================================
 * Public Dispatcher (not used in ESP32, but keep for API compatibility)
 * ============================================================ */
void cf_gpio_exti_callback(uint16_t GPIO_Pin)
{
    // ESP32 uses gpio_isr_handler directly, this function is not needed
    // But keep it for API compatibility with STM32 port
    (void)GPIO_Pin;
}
