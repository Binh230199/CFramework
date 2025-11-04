/**
 * @file cf_gpio.c
 * @brief GPIO HAL implementation (platform-independent)
 */

#include "hal/cf_gpio.h"
#include "hal/cf_gpio_port.h"
#include "cf_assert.h"

#if CF_RTOS_ENABLED
    #include "os/cf_mutex.h"
#endif

#include <string.h>

//==============================================================================
// PRIVATE TYPES
//==============================================================================

struct cf_gpio_handle_s {
    bool initialized;
    cf_gpio_config_t config;
    void* platform_data;  /**< Platform-specific data */
};

//==============================================================================
// PRIVATE VARIABLES
//==============================================================================

static struct cf_gpio_handle_s g_gpio_handles[CF_HAL_GPIO_MAX_HANDLES];

#if CF_RTOS_ENABLED
static cf_mutex_t g_gpio_mutex = NULL;
#endif

static bool g_gpio_initialized = false;

//==============================================================================
// PRIVATE FUNCTIONS
//==============================================================================

static cf_status_t gpio_module_init(void)
{
    if (g_gpio_initialized) {
        return CF_OK;
    }

#if CF_RTOS_ENABLED
    cf_status_t status = cf_mutex_create(&g_gpio_mutex);
    if (status != CF_OK) {
        return status;
    }
#endif

    memset(g_gpio_handles, 0, sizeof(g_gpio_handles));
    g_gpio_initialized = true;

    return CF_OK;
}

static cf_gpio_handle_t find_free_handle(void)
{
    for (uint32_t i = 0; i < CF_HAL_GPIO_MAX_HANDLES; i++) {
        if (!g_gpio_handles[i].initialized) {
            return &g_gpio_handles[i];
        }
    }
    return NULL;
}

static bool is_valid_config(const cf_gpio_config_t* config)
{
    if (config == NULL) {
        return false;
    }

    if (config->pin > 15) {
        return false;
    }

    if (config->mode > CF_GPIO_MODE_IT_RISING_FALLING) {  // Check against max mode
        return false;
    }

    return true;
}

//==============================================================================
// PUBLIC API IMPLEMENTATION
//==============================================================================

cf_status_t cf_gpio_init(cf_gpio_handle_t* handle, const cf_gpio_config_t* config)
{
    CF_PTR_CHECK(handle);
    CF_PTR_CHECK(config);

    if (!is_valid_config(config)) {
        return CF_ERROR_INVALID_PARAM;
    }

    // Initialize module if needed
    cf_status_t status = gpio_module_init();
    if (status != CF_OK) {
        return status;
    }

#if CF_RTOS_ENABLED
    CF_MUTEX_LOCK(g_gpio_mutex, CF_WAIT_FOREVER);
#endif

    // Find free handle
    cf_gpio_handle_t h = find_free_handle();
    if (h == NULL) {
#if CF_RTOS_ENABLED
        cf_mutex_unlock(g_gpio_mutex);
#endif
        return CF_ERROR_NO_RESOURCE;
    }

    // Copy configuration
    memcpy(&h->config, config, sizeof(cf_gpio_config_t));

    // Call platform-specific initialization
    status = cf_gpio_port_init(h, config);
    if (status != CF_OK) {
#if CF_RTOS_ENABLED
        cf_mutex_unlock(g_gpio_mutex);
#endif
        return status;
    }

    h->initialized = true;
    *handle = h;

#if CF_RTOS_ENABLED
    CF_MUTEX_UNLOCK(g_gpio_mutex);
#endif

    return CF_OK;
}

void cf_gpio_deinit(cf_gpio_handle_t handle)
{
    if (handle == NULL || !handle->initialized) {
        return;
    }

#if CF_RTOS_ENABLED
    cf_mutex_lock(g_gpio_mutex, CF_WAIT_FOREVER);
#endif

    cf_gpio_port_deinit(handle);

    handle->initialized = false;
    memset(handle, 0, sizeof(struct cf_gpio_handle_s));

#if CF_RTOS_ENABLED
    cf_mutex_unlock(g_gpio_mutex);
#endif
}

cf_status_t cf_gpio_write(cf_gpio_handle_t handle, cf_gpio_pin_state_t state)
{
    CF_PTR_CHECK(handle);

    if (!handle->initialized) {
        return CF_ERROR_NOT_INITIALIZED;
    }

    // Check if pin is configured as output
    if (handle->config.mode != CF_GPIO_MODE_OUTPUT_PP &&
        handle->config.mode != CF_GPIO_MODE_OUTPUT_OD) {
        return CF_ERROR_INVALID_STATE;
    }

    return cf_gpio_port_write(handle, state);
}

cf_status_t cf_gpio_read(cf_gpio_handle_t handle, cf_gpio_pin_state_t* state)
{
    CF_PTR_CHECK(handle);
    CF_PTR_CHECK(state);

    if (!handle->initialized) {
        return CF_ERROR_NOT_INITIALIZED;
    }

    return cf_gpio_port_read(handle, state);
}

cf_status_t cf_gpio_toggle(cf_gpio_handle_t handle)
{
    CF_PTR_CHECK(handle);

    if (!handle->initialized) {
        return CF_ERROR_NOT_INITIALIZED;
    }

    // Check if pin is configured as output
    if (handle->config.mode != CF_GPIO_MODE_OUTPUT_PP &&
        handle->config.mode != CF_GPIO_MODE_OUTPUT_OD) {
        return CF_ERROR_INVALID_STATE;
    }

    return cf_gpio_port_toggle(handle);
}

void cf_gpio_config_default(cf_gpio_config_t* config)
{
    if (config == NULL) {
        return;
    }

    memset(config, 0, sizeof(cf_gpio_config_t));
    config->mode = CF_GPIO_MODE_INPUT;
    config->pull = CF_GPIO_PULL_NONE;
    config->speed = CF_GPIO_SPEED_LOW;
}
