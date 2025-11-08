/**
 * @file cf_event_domain_rain.h
 * @brief Rain Sensor Event Domain
 * @version 1.0.0
 * @date 2025-11-01
 * @author CFramework
 *
 * Event domain for rain sensor manager and applications
 */

#ifndef CF_EVENT_DOMAIN_RAIN_H
#define CF_EVENT_DOMAIN_RAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cf_event.h"
#include "cf_event_types.h"

#ifdef ESP_PLATFORM
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
#else
    #include "FreeRTOS.h"
    #include "task.h"
#endif

//==============================================================================
// DOMAIN DEFINITION
//==============================================================================

/**
 * @brief Rain sensor domain ID (Manager domain range)
 */
#define CF_EVENT_DOMAIN_RAIN        0x1000

/* Compile-time validation */
CF_EVENT_DOMAIN_ASSERT_RANGE(CF_EVENT_DOMAIN_RAIN);

//==============================================================================
// EVENT DEFINITIONS
//==============================================================================

/**
 * @brief Rain sensor tipping bucket event
 *
 * Published when rain sensor detects a tip (typically 0.2mm rainfall)
 */
#define EVENT_RAIN_TIPPING      CF_EVENT_MAKE_ID(CF_EVENT_DOMAIN_RAIN, 0x0001)

/**
 * @brief Rain sensor error event
 *
 * Published when sensor detects hardware error or malfunction
 */
#define EVENT_RAIN_ERROR        CF_EVENT_MAKE_ID(CF_EVENT_DOMAIN_RAIN, 0x00FF)

//==============================================================================
// EVENT DATA STRUCTURES
//==============================================================================

/**
 * @brief Rain tipping event data
 *
 * Contains information about the rain tip event
 */
typedef struct {
    cf_event_header_t header;       /**< Common event header */

    uint32_t tipping_count;         /**< Total number of tips since reset */
    float rainfall_mm;              /**< Total rainfall in mm */
    uint32_t interval_ms;           /**< Time since last tip (ms) */
    uint32_t timestamp;             /**< System timestamp (tick count) */
} rain_tipping_event_t;

/**
 * @brief Rain sensor error event data
 */
typedef struct {
    cf_event_header_t header;       /**< Common event header */

    uint32_t error_code;            /**< Error code */
    const char* error_msg;          /**< Error message */
    uint32_t timestamp;             /**< System timestamp */
} rain_error_event_t;

//==============================================================================
// HELPER FUNCTIONS
//==============================================================================

/**
 * @brief Initialize rain tipping event structure
 *
 * @param[out] event Pointer to event structure
 * @param[in] count Total tipping count
 * @param[in] rainfall Total rainfall in mm
 * @param[in] interval Interval since last tip (ms)
 */
static inline void rain_tipping_event_init(rain_tipping_event_t* event,
                                           uint32_t count,
                                           float rainfall,
                                           uint32_t interval)
{
    if (event) {
        event->header.timestamp = xTaskGetTickCount();
        event->header.priority = 0;
        event->header.sequence = count;

        event->tipping_count = count;
        event->rainfall_mm = rainfall;
        event->interval_ms = interval;
        event->timestamp = event->header.timestamp;
    }
}

/**
 * @brief Initialize rain error event structure
 *
 * @param[out] event Pointer to event structure
 * @param[in] error_code Error code
 * @param[in] error_msg Error message string
 */
static inline void rain_error_event_init(rain_error_event_t* event,
                                         uint32_t error_code,
                                         const char* error_msg)
{
    if (event) {
        event->header.timestamp = xTaskGetTickCount();
        event->header.priority = 1;  /* High priority for errors */
        event->header.sequence = 0;

        event->error_code = error_code;
        event->error_msg = error_msg ? error_msg : "Unknown error";
        event->timestamp = event->header.timestamp;
    }
}

#ifdef __cplusplus
}
#endif

#endif /* CF_EVENT_DOMAIN_RAIN_H */
