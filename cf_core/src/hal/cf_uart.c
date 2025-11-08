/**
 * @file cf_uart.c
 * @brief UART HAL common implementation
 * @version 1.0.0
 * @date 2025-11-04
 */

#include "hal/cf_uart.h"
#include "hal/cf_uart_port.h"
#include "cf_assert.h"
#include <string.h>

/* ============================================================
 * INTERNAL STRUCTURES
 * ============================================================ */

struct cf_uart_handle_s {
    cf_uart_config_t config;        // Configuration
    void* platform_data;            // Platform-specific data
    cf_uart_error_t last_error;     // Last error
};

/* Static pool for UART handles (no heap allocation) */
#define MAX_UART_HANDLES 8
static struct cf_uart_handle_s g_uart_pool[MAX_UART_HANDLES];
static uint8_t g_uart_used[MAX_UART_HANDLES] = {0};

/* ============================================================
 * HELPER FUNCTIONS
 * ============================================================ */

/**
 * @brief Allocate UART handle from static pool
 */
static struct cf_uart_handle_s* alloc_uart_handle(void)
{
    for (int i = 0; i < MAX_UART_HANDLES; i++) {
        if (g_uart_used[i] == 0) {
            g_uart_used[i] = 1;
            memset(&g_uart_pool[i], 0, sizeof(struct cf_uart_handle_s));
            return &g_uart_pool[i];
        }
    }
    return NULL;  // Pool exhausted
}

/**
 * @brief Free UART handle back to pool
 */
static void free_uart_handle(struct cf_uart_handle_s* handle)
{
    if (handle == NULL) return;

    for (int i = 0; i < MAX_UART_HANDLES; i++) {
        if (&g_uart_pool[i] == handle) {
            g_uart_used[i] = 0;
            memset(handle, 0, sizeof(struct cf_uart_handle_s));
            return;
        }
    }
}

/**
 * @brief Validate UART configuration
 */
static bool validate_config(const cf_uart_config_t* config)
{
    if (config == NULL) return false;

    // Check instance (0-7 for most MCUs)
    if (config->instance > 7) {
        return false;
    }

    // Check baudrate
    if (config->baudrate == 0 || config->baudrate > 10000000) {
        return false;
    }

    // Check word length
    if (config->word_length > CF_UART_WORDLENGTH_9B) {
        return false;
    }

    // Check stop bits
    if (config->stop_bits > CF_UART_STOPBITS_2) {
        return false;
    }

    // Check parity
    if (config->parity > CF_UART_PARITY_ODD) {
        return false;
    }

    // Check hardware flow control
    if (config->hw_flow_control > CF_UART_HWCONTROL_RTS_CTS) {
        return false;
    }

    // Check mode
    if (config->mode > CF_UART_MODE_DMA) {
        return false;
    }

    // If using IT or DMA mode, callback must be provided
    if ((config->mode == CF_UART_MODE_INTERRUPT || config->mode == CF_UART_MODE_DMA)
        && config->callback == NULL) {
        return false;
    }

    return true;
}/* ============================================================
 * PUBLIC API IMPLEMENTATION
 * ============================================================ */

void cf_uart_config_default(cf_uart_config_t* config)
{
    if (config == NULL) return;

    config->instance = 0;  // Default to UART1
    config->baudrate = CF_UART_BAUD_115200;
    config->word_length = CF_UART_WORDLENGTH_8B;
    config->stop_bits = CF_UART_STOPBITS_1;
    config->parity = CF_UART_PARITY_NONE;
    config->hw_flow_control = CF_UART_HWCONTROL_NONE;
    config->mode = CF_UART_MODE_BLOCKING;
    config->callback = NULL;
    config->user_data = NULL;
}

cf_status_t cf_uart_init(cf_uart_handle_t* handle, const cf_uart_config_t* config)
{
    CF_PTR_CHECK(handle);
    CF_PTR_CHECK(config);

    // Validate configuration
    if (!validate_config(config)) {
        return CF_ERROR_INVALID_PARAM;
    }

    // Allocate handle
    struct cf_uart_handle_s* uart = alloc_uart_handle();
    if (uart == NULL) {
        return CF_ERROR_NO_MEMORY;
    }

    // Store configuration
    memcpy(&uart->config, config, sizeof(cf_uart_config_t));
    uart->last_error = CF_UART_ERROR_NONE;

    // Call platform-specific initialization
    cf_status_t status = cf_uart_port_init(uart, config);
    if (status != CF_OK) {
        free_uart_handle(uart);
        return status;
    }

    *handle = uart;
    return CF_OK;
}

cf_status_t cf_uart_deinit(cf_uart_handle_t handle)
{
    CF_PTR_CHECK(handle);

    // Call platform-specific deinitialization
    cf_status_t status = cf_uart_port_deinit(handle);

    // Free handle
    free_uart_handle(handle);

    return status;
}

cf_status_t cf_uart_transmit(cf_uart_handle_t handle, const uint8_t* data, uint16_t size, uint32_t timeout_ms)
{
    CF_PTR_CHECK(handle);
    CF_PTR_CHECK(data);

    if (size == 0) {
        return CF_ERROR_INVALID_PARAM;
    }

    return cf_uart_port_transmit(handle, data, size, timeout_ms);
}

cf_status_t cf_uart_receive(cf_uart_handle_t handle, uint8_t* data, uint16_t size, uint32_t timeout_ms)
{
    CF_PTR_CHECK(handle);
    CF_PTR_CHECK(data);

    if (size == 0) {
        return CF_ERROR_INVALID_PARAM;
    }

    return cf_uart_port_receive(handle, data, size, timeout_ms);
}

cf_status_t cf_uart_transmit_it(cf_uart_handle_t handle, const uint8_t* data, uint16_t size)
{
    CF_PTR_CHECK(handle);
    CF_PTR_CHECK(data);

    if (size == 0) {
        return CF_ERROR_INVALID_PARAM;
    }

    return cf_uart_port_transmit_it(handle, data, size);
}

cf_status_t cf_uart_receive_it(cf_uart_handle_t handle, uint8_t* data, uint16_t size)
{
    CF_PTR_CHECK(handle);
    CF_PTR_CHECK(data);

    if (size == 0) {
        return CF_ERROR_INVALID_PARAM;
    }

    return cf_uart_port_receive_it(handle, data, size);
}

cf_status_t cf_uart_transmit_dma(cf_uart_handle_t handle, const uint8_t* data, uint16_t size)
{
    CF_PTR_CHECK(handle);
    CF_PTR_CHECK(data);

    if (size == 0) {
        return CF_ERROR_INVALID_PARAM;
    }

    return cf_uart_port_transmit_dma(handle, data, size);
}

cf_status_t cf_uart_receive_dma(cf_uart_handle_t handle, uint8_t* data, uint16_t size)
{
    CF_PTR_CHECK(handle);
    CF_PTR_CHECK(data);

    if (size == 0) {
        return CF_ERROR_INVALID_PARAM;
    }

    return cf_uart_port_receive_dma(handle, data, size);
}

cf_status_t cf_uart_abort_transmit(cf_uart_handle_t handle)
{
    CF_PTR_CHECK(handle);
    return cf_uart_port_abort_transmit(handle);
}

cf_status_t cf_uart_abort_receive(cf_uart_handle_t handle)
{
    CF_PTR_CHECK(handle);
    return cf_uart_port_abort_receive(handle);
}

cf_status_t cf_uart_get_error(cf_uart_handle_t handle, cf_uart_error_t* error)
{
    CF_PTR_CHECK(handle);
    CF_PTR_CHECK(error);

    *error = handle->last_error;
    return CF_OK;
}

cf_status_t cf_uart_receive_to_idle_dma(cf_uart_handle_t handle, uint8_t* data, uint16_t size)
{
    CF_PTR_CHECK(handle);
    CF_PTR_CHECK(data);

    if (size == 0) {
        return CF_ERROR_INVALID_PARAM;
    }

    return cf_uart_port_receive_to_idle_dma(handle, data, size);
}

cf_status_t cf_uart_get_received_count(cf_uart_handle_t handle, uint16_t* count)
{
    CF_PTR_CHECK(handle);
    CF_PTR_CHECK(count);

    return cf_uart_port_get_received_count(handle, count);
}
