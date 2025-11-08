/**
 * @file cf_uart.h
 * @brief UART HAL abstraction layer for CFramework
 * @version 1.0.0
 * @date 2025-11-04
 *
 * @copyright Copyright (c) 2025 CFramework
 * Licensed under MIT License
 *
 * Platform-independent UART driver interface
 * Supports blocking, interrupt, and DMA modes
 */

#ifndef CF_UART_H
#define CF_UART_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cf_common.h"

/* ============================================================
 * TYPE DEFINITIONS
 * ============================================================ */

/**
 * @brief Opaque UART handle
 */
typedef struct cf_uart_handle_s* cf_uart_handle_t;

/**
 * @brief UART baudrate presets
 */
typedef enum {
    CF_UART_BAUD_9600    = 9600,
    CF_UART_BAUD_19200   = 19200,
    CF_UART_BAUD_38400   = 38400,
    CF_UART_BAUD_57600   = 57600,
    CF_UART_BAUD_115200  = 115200,
    CF_UART_BAUD_230400  = 230400,
    CF_UART_BAUD_460800  = 460800,
    CF_UART_BAUD_921600  = 921600
} cf_uart_baudrate_t;

/**
 * @brief UART word length
 */
typedef enum {
    CF_UART_WORDLENGTH_7B = 0,
    CF_UART_WORDLENGTH_8B = 1,
    CF_UART_WORDLENGTH_9B = 2
} cf_uart_wordlength_t;

/**
 * @brief UART stop bits
 */
typedef enum {
    CF_UART_STOPBITS_0_5 = 0,
    CF_UART_STOPBITS_1   = 1,
    CF_UART_STOPBITS_1_5 = 2,
    CF_UART_STOPBITS_2   = 3
} cf_uart_stopbits_t;

/**
 * @brief UART parity
 */
typedef enum {
    CF_UART_PARITY_NONE = 0,
    CF_UART_PARITY_EVEN = 1,
    CF_UART_PARITY_ODD  = 2
} cf_uart_parity_t;

/**
 * @brief UART hardware flow control
 */
typedef enum {
    CF_UART_HWCONTROL_NONE    = 0,
    CF_UART_HWCONTROL_RTS     = 1,
    CF_UART_HWCONTROL_CTS     = 2,
    CF_UART_HWCONTROL_RTS_CTS = 3
} cf_uart_hwcontrol_t;

/**
 * @brief UART transfer mode
 */
typedef enum {
    CF_UART_MODE_BLOCKING   = 0,  // Polling mode
    CF_UART_MODE_INTERRUPT  = 1,  // Interrupt mode
    CF_UART_MODE_DMA        = 2   // DMA mode
} cf_uart_mode_t;

/**
 * @brief UART event types for callbacks
 */
typedef enum {
    CF_UART_EVENT_TX_COMPLETE,    // Transmit complete
    CF_UART_EVENT_RX_COMPLETE,    // Receive complete
    CF_UART_EVENT_ERROR,          // Error occurred
    CF_UART_EVENT_IDLE            // IDLE line detected
} cf_uart_event_t;

/**
 * @brief UART error flags
 */
typedef enum {
    CF_UART_ERROR_NONE    = 0x00,
    CF_UART_ERROR_PARITY  = 0x01,
    CF_UART_ERROR_NOISE   = 0x02,
    CF_UART_ERROR_FRAME   = 0x04,
    CF_UART_ERROR_OVERRUN = 0x08,
    CF_UART_ERROR_DMA     = 0x10
} cf_uart_error_t;

/**
 * @brief UART event callback function type
 *
 * @param handle UART handle
 * @param event Event type
 * @param user_data User-provided context
 */
typedef void (*cf_uart_callback_t)(cf_uart_handle_t handle, cf_uart_event_t event, void* user_data);

/**
 * @brief UART configuration structure
 */
typedef struct {
    uint32_t instance;                  // UART instance (0=UART1, 1=UART2, etc.)
    uint32_t baudrate;                  // Baudrate (e.g., 115200)
    cf_uart_wordlength_t word_length;   // Word length
    cf_uart_stopbits_t stop_bits;       // Stop bits
    cf_uart_parity_t parity;            // Parity
    cf_uart_hwcontrol_t hw_flow_control; // Hardware flow control
    cf_uart_mode_t mode;                // Transfer mode (blocking/IT/DMA)
    cf_uart_callback_t callback;        // Event callback (for IT/DMA modes)
    void* user_data;                    // User context for callback
} cf_uart_config_t;

/* ============================================================
 * PUBLIC API
 * ============================================================ */

/**
 * @brief Set default UART configuration
 *
 * Default: 115200 8N1, no flow control, blocking mode
 *
 * @param[out] config Configuration structure to initialize
 */
void cf_uart_config_default(cf_uart_config_t* config);

/**
 * @brief Initialize UART peripheral
 *
 * @param[out] handle Pointer to receive UART handle
 * @param[in] config UART configuration
 *
 * @return CF_OK on success
 * @return CF_ERROR_NULL_POINTER if handle or config is NULL
 * @return CF_ERROR_INVALID_PARAM if configuration is invalid
 * @return CF_ERROR_HARDWARE if hardware initialization failed
 */
cf_status_t cf_uart_init(cf_uart_handle_t* handle, const cf_uart_config_t* config);

/**
 * @brief Deinitialize UART peripheral
 *
 * @param handle UART handle
 *
 * @return CF_OK on success
 * @return CF_ERROR_NULL_POINTER if handle is NULL
 */
cf_status_t cf_uart_deinit(cf_uart_handle_t handle);

/**
 * @brief Transmit data (blocking mode)
 *
 * @param handle UART handle
 * @param data Pointer to data buffer
 * @param size Number of bytes to transmit
 * @param timeout_ms Timeout in milliseconds (0 = no timeout)
 *
 * @return CF_OK on success
 * @return CF_ERROR_NULL_POINTER if handle or data is NULL
 * @return CF_ERROR_TIMEOUT if timeout occurred
 * @return CF_ERROR_HARDWARE if transmission error
 */
cf_status_t cf_uart_transmit(cf_uart_handle_t handle, const uint8_t* data, uint16_t size, uint32_t timeout_ms);

/**
 * @brief Receive data (blocking mode)
 *
 * @param handle UART handle
 * @param data Pointer to receive buffer
 * @param size Number of bytes to receive
 * @param timeout_ms Timeout in milliseconds (0 = no timeout)
 *
 * @return CF_OK on success
 * @return CF_ERROR_NULL_POINTER if handle or data is NULL
 * @return CF_ERROR_TIMEOUT if timeout occurred
 * @return CF_ERROR_HARDWARE if reception error
 */
cf_status_t cf_uart_receive(cf_uart_handle_t handle, uint8_t* data, uint16_t size, uint32_t timeout_ms);

/**
 * @brief Transmit data (interrupt mode)
 *
 * Non-blocking. Callback will be invoked when transmission completes.
 *
 * @param handle UART handle
 * @param data Pointer to data buffer (must remain valid until callback)
 * @param size Number of bytes to transmit
 *
 * @return CF_OK on success
 * @return CF_ERROR_NULL_POINTER if handle or data is NULL
 * @return CF_ERROR_BUSY if UART is busy
 * @return CF_ERROR_HARDWARE if hardware error
 */
cf_status_t cf_uart_transmit_it(cf_uart_handle_t handle, const uint8_t* data, uint16_t size);

/**
 * @brief Receive data (interrupt mode)
 *
 * Non-blocking. Callback will be invoked when reception completes.
 *
 * @param handle UART handle
 * @param data Pointer to receive buffer (must remain valid until callback)
 * @param size Number of bytes to receive
 *
 * @return CF_OK on success
 * @return CF_ERROR_NULL_POINTER if handle or data is NULL
 * @return CF_ERROR_BUSY if UART is busy
 * @return CF_ERROR_HARDWARE if hardware error
 */
cf_status_t cf_uart_receive_it(cf_uart_handle_t handle, uint8_t* data, uint16_t size);

/**
 * @brief Transmit data (DMA mode)
 *
 * Non-blocking. Callback will be invoked when transmission completes.
 *
 * @param handle UART handle
 * @param data Pointer to data buffer (must remain valid until callback)
 * @param size Number of bytes to transmit
 *
 * @return CF_OK on success
 * @return CF_ERROR_NULL_POINTER if handle or data is NULL
 * @return CF_ERROR_BUSY if UART is busy
 * @return CF_ERROR_HARDWARE if hardware error
 */
cf_status_t cf_uart_transmit_dma(cf_uart_handle_t handle, const uint8_t* data, uint16_t size);

/**
 * @brief Receive data (DMA mode)
 *
 * Non-blocking. Callback will be invoked when reception completes.
 *
 * @param handle UART handle
 * @param data Pointer to receive buffer (must remain valid until callback)
 * @param size Number of bytes to receive
 *
 * @return CF_OK on success
 * @return CF_ERROR_NULL_POINTER if handle or data is NULL
 * @return CF_ERROR_BUSY if UART is busy
 * @return CF_ERROR_HARDWARE if hardware error
 */
cf_status_t cf_uart_receive_dma(cf_uart_handle_t handle, uint8_t* data, uint16_t size);

/**
 * @brief Abort ongoing transmission
 *
 * @param handle UART handle
 *
 * @return CF_OK on success
 * @return CF_ERROR_NULL_POINTER if handle is NULL
 */
cf_status_t cf_uart_abort_transmit(cf_uart_handle_t handle);

/**
 * @brief Abort ongoing reception
 *
 * @param handle UART handle
 *
 * @return CF_OK on success
 * @return CF_ERROR_NULL_POINTER if handle is NULL
 */
cf_status_t cf_uart_abort_receive(cf_uart_handle_t handle);

/**
 * @brief Get last UART error
 *
 * @param handle UART handle
 * @param[out] error Pointer to receive error flags
 *
 * @return CF_OK on success
 * @return CF_ERROR_NULL_POINTER if handle or error is NULL
 */
cf_status_t cf_uart_get_error(cf_uart_handle_t handle, cf_uart_error_t* error);

/**
 * @brief Start UART receive with IDLE line detection (DMA mode)
 *
 * Perfect for variable-length packet reception. Callback will be invoked when:
 * - Buffer is full (received 'size' bytes), OR
 * - IDLE line detected (no data for ~1 byte time)
 *
 * Use cf_uart_get_received_count() in callback to get actual bytes received.
 *
 * @param handle UART handle
 * @param data Receive buffer (must remain valid until callback)
 * @param size Maximum buffer size
 *
 * @return CF_OK on success
 * @return CF_ERROR_NULL_POINTER if handle or data is NULL
 * @return CF_ERROR_BUSY if UART is busy
 * @return CF_ERROR_HARDWARE if hardware error
 */
cf_status_t cf_uart_receive_to_idle_dma(cf_uart_handle_t handle, uint8_t* data, uint16_t size);

/**
 * @brief Get number of bytes received (useful in IDLE callback)
 *
 * @param handle UART handle
 * @param[out] count Pointer to receive byte count
 *
 * @return CF_OK on success
 * @return CF_ERROR_NULL_POINTER if handle or count is NULL
 */
cf_status_t cf_uart_get_received_count(cf_uart_handle_t handle, uint16_t* count);

#ifdef __cplusplus
}
#endif

#endif /* CF_UART_H */
