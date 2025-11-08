/**
 * @file cf_uart_port.h
 * @brief UART platform interface for CFramework
 * @version 1.0.0
 * @date 2025-11-04
 *
 * @copyright Copyright (c) 2025 CFramework
 * Licensed under MIT License
 *
 * Platform-specific UART interface to be implemented by each port
 */

#ifndef CF_UART_PORT_H
#define CF_UART_PORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "hal/cf_uart.h"

/* ============================================================
 * ACCESSOR FUNCTIONS (for port layer to access handle internals)
 * ============================================================ */

/**
 * @brief Get platform data pointer from handle
 *
 * @param handle UART handle
 * @return void* Platform data pointer
 */
static inline void* cf_uart_get_platform_data(cf_uart_handle_t handle)
{
    return handle ? ((struct { void* _1; void* platform_data; }*)handle)->platform_data : NULL;
}

/**
 * @brief Set platform data pointer in handle
 *
 * @param handle UART handle
 * @param pdata Platform data pointer
 */
static inline void cf_uart_set_platform_data(cf_uart_handle_t handle, void* pdata)
{
    if (handle) {
        ((struct { void* _1; void* platform_data; }*)handle)->platform_data = pdata;
    }
}

/* ============================================================
 * PLATFORM INTERFACE (to be implemented by port layer)
 * ============================================================ *//**
 * @brief Platform-specific UART initialization
 *
 * @param handle UART handle
 * @param config UART configuration
 *
 * @return CF_OK on success, error code otherwise
 */
cf_status_t cf_uart_port_init(cf_uart_handle_t handle, const cf_uart_config_t* config);

/**
 * @brief Platform-specific UART deinitialization
 *
 * @param handle UART handle
 *
 * @return CF_OK on success, error code otherwise
 */
cf_status_t cf_uart_port_deinit(cf_uart_handle_t handle);

/**
 * @brief Platform-specific UART transmit (blocking)
 *
 * @param handle UART handle
 * @param data Data buffer
 * @param size Number of bytes
 * @param timeout_ms Timeout in milliseconds
 *
 * @return CF_OK on success, error code otherwise
 */
cf_status_t cf_uart_port_transmit(cf_uart_handle_t handle, const uint8_t* data, uint16_t size, uint32_t timeout_ms);

/**
 * @brief Platform-specific UART receive (blocking)
 *
 * @param handle UART handle
 * @param data Receive buffer
 * @param size Number of bytes
 * @param timeout_ms Timeout in milliseconds
 *
 * @return CF_OK on success, error code otherwise
 */
cf_status_t cf_uart_port_receive(cf_uart_handle_t handle, uint8_t* data, uint16_t size, uint32_t timeout_ms);

/**
 * @brief Platform-specific UART transmit (interrupt)
 *
 * @param handle UART handle
 * @param data Data buffer
 * @param size Number of bytes
 *
 * @return CF_OK on success, error code otherwise
 */
cf_status_t cf_uart_port_transmit_it(cf_uart_handle_t handle, const uint8_t* data, uint16_t size);

/**
 * @brief Platform-specific UART receive (interrupt)
 *
 * @param handle UART handle
 * @param data Receive buffer
 * @param size Number of bytes
 *
 * @return CF_OK on success, error code otherwise
 */
cf_status_t cf_uart_port_receive_it(cf_uart_handle_t handle, uint8_t* data, uint16_t size);

/**
 * @brief Platform-specific UART transmit (DMA)
 *
 * @param handle UART handle
 * @param data Data buffer
 * @param size Number of bytes
 *
 * @return CF_OK on success, error code otherwise
 */
cf_status_t cf_uart_port_transmit_dma(cf_uart_handle_t handle, const uint8_t* data, uint16_t size);

/**
 * @brief Platform-specific UART receive (DMA)
 *
 * @param handle UART handle
 * @param data Receive buffer
 * @param size Number of bytes
 *
 * @return CF_OK on success, error code otherwise
 */
cf_status_t cf_uart_port_receive_dma(cf_uart_handle_t handle, uint8_t* data, uint16_t size);

/**
 * @brief Platform-specific UART abort transmit
 *
 * @param handle UART handle
 *
 * @return CF_OK on success, error code otherwise
 */
cf_status_t cf_uart_port_abort_transmit(cf_uart_handle_t handle);

/**
 * @brief Platform-specific UART abort receive
 *
 * @param handle UART handle
 *
 * @return CF_OK on success, error code otherwise
 */
cf_status_t cf_uart_port_abort_receive(cf_uart_handle_t handle);

/**
 * @brief Platform-specific UART get error
 *
 * @param handle UART handle
 * @param error Pointer to receive error flags
 *
 * @return CF_OK on success, error code otherwise
 */
cf_status_t cf_uart_port_get_error(cf_uart_handle_t handle, cf_uart_error_t* error);

/**
 * @brief Platform-specific UART receive to IDLE (DMA + IDLE detection)
 *
 * @param handle UART handle
 * @param data Receive buffer
 * @param size Maximum buffer size
 *
 * @return CF_OK on success, error code otherwise
 */
cf_status_t cf_uart_port_receive_to_idle_dma(cf_uart_handle_t handle, uint8_t* data, uint16_t size);

/**
 * @brief Platform-specific UART get received count
 *
 * @param handle UART handle
 * @param count Pointer to receive byte count
 *
 * @return CF_OK on success, error code otherwise
 */
cf_status_t cf_uart_port_get_received_count(cf_uart_handle_t handle, uint16_t* count);

#ifdef __cplusplus
}
#endif

#endif /* CF_UART_PORT_H */
