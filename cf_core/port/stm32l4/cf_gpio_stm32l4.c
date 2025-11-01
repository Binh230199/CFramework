/**
 * @file cf_gpio_stm32l4.c
 * @brief GPIO implementation for STM32L4 series
 * @version 1.0.0
 * @date 2025-10-29
 * @author CFramework Contributors
 *
 * @copyright Copyright (c) 2025 CFramework
 * Licensed under MIT License
 */

#include "hal/cf_gpio.h"
#include "hal/cf_gpio_port.h"

#ifdef CF_PLATFORM_STM32L4

#include "stm32l4xx_hal.h"
#include <string.h>

//==============================================================================
// FORWARD DECLARATION - GPIO handle structure from cf_gpio.c
//==============================================================================

struct cf_gpio_handle_s {
    bool initialized;
    cf_gpio_config_t config;
    void* platform_data;
};

//==============================================================================
// PLATFORM DATA
//==============================================================================

typedef struct {
    GPIO_TypeDef* gpio_port;
    uint16_t gpio_pin;
} stm32l4_gpio_data_t;

//==============================================================================
// HELPER FUNCTIONS
//==============================================================================

static GPIO_TypeDef* get_gpio_port(uint32_t port)
{
    switch (port) {
        case 0: return GPIOA;
        case 1: return GPIOB;
        case 2: return GPIOC;
        case 3: return GPIOD;
        case 4: return GPIOE;
#ifdef GPIOF
        case 5: return GPIOF;
#endif
#ifdef GPIOG
        case 6: return GPIOG;
#endif
#ifdef GPIOH
        case 7: return GPIOH;
#endif
        default: return NULL;
    }
}

static void enable_gpio_clock(GPIO_TypeDef* port)
{
    if (port == GPIOA) __HAL_RCC_GPIOA_CLK_ENABLE();
    else if (port == GPIOB) __HAL_RCC_GPIOB_CLK_ENABLE();
    else if (port == GPIOC) __HAL_RCC_GPIOC_CLK_ENABLE();
    else if (port == GPIOD) __HAL_RCC_GPIOD_CLK_ENABLE();
    else if (port == GPIOE) __HAL_RCC_GPIOE_CLK_ENABLE();
#ifdef GPIOF
    else if (port == GPIOF) __HAL_RCC_GPIOF_CLK_ENABLE();
#endif
#ifdef GPIOG
    else if (port == GPIOG) __HAL_RCC_GPIOG_CLK_ENABLE();
#endif
#ifdef GPIOH
    else if (port == GPIOH) __HAL_RCC_GPIOH_CLK_ENABLE();
#endif
}

static uint32_t convert_mode(cf_gpio_mode_t mode)
{
    switch (mode) {
        case CF_GPIO_MODE_INPUT:      return GPIO_MODE_INPUT;
        case CF_GPIO_MODE_OUTPUT_PP:  return GPIO_MODE_OUTPUT_PP;
        case CF_GPIO_MODE_OUTPUT_OD:  return GPIO_MODE_OUTPUT_OD;
        case CF_GPIO_MODE_AF_PP:      return GPIO_MODE_AF_PP;
        case CF_GPIO_MODE_AF_OD:      return GPIO_MODE_AF_OD;
        case CF_GPIO_MODE_ANALOG:     return GPIO_MODE_ANALOG;
        default:                       return GPIO_MODE_INPUT;
    }
}

static uint32_t convert_pull(cf_gpio_pull_t pull)
{
    switch (pull) {
        case CF_GPIO_PULL_NONE: return GPIO_NOPULL;
        case CF_GPIO_PULL_UP:   return GPIO_PULLUP;
        case CF_GPIO_PULL_DOWN: return GPIO_PULLDOWN;
        default:                 return GPIO_NOPULL;
    }
}

static uint32_t convert_speed(cf_gpio_speed_t speed)
{
    switch (speed) {
        case CF_GPIO_SPEED_LOW:       return GPIO_SPEED_FREQ_LOW;
        case CF_GPIO_SPEED_MEDIUM:    return GPIO_SPEED_FREQ_MEDIUM;
        case CF_GPIO_SPEED_HIGH:      return GPIO_SPEED_FREQ_HIGH;
        case CF_GPIO_SPEED_VERY_HIGH: return GPIO_SPEED_FREQ_VERY_HIGH;
        default:                       return GPIO_SPEED_FREQ_LOW;
    }
}

//==============================================================================
// PORT INTERFACE IMPLEMENTATION
//==============================================================================

cf_status_t cf_gpio_port_init(cf_gpio_handle_t handle, const cf_gpio_config_t* config)
{
    // Get GPIO port
    GPIO_TypeDef* port = get_gpio_port(config->port);
    if (port == NULL) {
        return CF_ERROR_INVALID_PARAM;
    }

    // Enable clock
    enable_gpio_clock(port);

    // Setup HAL GPIO init structure
    GPIO_InitTypeDef gpio_init = {0};
    gpio_init.Pin = (1U << config->pin);
    gpio_init.Mode = convert_mode(config->mode);
    gpio_init.Pull = convert_pull(config->pull);
    gpio_init.Speed = convert_speed(config->speed);
    gpio_init.Alternate = config->alternate;

    // Initialize GPIO
    HAL_GPIO_Init(port, &gpio_init);

    // Store platform data
    stm32l4_gpio_data_t* pdata = (stm32l4_gpio_data_t*)&handle->platform_data;
    pdata->gpio_port = port;
    pdata->gpio_pin = (1U << config->pin);

    return CF_OK;
}

void cf_gpio_port_deinit(cf_gpio_handle_t handle)
{
    if (handle == NULL) {
        return;
    }

    stm32l4_gpio_data_t* pdata = (stm32l4_gpio_data_t*)&handle->platform_data;
    if (pdata->gpio_port != NULL) {
        HAL_GPIO_DeInit(pdata->gpio_port, pdata->gpio_pin);
    }
}

cf_status_t cf_gpio_port_write(cf_gpio_handle_t handle, cf_gpio_pin_state_t state)
{
    stm32l4_gpio_data_t* pdata = (stm32l4_gpio_data_t*)&handle->platform_data;

    HAL_GPIO_WritePin(pdata->gpio_port, pdata->gpio_pin,
                      state == CF_GPIO_PIN_SET ? GPIO_PIN_SET : GPIO_PIN_RESET);

    return CF_OK;
}

cf_status_t cf_gpio_port_read(cf_gpio_handle_t handle, cf_gpio_pin_state_t* state)
{
    stm32l4_gpio_data_t* pdata = (stm32l4_gpio_data_t*)&handle->platform_data;

    GPIO_PinState pin_state = HAL_GPIO_ReadPin(pdata->gpio_port, pdata->gpio_pin);
    *state = (pin_state == GPIO_PIN_SET) ? CF_GPIO_PIN_SET : CF_GPIO_PIN_RESET;

    return CF_OK;
}

cf_status_t cf_gpio_port_toggle(cf_gpio_handle_t handle)
{
    stm32l4_gpio_data_t* pdata = (stm32l4_gpio_data_t*)&handle->platform_data;

    HAL_GPIO_TogglePin(pdata->gpio_port, pdata->gpio_pin);

    return CF_OK;
}

#endif /* CF_PLATFORM_STM32L4 */
