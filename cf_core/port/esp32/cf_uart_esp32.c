/**
 * @file cf_uart_esp32.c
 * @brief UART platform implementation for ESP32
 * @version 1.0.0
 * @date 2025-11-04
 *
 * @copyright Copyright (c) 2025 CFramework
 * Licensed under MIT License
 *
 * ESP32 UART driver implementation using ESP-IDF
 * - Supports UART0-2
 * - Blocking, interrupt modes (DMA not applicable on ESP32)
 * - Static memory allocation
 */

#include "hal/cf_uart.h"
#include "hal/cf_uart_port.h"
#include "utils/cf_log.h"
#include "driver/uart.h"
#include <string.h>

/* ============================================================
 * PLATFORM DATA STRUCTURE
 * ============================================================ */

typedef struct {
    uart_port_t uart_num;               // ESP32 UART number (0-2)
    cf_uart_callback_t callback;        // User callback
    void* user_data;                    // User context
    QueueHandle_t uart_queue;           // UART event queue (for IT mode)
} esp32_uart_data_t;

/* Static pool for platform data */
#define MAX_UART_PLATFORM_DATA 3  // ESP32 has 3 UARTs
static esp32_uart_data_t g_platform_data_pool[MAX_UART_PLATFORM_DATA];
static uint8_t g_platform_data_used[MAX_UART_PLATFORM_DATA] = {0};

/* ============================================================
 * HELPER FUNCTIONS
 * ============================================================ */

/**
 * @brief Allocate platform data from static pool
 */
static esp32_uart_data_t* alloc_platform_data(void)
{
    for (int i = 0; i < MAX_UART_PLATFORM_DATA; i++) {
        if (g_platform_data_used[i] == 0) {
            g_platform_data_used[i] = 1;
            memset(&g_platform_data_pool[i], 0, sizeof(esp32_uart_data_t));
            return &g_platform_data_pool[i];
        }
    }
    return NULL;
}

/**
 * @brief Free platform data back to pool
 */
static void free_platform_data(esp32_uart_data_t* pdata)
{
    if (pdata == NULL) return;

    for (int i = 0; i < MAX_UART_PLATFORM_DATA; i++) {
        if (&g_platform_data_pool[i] == pdata) {
            g_platform_data_used[i] = 0;
            memset(pdata, 0, sizeof(esp32_uart_data_t));
            return;
        }
    }
}

/**
 * @brief Convert CFramework word length to ESP32
 */
static uart_word_length_t convert_word_length(cf_uart_wordlength_t word_length)
{
    switch (word_length) {
        case CF_UART_WORDLENGTH_7B: return UART_DATA_7_BITS;
        case CF_UART_WORDLENGTH_8B: return UART_DATA_8_BITS;
        default: return UART_DATA_8_BITS;  // ESP32 doesn't support 9-bit
    }
}

/**
 * @brief Convert CFramework stop bits to ESP32
 */
static uart_stop_bits_t convert_stop_bits(cf_uart_stopbits_t stop_bits)
{
    switch (stop_bits) {
        case CF_UART_STOPBITS_1:   return UART_STOP_BITS_1;
        case CF_UART_STOPBITS_1_5: return UART_STOP_BITS_1_5;
        case CF_UART_STOPBITS_2:   return UART_STOP_BITS_2;
        default: return UART_STOP_BITS_1;
    }
}

/**
 * @brief Convert CFramework parity to ESP32
 */
static uart_parity_t convert_parity(cf_uart_parity_t parity)
{
    switch (parity) {
        case CF_UART_PARITY_NONE: return UART_PARITY_DISABLE;
        case CF_UART_PARITY_EVEN: return UART_PARITY_EVEN;
        case CF_UART_PARITY_ODD:  return UART_PARITY_ODD;
        default: return UART_PARITY_DISABLE;
    }
}

/**
 * @brief Convert CFramework hardware flow control to ESP32
 */
static uart_hw_flowcontrol_t convert_hw_flow_control(cf_uart_hwcontrol_t hw_flow_control)
{
    switch (hw_flow_control) {
        case CF_UART_HWCONTROL_NONE:    return UART_HW_FLOWCTRL_DISABLE;
        case CF_UART_HWCONTROL_RTS:     return UART_HW_FLOWCTRL_RTS;
        case CF_UART_HWCONTROL_CTS:     return UART_HW_FLOWCTRL_CTS;
        case CF_UART_HWCONTROL_RTS_CTS: return UART_HW_FLOWCTRL_CTS_RTS;
        default: return UART_HW_FLOWCTRL_DISABLE;
    }
}

/**
 * @brief Convert ESP-IDF error to CFramework status
 */
static cf_status_t convert_esp_error(esp_err_t err)
{
    switch (err) {
        case ESP_OK:           return CF_OK;
        case ESP_ERR_TIMEOUT:  return CF_ERROR_TIMEOUT;
        case ESP_ERR_NO_MEM:   return CF_ERROR_NO_MEMORY;
        case ESP_ERR_INVALID_ARG: return CF_ERROR_INVALID_PARAM;
        default:               return CF_ERROR_HARDWARE;
    }
}

/* ============================================================
 * PLATFORM INTERFACE IMPLEMENTATION
 * ============================================================ */

cf_status_t cf_uart_port_init(cf_uart_handle_t handle, const cf_uart_config_t* config)
{
    if (handle == NULL || config == NULL) {
        return CF_ERROR_INVALID_PARAM;
    }

    // Allocate platform data
    esp32_uart_data_t* pdata = alloc_platform_data();
    if (pdata == NULL) {
        CF_LOG_E("Platform data pool exhausted");
        return CF_ERROR_NO_MEMORY;
    }

    // Map instance to ESP32 UART port
    uart_port_t uart_num = (uart_port_t)config->instance;
    if (uart_num >= UART_NUM_MAX) {
        free_platform_data(pdata);
        CF_LOG_E("Invalid UART instance: %lu", config->instance);
        return CF_ERROR_INVALID_PARAM;
    }

    // Configure UART
    uart_config_t uart_config = {
        .baud_rate = config->baudrate,
        .data_bits = convert_word_length(config->word_length),
        .parity = convert_parity(config->parity),
        .stop_bits = convert_stop_bits(config->stop_bits),
        .flow_ctrl = convert_hw_flow_control(config->hw_flow_control),
        .source_clk = UART_SCLK_APB,
    };

    // Install UART driver
    esp_err_t err = uart_driver_install(uart_num,
                                        1024,  // RX buffer size
                                        1024,  // TX buffer size
                                        (config->mode == CF_UART_MODE_INTERRUPT) ? 10 : 0,  // Event queue size
                                        (config->mode == CF_UART_MODE_INTERRUPT) ? &pdata->uart_queue : NULL,
                                        0);
    if (err != ESP_OK) {
        free_platform_data(pdata);
        CF_LOG_E("uart_driver_install failed: %d", err);
        return convert_esp_error(err);
    }

    // Configure UART parameters
    err = uart_param_config(uart_num, &uart_config);
    if (err != ESP_OK) {
        uart_driver_delete(uart_num);
        free_platform_data(pdata);
        CF_LOG_E("uart_param_config failed: %d", err);
        return convert_esp_error(err);
    }

    // Set default pins (user can change via esp-idf if needed)
    // UART0: TX=GPIO1, RX=GPIO3 (USB serial)
    // UART1: TX=GPIO10, RX=GPIO9 (conflicts with flash, usually not used)
    // UART2: TX=GPIO17, RX=GPIO16
    if (uart_num == UART_NUM_0) {
        uart_set_pin(uart_num, 1, 3, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    } else if (uart_num == UART_NUM_2) {
        uart_set_pin(uart_num, 17, 16, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    }

    // Store platform data
    pdata->uart_num = uart_num;
    pdata->callback = config->callback;
    pdata->user_data = config->user_data;

    // Update handle's platform data
    cf_uart_set_platform_data(handle, pdata);

    CF_LOG_I("UART%d initialized: %lu baud, %dN%d",
             uart_num, config->baudrate,
             (config->word_length == CF_UART_WORDLENGTH_8B) ? 8 : 7,
             (config->stop_bits == CF_UART_STOPBITS_1) ? 1 : 2);

    return CF_OK;
}

cf_status_t cf_uart_port_deinit(cf_uart_handle_t handle)
{
    if (handle == NULL) {
        return CF_ERROR_INVALID_PARAM;
    }

    esp32_uart_data_t* pdata = (esp32_uart_data_t*)cf_uart_get_platform_data(handle);
    if (pdata == NULL) {
        return CF_ERROR_INVALID_PARAM;
    }

    // Delete UART driver
    uart_driver_delete(pdata->uart_num);

    // Free platform data
    free_platform_data(pdata);
    cf_uart_set_platform_data(handle, NULL);

    return CF_OK;
}

cf_status_t cf_uart_port_transmit(cf_uart_handle_t handle, const uint8_t* data, uint16_t size, uint32_t timeout_ms)
{
    if (handle == NULL || data == NULL) {
        return CF_ERROR_INVALID_PARAM;
    }

    esp32_uart_data_t* pdata = (esp32_uart_data_t*)cf_uart_get_platform_data(handle);
    if (pdata == NULL) {
        return CF_ERROR_INVALID_PARAM;
    }

    int bytes_written = uart_write_bytes(pdata->uart_num, (const char*)data, size);

    if (bytes_written < 0) {
        return CF_ERROR_HARDWARE;
    }

    // Wait for TX done (optional timeout)
    esp_err_t err = uart_wait_tx_done(pdata->uart_num, pdMS_TO_TICKS(timeout_ms));

    return convert_esp_error(err);
}

cf_status_t cf_uart_port_receive(cf_uart_handle_t handle, uint8_t* data, uint16_t size, uint32_t timeout_ms)
{
    if (handle == NULL || data == NULL) {
        return CF_ERROR_INVALID_PARAM;
    }

    esp32_uart_data_t* pdata = (esp32_uart_data_t*)cf_uart_get_platform_data(handle);
    if (pdata == NULL) {
        return CF_ERROR_INVALID_PARAM;
    }

    int bytes_read = uart_read_bytes(pdata->uart_num, data, size, pdMS_TO_TICKS(timeout_ms));

    if (bytes_read < 0) {
        return CF_ERROR_HARDWARE;
    } else if (bytes_read == 0) {
        return CF_ERROR_TIMEOUT;
    }

    return CF_OK;
}

cf_status_t cf_uart_port_transmit_it(cf_uart_handle_t handle, const uint8_t* data, uint16_t size)
{
    // ESP32 UART driver already uses interrupts internally
    // Just use blocking transmit (non-blocking at driver level)
    return cf_uart_port_transmit(handle, data, size, 0);
}

cf_status_t cf_uart_port_receive_it(cf_uart_handle_t handle, uint8_t* data, uint16_t size)
{
    // ESP32 UART driver already uses interrupts internally
    // Use event queue for async operation
    return cf_uart_port_receive(handle, data, size, 0);
}

cf_status_t cf_uart_port_transmit_dma(cf_uart_handle_t handle, const uint8_t* data, uint16_t size)
{
    // ESP32 UART driver internally uses DMA-like mechanism
    return cf_uart_port_transmit(handle, data, size, 0);
}

cf_status_t cf_uart_port_receive_dma(cf_uart_handle_t handle, uint8_t* data, uint16_t size)
{
    // ESP32 UART driver internally uses DMA-like mechanism
    return cf_uart_port_receive(handle, data, size, 0);
}

cf_status_t cf_uart_port_abort_transmit(cf_uart_handle_t handle)
{
    if (handle == NULL) {
        return CF_ERROR_INVALID_PARAM;
    }

    esp32_uart_data_t* pdata = (esp32_uart_data_t*)cf_uart_get_platform_data(handle);
    if (pdata == NULL) {
        return CF_ERROR_INVALID_PARAM;
    }

    // Flush TX buffer
    esp_err_t err = uart_flush_input(pdata->uart_num);

    return convert_esp_error(err);
}

cf_status_t cf_uart_port_abort_receive(cf_uart_handle_t handle)
{
    if (handle == NULL) {
        return CF_ERROR_INVALID_PARAM;
    }

    esp32_uart_data_t* pdata = (esp32_uart_data_t*)cf_uart_get_platform_data(handle);
    if (pdata == NULL) {
        return CF_ERROR_INVALID_PARAM;
    }

    // Flush RX buffer
    esp_err_t err = uart_flush(pdata->uart_num);

    return convert_esp_error(err);
}

cf_status_t cf_uart_port_get_error(cf_uart_handle_t handle, cf_uart_error_t* error)
{
    if (handle == NULL || error == NULL) {
        return CF_ERROR_INVALID_PARAM;
    }

    // ESP32 UART driver doesn't expose error flags directly
    // Would need to implement error tracking in event handler
    *error = CF_UART_ERROR_NONE;

    return CF_OK;
}

cf_status_t cf_uart_port_receive_to_idle_dma(cf_uart_handle_t handle, uint8_t* data, uint16_t size)
{
    // ESP32 UART driver doesn't have direct IDLE detection like STM32
    // But the event queue mechanism provides similar functionality
    // Just use normal DMA receive (driver handles it internally)
    return cf_uart_port_receive_dma(handle, data, size);
}

cf_status_t cf_uart_port_get_received_count(cf_uart_handle_t handle, uint16_t* count)
{
    if (handle == NULL || count == NULL) {
        return CF_ERROR_INVALID_PARAM;
    }

    esp32_uart_data_t* pdata = (esp32_uart_data_t*)cf_uart_get_platform_data(handle);
    if (pdata == NULL) {
        return CF_ERROR_INVALID_PARAM;
    }

    // Get bytes available in RX buffer
    size_t available = 0;
    uart_get_buffered_data_len(pdata->uart_num, &available);
    *count = (uint16_t)available;

    return CF_OK;
}
