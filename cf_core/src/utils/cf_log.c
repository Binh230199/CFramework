/**
 * @file cf_log.c
 * @brief Logger system implementation
 */

#include "utils/cf_log.h"

#if CF_LOG_ENABLED

#include "utils/cf_string.h"
#include "cf_assert.h"

#if CF_RTOS_ENABLED
    #include "os/cf_mutex.h"
    #include "os/cf_task.h"
#endif

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

//==============================================================================
// PRIVATE VARIABLES
//==============================================================================

typedef struct {
    bool initialized;
    cf_log_level_t min_level;
    cf_log_sink_t* sinks[CF_LOG_MAX_SINKS];
    uint8_t sink_count;
    char buffer[CF_LOG_BUFFER_SIZE];
#if CF_RTOS_ENABLED
    cf_mutex_t mutex;
#endif
} cf_logger_t;

static cf_logger_t g_logger = {
    .initialized = false,
    .min_level = CF_LOG_DEBUG,
    .sink_count = 0
};

//==============================================================================
// PRIVATE FUNCTIONS
//==============================================================================

static const char* level_strings[] = {
    "TRACE",
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
    "FATAL"
};

//==============================================================================
// PUBLIC API IMPLEMENTATION
//==============================================================================

cf_status_t cf_log_init(void)
{
    if (g_logger.initialized) {
        return CF_OK;
    }

#if CF_RTOS_ENABLED
    cf_status_t status = cf_mutex_create(&g_logger.mutex);
    if (status != CF_OK) {
        return status;
    }
#endif

    g_logger.sink_count = 0;
    memset(g_logger.sinks, 0, sizeof(g_logger.sinks));
    g_logger.initialized = true;

    return CF_OK;
}

void cf_log_deinit(void)
{
    if (!g_logger.initialized) {
        return;
    }

#if CF_RTOS_ENABLED
    cf_mutex_lock(g_logger.mutex, CF_WAIT_FOREVER);
#endif

    g_logger.sink_count = 0;
    memset(g_logger.sinks, 0, sizeof(g_logger.sinks));

#if CF_RTOS_ENABLED
    cf_mutex_unlock(g_logger.mutex);
    cf_mutex_destroy(g_logger.mutex);
    g_logger.mutex = NULL;
#endif

    g_logger.initialized = false;
}

cf_status_t cf_log_add_sink(cf_log_sink_t* sink)
{
    CF_PTR_CHECK(sink);

    if (!g_logger.initialized) {
        return CF_ERROR_NOT_INITIALIZED;
    }

#if CF_RTOS_ENABLED
    CF_MUTEX_LOCK(g_logger.mutex, CF_WAIT_FOREVER);
#endif

    // Check if already registered
    for (uint8_t i = 0; i < g_logger.sink_count; i++) {
        if (g_logger.sinks[i] == sink) {
#if CF_RTOS_ENABLED
            cf_mutex_unlock(g_logger.mutex);
#endif
            return CF_OK;
        }
    }

    // Check if max sinks reached
    if (g_logger.sink_count >= CF_LOG_MAX_SINKS) {
#if CF_RTOS_ENABLED
        cf_mutex_unlock(g_logger.mutex);
#endif
        return CF_ERROR_NO_RESOURCE;
    }

    // Add sink
    g_logger.sinks[g_logger.sink_count] = sink;
    g_logger.sink_count++;

#if CF_RTOS_ENABLED
    CF_MUTEX_UNLOCK(g_logger.mutex);
#endif

    return CF_OK;
}

void cf_log_remove_sink(cf_log_sink_t* sink)
{
    if (sink == NULL || !g_logger.initialized) {
        return;
    }

#if CF_RTOS_ENABLED
    cf_mutex_lock(g_logger.mutex, CF_WAIT_FOREVER);
#endif

    // Find and remove sink
    for (uint8_t i = 0; i < g_logger.sink_count; i++) {
        if (g_logger.sinks[i] == sink) {
            // Shift remaining sinks
            for (uint8_t j = i; j < g_logger.sink_count - 1; j++) {
                g_logger.sinks[j] = g_logger.sinks[j + 1];
            }
            g_logger.sink_count--;
            g_logger.sinks[g_logger.sink_count] = NULL;
            break;
        }
    }

#if CF_RTOS_ENABLED
    cf_mutex_unlock(g_logger.mutex);
#endif
}

void cf_log_clear_sinks(void)
{
    if (!g_logger.initialized) {
        return;
    }

#if CF_RTOS_ENABLED
    cf_mutex_lock(g_logger.mutex, CF_WAIT_FOREVER);
#endif

    g_logger.sink_count = 0;
    memset(g_logger.sinks, 0, sizeof(g_logger.sinks));

#if CF_RTOS_ENABLED
    cf_mutex_unlock(g_logger.mutex);
#endif
}

uint8_t cf_log_get_sink_count(void)
{
    return g_logger.sink_count;
}

void cf_log_set_level(cf_log_level_t level)
{
    if (level < CF_LOG_LEVEL_COUNT) {
        g_logger.min_level = level;
    }
}

cf_log_level_t cf_log_get_level(void)
{
    return g_logger.min_level;
}

void cf_log_write(cf_log_level_t level, const char* fmt, ...)
{
    if (!g_logger.initialized || level < g_logger.min_level) {
        return;
    }

#if CF_RTOS_ENABLED
    cf_mutex_lock(g_logger.mutex, CF_WAIT_FOREVER);
#endif

    // Format message
    va_list args;
    va_start(args, fmt);
    vsnprintf(g_logger.buffer, CF_LOG_BUFFER_SIZE, fmt, args);
    va_end(args);

    // Ensure null termination
    g_logger.buffer[CF_LOG_BUFFER_SIZE - 1] = '\0';

    // Send to all sinks
    for (uint8_t i = 0; i < g_logger.sink_count; i++) {
        cf_log_sink_t* sink = g_logger.sinks[i];
        if (sink != NULL && sink->vtable != NULL && sink->vtable->write != NULL) {
            if (cf_log_sink_should_log(sink, level)) {
                sink->vtable->write(sink, level, g_logger.buffer);
            }
        }
    }

#if CF_RTOS_ENABLED
    cf_mutex_unlock(g_logger.mutex);
#endif
}

const char* cf_log_level_to_string(cf_log_level_t level)
{
    if (level < CF_LOG_LEVEL_COUNT) {
        return level_strings[level];
    }
    return "UNKNOWN";
}

//==============================================================================
// SINK HELPER IMPLEMENTATION
//==============================================================================

void cf_log_sink_init(cf_log_sink_t* sink,
                      const cf_log_sink_vtable_t* vtable,
                      const char* name,
                      cf_log_level_t min_level)
{
    if (sink == NULL) {
        return;
    }

    memset(sink, 0, sizeof(cf_log_sink_t));
    sink->vtable = vtable;
    sink->min_level = min_level;

    if (name != NULL) {
        CF_STRNCPY(sink->name, name, sizeof(sink->name));
    }
}

bool cf_log_sink_should_log(const cf_log_sink_t* sink, cf_log_level_t level)
{
    if (sink == NULL) {
        return false;
    }

    return level >= sink->min_level;
}

#endif /* CF_LOG_ENABLED */
