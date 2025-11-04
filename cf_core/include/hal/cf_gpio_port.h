/**
 * @file cf_gpio_port.h
 * @brief GPIO port interface for platform implementations
 * @version 1.0.0
 * @date 2025-10-29
 * @author CFramework Contributors
 *
 * @copyright Copyright (c) 2025 CFramework
 * Licensed under MIT License
 *
 * This file defines the interface that platform-specific implementations
 * must provide for GPIO functionality.
 */

#ifndef CF_GPIO_PORT_H
#define CF_GPIO_PORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "hal/cf_gpio.h"

//==============================================================================
// PLATFORM PORT INTERFACE
//==============================================================================

/**
 * @brief Platform-specific GPIO initialization
 *
 * @param[in] handle GPIO handle (already allocated)
 * @param[in] config GPIO configuration
 *
 * @return CF_OK on success, error code on failure
 *
 * @note This function must be implemented by platform port
 */
cf_status_t cf_gpio_port_init(cf_gpio_handle_t handle, const cf_gpio_config_t* config);

/**
 * @brief Platform-specific GPIO deinitialization
 *
 * @param[in] handle GPIO handle
 *
 * @note This function must be implemented by platform port
 */
void cf_gpio_port_deinit(cf_gpio_handle_t handle);

/**
 * @brief Platform-specific GPIO write
 *
 * @param[in] handle GPIO handle
 * @param[in] state Pin state to write
 *
 * @return CF_OK on success, error code on failure
 *
 * @note This function must be implemented by platform port
 */
cf_status_t cf_gpio_port_write(cf_gpio_handle_t handle, cf_gpio_pin_state_t state);

/**
 * @brief Platform-specific GPIO read
 *
 * @param[in] handle GPIO handle
 * @param[out] state Pointer to receive pin state
 *
 * @return CF_OK on success, error code on failure
 *
 * @note This function must be implemented by platform port
 */
cf_status_t cf_gpio_port_read(cf_gpio_handle_t handle, cf_gpio_pin_state_t* state);

/**
 * @brief Platform-specific GPIO toggle
 *
 * @param[in] handle GPIO handle
 *
 * @return CF_OK on success, error code on failure
 *
 * @note This function must be implemented by platform port
 */
cf_status_t cf_gpio_port_toggle(cf_gpio_handle_t handle);

/**
 * @brief EXTI interrupt callback (platform-specific)
 *
 * @param GPIO_Pin Pin number bitmask that triggered the interrupt
 *
 * @note This function should be called from HAL_GPIO_EXTI_Callback()
 * @note Only available on platforms that support EXTI
 */
void cf_gpio_exti_callback(uint16_t GPIO_Pin);

#ifdef __cplusplus
}
#endif

#endif /* CF_GPIO_PORT_H */
