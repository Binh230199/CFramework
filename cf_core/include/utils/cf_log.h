/**
 * @file cf_log.h
 * @brief Logger system with sink pattern
 * @version 1.0.0
 * @date 2025-10-29
 * @author CFramework Contributors
 *
 * @copyright Copyright (c) 2025 CFramework
 * Licensed under MIT License
 */

#ifndef CF_LOG_H
#define CF_LOG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cf_common.h"

#if CF_LOG_ENABLED

//==============================================================================
// FORWARD DECLARATIONS
//==============================================================================

struct cf_log_sink_s;

//==============================================================================
// TYPE DEFINITIONS
//==============================================================================

/**
 * @brief Log level enumeration
 */
typedef enum {
    CF_LOG_TRACE = 0,
    CF_LOG_DEBUG,
    CF_LOG_INFO,
    CF_LOG_WARN,
    CF_LOG_ERROR,
    CF_LOG_FATAL,
    CF_LOG_LEVEL_COUNT
} cf_log_level_t;

/**
 * @brief Sink virtual table for polymorphism
 */
typedef struct {
    cf_status_t (*write)(struct cf_log_sink_s* self, cf_log_level_t level, const char* message);
    void (*set_level)(struct cf_log_sink_s* self, cf_log_level_t level);
    cf_log_level_t (*get_level)(struct cf_log_sink_s* self);
    void (*destroy)(struct cf_log_sink_s* self);
} cf_log_sink_vtable_t;

/**
 * @brief Base log sink structure
 */
typedef struct cf_log_sink_s {
    const cf_log_sink_vtable_t* vtable;
    cf_log_level_t min_level;
    char name[16];
} cf_log_sink_t;

//==============================================================================
// LOGGER API
//==============================================================================

/**
 * @brief Initialize logger system
 *
 * @return CF_OK on success
 *
 * @note This must be called before any logging operations
 * @note This function is thread-safe
 */
cf_status_t cf_log_init(void);

/**
 * @brief Deinitialize logger system
 *
 * @note This function is thread-safe
 */
void cf_log_deinit(void);

/**
 * @brief Register a sink with the logger
 *
 * @param[in] sink Pointer to sink to register
 *
 * @return CF_OK on success
 * @return CF_ERROR_NULL_POINTER if sink is NULL
 * @return CF_ERROR_NO_RESOURCE if max sinks reached
 *
 * @note This function is thread-safe
 */
cf_status_t cf_log_add_sink(cf_log_sink_t* sink);

/**
 * @brief Unregister a sink from the logger
 *
 * @param[in] sink Pointer to sink to unregister
 *
 * @note This function is thread-safe
 */
void cf_log_remove_sink(cf_log_sink_t* sink);

/**
 * @brief Clear all registered sinks
 *
 * @note This function is thread-safe
 */
void cf_log_clear_sinks(void);

/**
 * @brief Get number of registered sinks
 *
 * @return Number of sinks
 *
 * @note This function is thread-safe
 */
uint8_t cf_log_get_sink_count(void);

/**
 * @brief Set global minimum log level
 *
 * @param[in] level Minimum level to log
 *
 * @note This function is thread-safe
 */
void cf_log_set_level(cf_log_level_t level);

/**
 * @brief Get global minimum log level
 *
 * @return Current minimum level
 *
 * @note This function is thread-safe
 */
cf_log_level_t cf_log_get_level(void);

/**
 * @brief Write log message
 *
 * @param[in] level Log level
 * @param[in] fmt Format string
 * @param[in] ... Format arguments
 *
 * @note This function is thread-safe
 */
void cf_log_write(cf_log_level_t level, const char* fmt, ...);

/**
 * @brief Convert log level to string
 *
 * @param[in] level Log level
 *
 * @return Level string (never NULL)
 */
const char* cf_log_level_to_string(cf_log_level_t level);

//==============================================================================
// SINK HELPER FUNCTIONS
//==============================================================================

/**
 * @brief Initialize base sink structure
 *
 * @param[out] sink Sink to initialize
 * @param[in] vtable Virtual table
 * @param[in] name Sink name
 * @param[in] min_level Minimum log level
 */
void cf_log_sink_init(cf_log_sink_t* sink,
                      const cf_log_sink_vtable_t* vtable,
                      const char* name,
                      cf_log_level_t min_level);

/**
 * @brief Check if sink should log this level
 *
 * @param[in] sink Sink to check
 * @param[in] level Log level
 *
 * @return true if should log, false otherwise
 */
bool cf_log_sink_should_log(const cf_log_sink_t* sink, cf_log_level_t level);

//==============================================================================
// LOGGING MACROS
//==============================================================================

#if CF_RTOS_ENABLED
    #include "os/cf_task.h"
    #define CF_LOG_HEADER "[%c][%16s][%s:%d] "
    #define CF_LOG_ARGS(level) \
        cf_log_level_to_string(level)[0], \
        cf_task_get_name(NULL), \
        __func__, \
        __LINE__
#else
    #define CF_LOG_HEADER "[%c][%s:%d] "
    #define CF_LOG_ARGS(level) \
        cf_log_level_to_string(level)[0], \
        __func__, \
        __LINE__
#endif

/**
 * @brief Trace log macro
 */
#define CF_LOG_T(fmt, ...) \
    cf_log_write(CF_LOG_TRACE, CF_LOG_HEADER fmt, CF_LOG_ARGS(CF_LOG_TRACE), ##__VA_ARGS__)

/**
 * @brief Debug log macro
 */
#define CF_LOG_D(fmt, ...) \
    cf_log_write(CF_LOG_DEBUG, CF_LOG_HEADER fmt, CF_LOG_ARGS(CF_LOG_DEBUG), ##__VA_ARGS__)

/**
 * @brief Info log macro
 */
#define CF_LOG_I(fmt, ...) \
    cf_log_write(CF_LOG_INFO, CF_LOG_HEADER fmt, CF_LOG_ARGS(CF_LOG_INFO), ##__VA_ARGS__)

/**
 * @brief Warning log macro
 */
#define CF_LOG_W(fmt, ...) \
    cf_log_write(CF_LOG_WARN, CF_LOG_HEADER fmt, CF_LOG_ARGS(CF_LOG_WARN), ##__VA_ARGS__)

/**
 * @brief Error log macro
 */
#define CF_LOG_E(fmt, ...) \
    cf_log_write(CF_LOG_ERROR, CF_LOG_HEADER fmt, CF_LOG_ARGS(CF_LOG_ERROR), ##__VA_ARGS__)

/**
 * @brief Fatal log macro
 */
#define CF_LOG_F(fmt, ...) \
    cf_log_write(CF_LOG_FATAL, CF_LOG_HEADER fmt, CF_LOG_ARGS(CF_LOG_FATAL), ##__VA_ARGS__)

#else

// Logging disabled
#define CF_LOG_T(fmt, ...) ((void)0)
#define CF_LOG_D(fmt, ...) ((void)0)
#define CF_LOG_I(fmt, ...) ((void)0)
#define CF_LOG_W(fmt, ...) ((void)0)
#define CF_LOG_E(fmt, ...) ((void)0)
#define CF_LOG_F(fmt, ...) ((void)0)

#endif /* CF_LOG_ENABLED */

#ifdef __cplusplus
}
#endif

#endif /* CF_LOG_H */
