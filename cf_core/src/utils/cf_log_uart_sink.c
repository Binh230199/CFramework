/**
 * @file cf_log_uart_sink.c
 * @brief UART sink implementation
 */

#include "utils/cf_log_uart_sink.h"

#if CF_LOG_ENABLED

#include <string.h>

//==============================================================================
// FORWARD DECLARATIONS
//==============================================================================

static cf_status_t uart_sink_write(cf_log_sink_t* self, cf_log_level_t level, const char* message);
static void uart_sink_set_level(cf_log_sink_t* self, cf_log_level_t level);
static cf_log_level_t uart_sink_get_level(cf_log_sink_t* self);
static void uart_sink_destroy(cf_log_sink_t* self);

//==============================================================================
// VIRTUAL TABLE
//==============================================================================

static const cf_log_sink_vtable_t uart_sink_vtable = {
    .write = uart_sink_write,
    .set_level = uart_sink_set_level,
    .get_level = uart_sink_get_level,
    .destroy = uart_sink_destroy
};

//==============================================================================
// PLATFORM-SPECIFIC WRITE FUNCTIONS
//==============================================================================

#if defined(CF_PLATFORM_STM32L4) || defined(CF_PLATFORM_STM32L1) || \
    defined(CF_PLATFORM_STM32F1) || defined(CF_PLATFORM_STM32F4)

static cf_status_t platform_uart_write(cf_uart_port_t uart, const uint8_t* data, uint32_t len, uint32_t timeout)
{
    HAL_StatusTypeDef status = HAL_UART_Transmit(uart, (uint8_t*)data, len, timeout);

    if (status == HAL_OK) {
        return CF_OK;
    }

    return CF_ERROR_HARDWARE;
}

#elif defined(CF_PLATFORM_ESP32)

#include "driver/uart.h"

static cf_status_t platform_uart_write(cf_uart_port_t uart, const uint8_t* data, uint32_t len, uint32_t timeout)
{
    int written = uart_write_bytes(uart, (const char*)data, len);

    if (written == len) {
        return CF_OK;
    }

    return CF_ERROR_HARDWARE;
}

#else

static cf_status_t platform_uart_write(cf_uart_port_t uart, const uint8_t* data, uint32_t len, uint32_t timeout)
{
    // Placeholder for other platforms
    (void)uart;
    (void)data;
    (void)len;
    (void)timeout;
    return CF_ERROR_NOT_SUPPORTED;
}

#endif

//==============================================================================
// VIRTUAL TABLE IMPLEMENTATION
//==============================================================================

static cf_status_t uart_sink_write(cf_log_sink_t* self, cf_log_level_t level, const char* message)
{
    cf_uart_sink_t* uart_sink = (cf_uart_sink_t*)self;

    if (!cf_log_sink_should_log(self, level)) {
        return CF_OK;
    }

    size_t len = strlen(message);
    return platform_uart_write(uart_sink->uart, (const uint8_t*)message, len, uart_sink->timeout_ms);
}

static void uart_sink_set_level(cf_log_sink_t* self, cf_log_level_t level)
{
    if (self != NULL) {
        self->min_level = level;
    }
}

static cf_log_level_t uart_sink_get_level(cf_log_sink_t* self)
{
    if (self != NULL) {
        return self->min_level;
    }
    return CF_LOG_TRACE;
}

static void uart_sink_destroy(cf_log_sink_t* self)
{
    if (self != NULL) {
        memset(self, 0, sizeof(cf_uart_sink_t));
    }
}

//==============================================================================
// PUBLIC API IMPLEMENTATION
//==============================================================================

cf_status_t cf_uart_sink_create(cf_uart_sink_t* sink,
                                 const cf_uart_sink_config_t* config,
                                 cf_log_level_t min_level)
{
    CF_PTR_CHECK(sink);
    CF_PTR_CHECK(config);

    memset(sink, 0, sizeof(cf_uart_sink_t));

    // Initialize base sink
    cf_log_sink_init(&sink->base, &uart_sink_vtable, "UART", min_level);

    // Copy configuration
    sink->uart = config->uart;
    sink->timeout_ms = config->timeout_ms;

    return CF_OK;
}

void cf_uart_sink_destroy(cf_uart_sink_t* sink)
{
    if (sink != NULL) {
        memset(sink, 0, sizeof(cf_uart_sink_t));
    }
}

#endif /* CF_LOG_ENABLED */
