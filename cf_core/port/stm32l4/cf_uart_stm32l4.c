/**
 * @file cf_uart_stm32l4.c
 * @brief UART platform implementation for STM32L4
 * @version 1.0.0
 * @date 2025-11-04
 *
 * @copyright Copyright (c) 2025 CFramework
 * Licensed under MIT License
 *
 * STM32L4 UART driver implementation using HAL
 * - Supports USART1-5, LPUART1
 * - Blocking, interrupt, and DMA modes
 * - Static memory allocation
 */

#include "hal/cf_uart.h"
#include "hal/cf_uart_port.h"
#include "stm32l4xx_hal.h"
#include <string.h>

/* ============================================================
 * PLATFORM DATA STRUCTURE
 * ============================================================ */

typedef struct {
    UART_HandleTypeDef* hal_uart;       // STM32 HAL UART handle
    cf_uart_callback_t callback;        // User callback
    void* user_data;                    // User context
    uint16_t rx_buffer_size;            // RX buffer size (for IDLE mode)
    uint16_t rx_received_count;         // Bytes received (updated in IDLE callback)
} stm32l4_uart_data_t;

/* Static pool for platform data */
#define MAX_UART_PLATFORM_DATA 8
static stm32l4_uart_data_t g_platform_data_pool[MAX_UART_PLATFORM_DATA];
static uint8_t g_platform_data_used[MAX_UART_PLATFORM_DATA] = {0};

/* Lookup table: HAL UART handle -> platform data (for callbacks) */
static stm32l4_uart_data_t* g_uart_callback_table[MAX_UART_PLATFORM_DATA] = {NULL};

/* ============================================================
 * HELPER FUNCTIONS
 * ============================================================ */

/**
 * @brief Map UART instance to STM32 HAL handle
 * NOTE: User must define these extern handles in their application
 */
static UART_HandleTypeDef* get_uart_handle(uint32_t instance)
{
    // Only UART2 and UART3 are available in this project
    extern UART_HandleTypeDef huart2;
    extern UART_HandleTypeDef huart3;

    switch (instance) {
        case 0: return &huart2;     // UART1 → UART2 (remap)
        case 1: return &huart2;     // UART2
        case 2: return &huart3;     // UART3
        case 3: return &huart3;     // UART4 → UART3 (remap)
        case 4: return &huart3;     // UART5 → UART3 (remap)
        case 5: return &huart3;     // LPUART1 → UART3 (remap)
        default: return NULL;
    }
}

/**
 * @brief Allocate platform data from static pool
 */
static stm32l4_uart_data_t* alloc_platform_data(void)
{
    for (int i = 0; i < MAX_UART_PLATFORM_DATA; i++) {
        if (g_platform_data_used[i] == 0) {
            g_platform_data_used[i] = 1;
            memset(&g_platform_data_pool[i], 0, sizeof(stm32l4_uart_data_t));
            return &g_platform_data_pool[i];
        }
    }
    return NULL;
}

/**
 * @brief Free platform data back to pool
 */
static void free_platform_data(stm32l4_uart_data_t* pdata)
{
    if (pdata == NULL) return;

    for (int i = 0; i < MAX_UART_PLATFORM_DATA; i++) {
        if (&g_platform_data_pool[i] == pdata) {
            // Remove from callback table
            g_uart_callback_table[i] = NULL;

            g_platform_data_used[i] = 0;
            memset(pdata, 0, sizeof(stm32l4_uart_data_t));
            return;
        }
    }
}

/**
 * @brief Find platform data by HAL UART handle
 */
static stm32l4_uart_data_t* find_platform_data(UART_HandleTypeDef* hal_uart)
{
    for (int i = 0; i < MAX_UART_PLATFORM_DATA; i++) {
        if (g_uart_callback_table[i] != NULL &&
            g_uart_callback_table[i]->hal_uart == hal_uart) {
            return g_uart_callback_table[i];
        }
    }
    return NULL;
}

/**
 * @brief Convert CFramework word length to STM32 HAL
 */
static uint32_t convert_word_length(cf_uart_wordlength_t word_length)
{
    switch (word_length) {
        case CF_UART_WORDLENGTH_7B: return UART_WORDLENGTH_7B;
        case CF_UART_WORDLENGTH_8B: return UART_WORDLENGTH_8B;
        case CF_UART_WORDLENGTH_9B: return UART_WORDLENGTH_9B;
        default: return UART_WORDLENGTH_8B;
    }
}

/**
 * @brief Convert CFramework stop bits to STM32 HAL
 */
static uint32_t convert_stop_bits(cf_uart_stopbits_t stop_bits)
{
    switch (stop_bits) {
        case CF_UART_STOPBITS_0_5: return UART_STOPBITS_0_5;
        case CF_UART_STOPBITS_1:   return UART_STOPBITS_1;
        case CF_UART_STOPBITS_1_5: return UART_STOPBITS_1_5;
        case CF_UART_STOPBITS_2:   return UART_STOPBITS_2;
        default: return UART_STOPBITS_1;
    }
}

/**
 * @brief Convert CFramework parity to STM32 HAL
 */
static uint32_t convert_parity(cf_uart_parity_t parity)
{
    switch (parity) {
        case CF_UART_PARITY_NONE: return UART_PARITY_NONE;
        case CF_UART_PARITY_EVEN: return UART_PARITY_EVEN;
        case CF_UART_PARITY_ODD:  return UART_PARITY_ODD;
        default: return UART_PARITY_NONE;
    }
}

/**
 * @brief Convert CFramework hardware flow control to STM32 HAL
 */
static uint32_t convert_hw_flow_control(cf_uart_hwcontrol_t hw_flow_control)
{
    switch (hw_flow_control) {
        case CF_UART_HWCONTROL_NONE:    return UART_HWCONTROL_NONE;
        case CF_UART_HWCONTROL_RTS:     return UART_HWCONTROL_RTS;
        case CF_UART_HWCONTROL_CTS:     return UART_HWCONTROL_CTS;
        case CF_UART_HWCONTROL_RTS_CTS: return UART_HWCONTROL_RTS_CTS;
        default: return UART_HWCONTROL_NONE;
    }
}

/**
 * @brief Convert HAL status to CFramework status
 */
static cf_status_t convert_hal_status(HAL_StatusTypeDef hal_status)
{
    switch (hal_status) {
        case HAL_OK:      return CF_OK;
        case HAL_ERROR:   return CF_ERROR_HARDWARE;
        case HAL_BUSY:    return CF_ERROR_BUSY;
        case HAL_TIMEOUT: return CF_ERROR_TIMEOUT;
        default:          return CF_ERROR_HARDWARE;
    }
}

/**
 * @brief Convert HAL error to CFramework error
 */
static cf_uart_error_t convert_hal_error(uint32_t hal_error)
{
    cf_uart_error_t error = CF_UART_ERROR_NONE;

    if (hal_error & HAL_UART_ERROR_PE)  error |= CF_UART_ERROR_PARITY;
    if (hal_error & HAL_UART_ERROR_NE)  error |= CF_UART_ERROR_NOISE;
    if (hal_error & HAL_UART_ERROR_FE)  error |= CF_UART_ERROR_FRAME;
    if (hal_error & HAL_UART_ERROR_ORE) error |= CF_UART_ERROR_OVERRUN;
    if (hal_error & HAL_UART_ERROR_DMA) error |= CF_UART_ERROR_DMA;

    return error;
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
    stm32l4_uart_data_t* pdata = alloc_platform_data();
    if (pdata == NULL) {
        return CF_ERROR_NO_MEMORY;
    }

    // Get HAL UART handle based on instance
    UART_HandleTypeDef* hal_uart = get_uart_handle(config->instance);
    if (hal_uart == NULL) {
        free_platform_data(pdata);
        return CF_ERROR_INVALID_PARAM;
    }

    // Configure HAL UART
    hal_uart->Init.BaudRate = config->baudrate;
    hal_uart->Init.WordLength = convert_word_length(config->word_length);
    hal_uart->Init.StopBits = convert_stop_bits(config->stop_bits);
    hal_uart->Init.Parity = convert_parity(config->parity);
    hal_uart->Init.Mode = UART_MODE_TX_RX;
    hal_uart->Init.HwFlowCtl = convert_hw_flow_control(config->hw_flow_control);
    hal_uart->Init.OverSampling = UART_OVERSAMPLING_16;

    // Initialize HAL UART
    HAL_StatusTypeDef status = HAL_UART_Init(hal_uart);
    if (status != HAL_OK) {
        free_platform_data(pdata);
        return convert_hal_status(status);
    }

    // Store platform data
    pdata->hal_uart = hal_uart;
    pdata->callback = config->callback;
    pdata->user_data = config->user_data;

    // Register in callback table
    for (int i = 0; i < MAX_UART_PLATFORM_DATA; i++) {
        if (g_uart_callback_table[i] == NULL) {
            g_uart_callback_table[i] = pdata;
            break;
        }
    }

    // Update handle's platform data
    cf_uart_set_platform_data(handle, pdata);

    return CF_OK;
}

cf_status_t cf_uart_port_deinit(cf_uart_handle_t handle)
{
    if (handle == NULL) {
        return CF_ERROR_INVALID_PARAM;
    }

    stm32l4_uart_data_t* pdata = (stm32l4_uart_data_t*)cf_uart_get_platform_data(handle);
    if (pdata == NULL) {
        return CF_ERROR_INVALID_PARAM;
    }

    // Deinitialize HAL UART
    HAL_UART_DeInit(pdata->hal_uart);

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

    stm32l4_uart_data_t* pdata = (stm32l4_uart_data_t*)cf_uart_get_platform_data(handle);
    if (pdata == NULL) {
        return CF_ERROR_INVALID_PARAM;
    }

    HAL_StatusTypeDef status = HAL_UART_Transmit(pdata->hal_uart, (uint8_t*)data, size, timeout_ms);

    return convert_hal_status(status);
}

cf_status_t cf_uart_port_receive(cf_uart_handle_t handle, uint8_t* data, uint16_t size, uint32_t timeout_ms)
{
    if (handle == NULL || data == NULL) {
        return CF_ERROR_INVALID_PARAM;
    }

    stm32l4_uart_data_t* pdata = (stm32l4_uart_data_t*)cf_uart_get_platform_data(handle);
    if (pdata == NULL) {
        return CF_ERROR_INVALID_PARAM;
    }

    HAL_StatusTypeDef status = HAL_UART_Receive(pdata->hal_uart, data, size, timeout_ms);

    return convert_hal_status(status);
}

cf_status_t cf_uart_port_transmit_it(cf_uart_handle_t handle, const uint8_t* data, uint16_t size)
{
    if (handle == NULL || data == NULL) {
        return CF_ERROR_INVALID_PARAM;
    }

    stm32l4_uart_data_t* pdata = (stm32l4_uart_data_t*)cf_uart_get_platform_data(handle);
    if (pdata == NULL) {
        return CF_ERROR_INVALID_PARAM;
    }

    HAL_StatusTypeDef status = HAL_UART_Transmit_IT(pdata->hal_uart, (uint8_t*)data, size);

    return convert_hal_status(status);
}

cf_status_t cf_uart_port_receive_it(cf_uart_handle_t handle, uint8_t* data, uint16_t size)
{
    if (handle == NULL || data == NULL) {
        return CF_ERROR_INVALID_PARAM;
    }

    stm32l4_uart_data_t* pdata = (stm32l4_uart_data_t*)cf_uart_get_platform_data(handle);
    if (pdata == NULL) {
        return CF_ERROR_INVALID_PARAM;
    }

    HAL_StatusTypeDef status = HAL_UART_Receive_IT(pdata->hal_uart, data, size);

    return convert_hal_status(status);
}

cf_status_t cf_uart_port_transmit_dma(cf_uart_handle_t handle, const uint8_t* data, uint16_t size)
{
    if (handle == NULL || data == NULL) {
        return CF_ERROR_INVALID_PARAM;
    }

    stm32l4_uart_data_t* pdata = (stm32l4_uart_data_t*)cf_uart_get_platform_data(handle);
    if (pdata == NULL) {
        return CF_ERROR_INVALID_PARAM;
    }

    HAL_StatusTypeDef status = HAL_UART_Transmit_DMA(pdata->hal_uart, (uint8_t*)data, size);

    return convert_hal_status(status);
}

cf_status_t cf_uart_port_receive_dma(cf_uart_handle_t handle, uint8_t* data, uint16_t size)
{
    if (handle == NULL || data == NULL) {
        return CF_ERROR_INVALID_PARAM;
    }

    stm32l4_uart_data_t* pdata = (stm32l4_uart_data_t*)cf_uart_get_platform_data(handle);
    if (pdata == NULL) {
        return CF_ERROR_INVALID_PARAM;
    }

    HAL_StatusTypeDef status = HAL_UART_Receive_DMA(pdata->hal_uart, data, size);

    return convert_hal_status(status);
}

cf_status_t cf_uart_port_abort_transmit(cf_uart_handle_t handle)
{
    if (handle == NULL) {
        return CF_ERROR_INVALID_PARAM;
    }

    stm32l4_uart_data_t* pdata = (stm32l4_uart_data_t*)cf_uart_get_platform_data(handle);
    if (pdata == NULL) {
        return CF_ERROR_INVALID_PARAM;
    }

    HAL_StatusTypeDef status = HAL_UART_AbortTransmit(pdata->hal_uart);

    return convert_hal_status(status);
}

cf_status_t cf_uart_port_abort_receive(cf_uart_handle_t handle)
{
    if (handle == NULL) {
        return CF_ERROR_INVALID_PARAM;
    }

    stm32l4_uart_data_t* pdata = (stm32l4_uart_data_t*)cf_uart_get_platform_data(handle);
    if (pdata == NULL) {
        return CF_ERROR_INVALID_PARAM;
    }

    HAL_StatusTypeDef status = HAL_UART_AbortReceive(pdata->hal_uart);

    return convert_hal_status(status);
}

cf_status_t cf_uart_port_get_error(cf_uart_handle_t handle, cf_uart_error_t* error)
{
    if (handle == NULL || error == NULL) {
        return CF_ERROR_INVALID_PARAM;
    }

    stm32l4_uart_data_t* pdata = (stm32l4_uart_data_t*)cf_uart_get_platform_data(handle);
    if (pdata == NULL) {
        return CF_ERROR_INVALID_PARAM;
    }

    uint32_t hal_error = HAL_UART_GetError(pdata->hal_uart);
    *error = convert_hal_error(hal_error);

    return CF_OK;
}

/* ============================================================
 * HAL CALLBACKS (called by STM32 HAL)
 * ============================================================ */

/**
 * @brief TX complete callback from HAL
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    stm32l4_uart_data_t* pdata = find_platform_data(huart);
    if (pdata != NULL && pdata->callback != NULL) {
        pdata->callback(NULL, CF_UART_EVENT_TX_COMPLETE, pdata->user_data);
    }
}

/**
 * @brief RX complete callback from HAL
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    stm32l4_uart_data_t* pdata = find_platform_data(huart);
    if (pdata != NULL && pdata->callback != NULL) {
        pdata->callback(NULL, CF_UART_EVENT_RX_COMPLETE, pdata->user_data);
    }
}

/**
 * @brief Error callback from HAL
 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    stm32l4_uart_data_t* pdata = find_platform_data(huart);
    if (pdata != NULL && pdata->callback != NULL) {
        pdata->callback(NULL, CF_UART_EVENT_ERROR, pdata->user_data);
    }
}

/**
 * @brief RX Event callback from HAL (IDLE detection)
 * NOTE: Available on STM32L4+ with HAL_UARTEx_ReceiveToIdle_DMA
 */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    stm32l4_uart_data_t* pdata = find_platform_data(huart);
    if (pdata != NULL) {
        // Store received count
        pdata->rx_received_count = Size;

        // Invoke user callback with IDLE event
        if (pdata->callback != NULL) {
            pdata->callback(NULL, CF_UART_EVENT_IDLE, pdata->user_data);
        }
    }
}

/* ============================================================
 * EXTENDED API IMPLEMENTATION (IDLE + DMA)
 * ============================================================ */

cf_status_t cf_uart_port_receive_to_idle_dma(cf_uart_handle_t handle, uint8_t* data, uint16_t size)
{
    if (handle == NULL || data == NULL) {
        return CF_ERROR_INVALID_PARAM;
    }

    stm32l4_uart_data_t* pdata = (stm32l4_uart_data_t*)cf_uart_get_platform_data(handle);
    if (pdata == NULL) {
        return CF_ERROR_INVALID_PARAM;
    }

    // Store buffer info
    pdata->rx_buffer_size = size;
    pdata->rx_received_count = 0;

    // Start UART RX with IDLE detection using DMA
    HAL_StatusTypeDef status = HAL_UARTEx_ReceiveToIdle_DMA(pdata->hal_uart, data, size);

    // Disable Half Transfer interrupt (important!)
    // Prevents HalfReception callback which would trigger IDLE event prematurely
    if (pdata->hal_uart->hdmarx != NULL) {
        __HAL_DMA_DISABLE_IT(pdata->hal_uart->hdmarx, DMA_IT_HT);
    }

    return convert_hal_status(status);
}

cf_status_t cf_uart_port_get_received_count(cf_uart_handle_t handle, uint16_t* count)
{
    if (handle == NULL || count == NULL) {
        return CF_ERROR_INVALID_PARAM;
    }

    stm32l4_uart_data_t* pdata = (stm32l4_uart_data_t*)cf_uart_get_platform_data(handle);
    if (pdata == NULL) {
        return CF_ERROR_INVALID_PARAM;
    }

    *count = pdata->rx_received_count;
    return CF_OK;
}
