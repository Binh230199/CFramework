/**
 * @file cf_log_uart_sink.h
 * @brief UART sink implementation for logger
 * @version 1.0.0
 * @date 2025-10-29
 * @author CFramework Contributors
 *
 * @copyright Copyright (c) 2025 CFramework
 * Licensed under MIT License
 */

#ifndef CF_LOG_UART_SINK_H
#define CF_LOG_UART_SINK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "utils/cf_log.h"

#if CF_LOG_ENABLED

//==============================================================================
// PLATFORM-SPECIFIC UART HANDLE
//==============================================================================

#ifdef CF_PLATFORM_STM32L4
    #include "stm32l4xx_hal.h"
    typedef UART_HandleTypeDef* cf_uart_port_t;
#elif defined(CF_PLATFORM_STM32L1)
    #include "stm32l1xx_hal.h"
    typedef UART_HandleTypeDef* cf_uart_port_t;
#elif defined(CF_PLATFORM_STM32F1)
    #include "stm32f1xx_hal.h"
    typedef UART_HandleTypeDef* cf_uart_port_t;
#elif defined(CF_PLATFORM_STM32F4)
    #include "stm32f4xx_hal.h"
    typedef UART_HandleTypeDef* cf_uart_port_t;
#elif defined(CF_PLATFORM_ESP32)
    typedef int cf_uart_port_t;  // ESP32 UART port number
#else
    typedef void* cf_uart_port_t;
#endif

//==============================================================================
// TYPE DEFINITIONS
//==============================================================================

/**
 * @brief UART sink configuration
 */
typedef struct {
    cf_uart_port_t uart;        /**< UART handle */
    uint32_t timeout_ms;        /**< Transmit timeout */
} cf_uart_sink_config_t;

/**
 * @brief UART sink structure
 */
typedef struct {
    cf_log_sink_t base;         /**< Base sink (MUST be first) */
    cf_uart_port_t uart;        /**< UART handle */
    uint32_t timeout_ms;        /**< Transmit timeout */
} cf_uart_sink_t;

//==============================================================================
// PUBLIC API
//==============================================================================

/**
 * @brief Create UART sink
 *
 * @param[out] sink Sink structure to initialize
 * @param[in] config Sink configuration
 * @param[in] min_level Minimum log level
 *
 * @return CF_OK on success
 * @return CF_ERROR_NULL_POINTER if sink or config is NULL
 *
 * @note Sink must be registered with cf_log_add_sink()
 */
cf_status_t cf_uart_sink_create(cf_uart_sink_t* sink,
                                 const cf_uart_sink_config_t* config,
                                 cf_log_level_t min_level);

/**
 * @brief Destroy UART sink
 *
 * @param[in] sink Sink to destroy
 *
 * @note Sink should be unregistered first with cf_log_remove_sink()
 */
void cf_uart_sink_destroy(cf_uart_sink_t* sink);

#endif /* CF_LOG_ENABLED */

#ifdef __cplusplus
}
#endif

#endif /* CF_LOG_UART_SINK_H */
