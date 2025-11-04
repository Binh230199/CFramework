/**
 * @file cf_gpio.h
 * @brief GPIO Hardware Abstraction Layer
 * @version 1.0.0
 * @date 2025-10-29
 * @author CFramework Contributors
 *
 * @copyright Copyright (c) 2025 CFramework
 * Licensed under MIT License
 */

#ifndef CF_GPIO_H
#define CF_GPIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cf_common.h"

//==============================================================================
// TYPE DEFINITIONS
//==============================================================================

/**
 * @brief Opaque GPIO handle
 */
typedef struct cf_gpio_handle_s* cf_gpio_handle_t;

/**
 * @brief GPIO mode
 */
typedef enum {
    CF_GPIO_MODE_INPUT,
    CF_GPIO_MODE_OUTPUT_PP,      /**< Push-pull output */
    CF_GPIO_MODE_OUTPUT_OD,      /**< Open-drain output */
    CF_GPIO_MODE_AF_PP,          /**< Alternate function push-pull */
    CF_GPIO_MODE_AF_OD,          /**< Alternate function open-drain */
    CF_GPIO_MODE_ANALOG,
    CF_GPIO_MODE_IT_RISING,         /**< Interrupt on rising edge */
    CF_GPIO_MODE_IT_FALLING,        /**< Interrupt on falling edge */
    CF_GPIO_MODE_IT_RISING_FALLING  /**< Interrupt on both edges */
} cf_gpio_mode_t;

/**
 * @brief GPIO pull configuration
 */
typedef enum {
    CF_GPIO_PULL_NONE,
    CF_GPIO_PULL_UP,
    CF_GPIO_PULL_DOWN
} cf_gpio_pull_t;

/**
 * @brief GPIO speed
 */
typedef enum {
    CF_GPIO_SPEED_LOW,
    CF_GPIO_SPEED_MEDIUM,
    CF_GPIO_SPEED_HIGH,
    CF_GPIO_SPEED_VERY_HIGH
} cf_gpio_speed_t;

/**
 * @brief GPIO pin state
 */
typedef enum {
    CF_GPIO_PIN_RESET = 0,
    CF_GPIO_PIN_SET = 1
} cf_gpio_pin_state_t;

/**
 * @brief GPIO interrupt callback function type
 *
 * @param handle GPIO handle that triggered the interrupt
 * @param user_data User data passed during callback registration
 *
 * @note This callback is called from ISR context - keep it short!
 * @note Do NOT call blocking functions or log from this callback
 */
typedef void (*cf_gpio_irq_callback_t)(cf_gpio_handle_t handle, void* user_data);

/**
 * @brief GPIO configuration
 */
typedef struct {
    uint32_t port;               /**< GPIO port (platform-specific) */
    uint32_t pin;                /**< GPIO pin number (0-15) */
    cf_gpio_mode_t mode;         /**< Pin mode */
    cf_gpio_pull_t pull;         /**< Pull-up/down configuration */
    cf_gpio_speed_t speed;       /**< Output speed */
    uint32_t alternate;          /**< Alternate function (if AF mode) */
    cf_gpio_irq_callback_t irq_callback;  /**< Interrupt callback (for EXTI modes) */
    void* irq_user_data;         /**< User data for interrupt callback */
} cf_gpio_config_t;

//==============================================================================
// PUBLIC API
//==============================================================================

/**
 * @brief Initialize GPIO pin
 *
 * @param[out] handle Pointer to receive GPIO handle
 * @param[in] config GPIO configuration
 *
 * @return CF_OK on success
 * @return CF_ERROR_NULL_POINTER if handle or config is NULL
 * @return CF_ERROR_INVALID_PARAM if config parameters are invalid
 * @return CF_ERROR_NO_RESOURCE if no free handle available
 * @return CF_ERROR_HARDWARE on hardware error
 *
 * @note This function is thread-safe
 */
cf_status_t cf_gpio_init(cf_gpio_handle_t* handle, const cf_gpio_config_t* config);

/**
 * @brief Deinitialize GPIO pin
 *
 * @param[in] handle GPIO handle
 *
 * @note This function is thread-safe
 */
void cf_gpio_deinit(cf_gpio_handle_t handle);

/**
 * @brief Write pin state
 *
 * @param[in] handle GPIO handle
 * @param[in] state Pin state to write
 *
 * @return CF_OK on success
 * @return CF_ERROR_NULL_POINTER if handle is NULL
 * @return CF_ERROR_INVALID_STATE if pin not configured as output
 *
 * @note This function is thread-safe
 */
cf_status_t cf_gpio_write(cf_gpio_handle_t handle, cf_gpio_pin_state_t state);

/**
 * @brief Read pin state
 *
 * @param[in] handle GPIO handle
 * @param[out] state Pointer to receive pin state
 *
 * @return CF_OK on success
 * @return CF_ERROR_NULL_POINTER if handle or state is NULL
 *
 * @note This function is thread-safe
 */
cf_status_t cf_gpio_read(cf_gpio_handle_t handle, cf_gpio_pin_state_t* state);

/**
 * @brief Toggle pin state
 *
 * @param[in] handle GPIO handle
 *
 * @return CF_OK on success
 * @return CF_ERROR_NULL_POINTER if handle is NULL
 * @return CF_ERROR_INVALID_STATE if pin not configured as output
 *
 * @note This function is thread-safe
 */
cf_status_t cf_gpio_toggle(cf_gpio_handle_t handle);

/**
 * @brief Get default GPIO configuration
 *
 * @param[out] config Configuration structure to initialize
 */
void cf_gpio_config_default(cf_gpio_config_t* config);

#ifdef __cplusplus
}
#endif

#endif /* CF_GPIO_H */
