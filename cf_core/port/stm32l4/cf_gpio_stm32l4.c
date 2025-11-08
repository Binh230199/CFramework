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
    cf_gpio_irq_callback_t irq_callback;
    void* irq_user_data;
} stm32l4_gpio_data_t;

// Static pool for platform data - NO DYNAMIC ALLOCATION!
static stm32l4_gpio_data_t g_platform_data_pool[CF_HAL_GPIO_MAX_HANDLES];
static bool g_pool_initialized = false;

// IRQ lookup table: map pin number to platform data
static stm32l4_gpio_data_t* g_irq_table[16] = {NULL};

//==============================================================================
// HELPER FUNCTIONS
//==============================================================================

static void init_platform_data_pool(void)
{
    if (!g_pool_initialized) {
        memset(g_platform_data_pool, 0, sizeof(g_platform_data_pool));
        g_pool_initialized = true;
    }
}

static stm32l4_gpio_data_t* alloc_platform_data(void)
{
    init_platform_data_pool();

    for (uint32_t i = 0; i < CF_HAL_GPIO_MAX_HANDLES; i++) {
        if (g_platform_data_pool[i].gpio_port == NULL) {
            return &g_platform_data_pool[i];
        }
    }
    return NULL;
}

static void free_platform_data(stm32l4_gpio_data_t* pdata)
{
    if (pdata != NULL) {
        pdata->gpio_port = NULL;
        pdata->gpio_pin = 0;
    }
}

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
        case CF_GPIO_MODE_INPUT:              return GPIO_MODE_INPUT;
        case CF_GPIO_MODE_OUTPUT_PP:          return GPIO_MODE_OUTPUT_PP;
        case CF_GPIO_MODE_OUTPUT_OD:          return GPIO_MODE_OUTPUT_OD;
        case CF_GPIO_MODE_AF_PP:              return GPIO_MODE_AF_PP;
        case CF_GPIO_MODE_AF_OD:              return GPIO_MODE_AF_OD;
        case CF_GPIO_MODE_ANALOG:             return GPIO_MODE_ANALOG;
        case CF_GPIO_MODE_IT_RISING:          return GPIO_MODE_IT_RISING;
        case CF_GPIO_MODE_IT_FALLING:         return GPIO_MODE_IT_FALLING;
        case CF_GPIO_MODE_IT_RISING_FALLING:  return GPIO_MODE_IT_RISING_FALLING;
        default:                               return GPIO_MODE_INPUT;
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

    // Allocate platform data from static pool (NO HEAP!)
    stm32l4_gpio_data_t* pdata = alloc_platform_data();
    if (pdata == NULL) {
        return CF_ERROR_NO_RESOURCE;
    }

    pdata->gpio_port = port;
    pdata->gpio_pin = (1U << config->pin);
    pdata->irq_callback = config->irq_callback;
    pdata->irq_user_data = config->irq_user_data;
    handle->platform_data = pdata;

    // If EXTI mode, register callback and enable NVIC
    if (config->mode >= CF_GPIO_MODE_IT_RISING) {
        // Map pin to platform data for ISR lookup
        g_irq_table[config->pin] = pdata;

        // Enable and configure NVIC for EXTI
        IRQn_Type irqn;
        if (config->pin == 0) irqn = EXTI0_IRQn;
        else if (config->pin == 1) irqn = EXTI1_IRQn;
        else if (config->pin == 2) irqn = EXTI2_IRQn;
        else if (config->pin == 3) irqn = EXTI3_IRQn;
        else if (config->pin == 4) irqn = EXTI4_IRQn;
        else if (config->pin >= 5 && config->pin <= 9) irqn = EXTI9_5_IRQn;
        else irqn = EXTI15_10_IRQn;

        HAL_NVIC_SetPriority(irqn, 6, 0);  // Priority 6 (above FreeRTOS threshold)
        HAL_NVIC_EnableIRQ(irqn);
    }

    return CF_OK;
}

void cf_gpio_port_deinit(cf_gpio_handle_t handle)
{
    if (handle == NULL) {
        return;
    }

    stm32l4_gpio_data_t* pdata = (stm32l4_gpio_data_t*)handle->platform_data;
    if (pdata != NULL) {
        if (pdata->gpio_port != NULL) {
            HAL_GPIO_DeInit(pdata->gpio_port, pdata->gpio_pin);
        }
        free_platform_data(pdata);  // Return to static pool
        handle->platform_data = NULL;
    }
}

cf_status_t cf_gpio_port_write(cf_gpio_handle_t handle, cf_gpio_pin_state_t state)
{
    stm32l4_gpio_data_t* pdata = (stm32l4_gpio_data_t*)handle->platform_data;

    HAL_GPIO_WritePin(pdata->gpio_port, pdata->gpio_pin,
                      state == CF_GPIO_PIN_SET ? GPIO_PIN_SET : GPIO_PIN_RESET);

    return CF_OK;
}

cf_status_t cf_gpio_port_read(cf_gpio_handle_t handle, cf_gpio_pin_state_t* state)
{
    stm32l4_gpio_data_t* pdata = (stm32l4_gpio_data_t*)handle->platform_data;

    GPIO_PinState pin_state = HAL_GPIO_ReadPin(pdata->gpio_port, pdata->gpio_pin);
    *state = (pin_state == GPIO_PIN_SET) ? CF_GPIO_PIN_SET : CF_GPIO_PIN_RESET;

    return CF_OK;
}

cf_status_t cf_gpio_port_toggle(cf_gpio_handle_t handle)
{
    stm32l4_gpio_data_t* pdata = (stm32l4_gpio_data_t*)handle->platform_data;

    HAL_GPIO_TogglePin(pdata->gpio_port, pdata->gpio_pin);

    return CF_OK;
}

//==============================================================================
// EXTI INTERRUPT HANDLER
//==============================================================================

/**
 * @brief Common EXTI callback - called by HAL_GPIO_EXTI_Callback
 *
 * @param GPIO_Pin Pin number that triggered interrupt (GPIO_PIN_x bitmask)
 *
 * @note This function should be called from HAL_GPIO_EXTI_Callback in user code
 */
void cf_gpio_exti_callback(uint16_t GPIO_Pin)
{
    // Find pin number from bitmask
    uint32_t pin_num = 0;
    for (pin_num = 0; pin_num < 16; pin_num++) {
        if (GPIO_Pin & (1U << pin_num)) {
            break;
        }
    }

    // Get platform data for this pin
    stm32l4_gpio_data_t* pdata = g_irq_table[pin_num];
    if (pdata != NULL && pdata->irq_callback != NULL) {
        // Call user callback with NULL handle (handle not needed in ISR)
        // User callback should only toggle GPIOs or set flags, not use handle
        pdata->irq_callback(NULL, pdata->irq_user_data);
    }
}

#endif /* CF_PLATFORM_STM32L4 */