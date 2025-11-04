/**
 * @file cf_event_types.h
 * @brief Common event data types and utilities
 * @version 1.0.0
 * @date 2025-11-01
 * @author CFramework Contributors
 *
 * @copyright Copyright (c) 2025 CFramework
 * Licensed under MIT License
 *
 * @note This file provides common types and macros for event data structures.
 *       Users should define their own event domains using these utilities.
 */

#ifndef CF_EVENT_TYPES_H
#define CF_EVENT_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cf_common.h"
#include <stdint.h>
#include <stdbool.h>

#if CF_EVENT_ENABLED

//==============================================================================
// DOMAIN ID RANGES
//==============================================================================

/**
 * @brief Domain ID bit allocation
 *
 * Event ID structure (32-bit):
 * ┌─────────────────┬─────────────────┐
 * │  Domain (16bit) │  Event (16bit)  │
 * │   0xXXXX        │   0xXXXX        │
 * └─────────────────┴─────────────────┘
 *
 * Domain ranges (recommended):
 * 0x0000-0x00FF : Reserved by framework
 * 0x0100-0x0FFF : User application domains
 * 0x1000-0xFFFF : Manager/driver domains
 */

#define CF_EVENT_DOMAIN_SHIFT           16
#define CF_EVENT_ID_MASK                0x0000FFFF
#define CF_EVENT_DOMAIN_MASK            0xFFFF0000

/**
 * @brief Reserved domains (do not use in user code)
 */
#define CF_EVENT_DOMAIN_RESERVED        0x0000  /**< Reserved for framework */
#define CF_EVENT_DOMAIN_SYSTEM          0x0001  /**< System events */

/**
 * @brief User domain range start
 */
#define CF_EVENT_DOMAIN_USER_START      0x0100
#define CF_EVENT_DOMAIN_USER_END        0x0FFF

/**
 * @brief Manager domain range start (for hardware managers)
 */
#define CF_EVENT_DOMAIN_MANAGER_START   0x1000
#define CF_EVENT_DOMAIN_MANAGER_END     0xFFFF

//==============================================================================
// EVENT ID CONSTRUCTION MACROS
//==============================================================================

/**
 * @brief Construct event ID from domain and event number
 *
 * @param domain Domain ID (0x0000-0xFFFF)
 * @param event Event number within domain (0x0000-0xFFFF)
 * @return Complete event ID
 *
 * @example
 * #define MY_DOMAIN 0x1000
 * #define EVENT_SENSOR_UPDATE  CF_EVENT_MAKE_ID(MY_DOMAIN, 0x0001)
 */
#define CF_EVENT_MAKE_ID(domain, event) \
    (((uint32_t)(domain) << CF_EVENT_DOMAIN_SHIFT) | ((uint32_t)(event) & CF_EVENT_ID_MASK))

/**
 * @brief Extract domain ID from event ID
 */
#define CF_EVENT_GET_DOMAIN(event_id) \
    (((event_id) & CF_EVENT_DOMAIN_MASK) >> CF_EVENT_DOMAIN_SHIFT)

/**
 * @brief Extract event number from event ID
 */
#define CF_EVENT_GET_EVENT(event_id) \
    ((event_id) & CF_EVENT_ID_MASK)

/**
 * @brief Check if event belongs to a domain
 */
#define CF_EVENT_IS_DOMAIN(event_id, domain) \
    (CF_EVENT_GET_DOMAIN(event_id) == (domain))

//==============================================================================
// COMMON EVENT DATA TYPES
//==============================================================================

/**
 * @brief Event timestamp type
 */
typedef uint32_t cf_event_timestamp_t;

/**
 * @brief Event priority levels (for future use)
 */
typedef enum {
    CF_EVENT_PRIORITY_LOW       = 0,
    CF_EVENT_PRIORITY_NORMAL    = 1,
    CF_EVENT_PRIORITY_HIGH      = 2,
    CF_EVENT_PRIORITY_CRITICAL  = 3
} cf_event_priority_t;

/**
 * @brief Generic event header (optional to use)
 * Users can embed this at the start of their event data structures
 * to have common fields across all events
 */
typedef struct {
    cf_event_timestamp_t timestamp;     /**< Event timestamp (HAL_GetTick or custom) */
    uint16_t sequence;                  /**< Sequence number for ordering */
    uint8_t priority;                   /**< Event priority */
    uint8_t reserved;                   /**< Reserved for future use */
} cf_event_header_t;

//==============================================================================
// TYPE-SAFE EVENT PUBLISHING MACROS
//==============================================================================

/**
 * @brief Publish typed event with automatic sizeof
 *
 * @param event_id Event identifier
 * @param data_ptr Pointer to event data structure
 * @param data_type Type of event data (for sizeof)
 *
 * @example
 * typedef struct { float temp; } sensor_event_t;
 * sensor_event_t data = {25.5f};
 * CF_EVENT_PUBLISH_TYPED(EVENT_SENSOR_TEMP, &data, sensor_event_t);
 */
#define CF_EVENT_PUBLISH_TYPED(event_id, data_ptr, data_type) \
    cf_event_publish_data(event_id, data_ptr, sizeof(data_type))

/**
 * @brief Subscribe with typed callback helper
 * Note: Callback still receives void* but this macro adds documentation
 */
#define CF_EVENT_SUBSCRIBE_TYPED(event_id, callback, user_data, data_type) \
    CF_EVENT_SUBSCRIBE_ASYNC(event_id, callback, user_data)

/**
 * @brief Validate event data size in callback
 *
 * @example
 * void handler(cf_event_id_t id, const void* data, size_t size, void* user) {
 *     CF_EVENT_VALIDATE_DATA(data, size, sensor_event_t);
 *     const sensor_event_t* evt = (const sensor_event_t*)data;
 *     // Use evt safely
 * }
 */
#define CF_EVENT_VALIDATE_DATA(data, size, expected_type) \
    do { \
        if ((data) == NULL || (size) != sizeof(expected_type)) { \
            CF_LOG_E("Invalid event data: expected size %u, got %u", \
                     sizeof(expected_type), (size)); \
            return; \
        } \
    } while(0)

/**
 * @brief Cast event data with validation
 *
 * @example
 * void handler(cf_event_id_t id, const void* data, size_t size, void* user) {
 *     CF_EVENT_CAST_DATA(evt, data, size, sensor_event_t);
 *     if (evt == NULL) return;
 *     printf("Temp: %.2f\n", evt->temp);
 * }
 */
#define CF_EVENT_CAST_DATA(var_name, data, size, type) \
    const type* var_name = NULL; \
    do { \
        if ((data) != NULL && (size) == sizeof(type)) { \
            var_name = (const type*)(data); \
        } else { \
            CF_LOG_E("Invalid event data cast to " #type); \
        } \
    } while(0)

//==============================================================================
// EVENT DOMAIN DECLARATION HELPERS
//==============================================================================

/**
 * @brief Begin event domain definition
 * Use in your domain header file
 *
 * @example
 * CF_EVENT_DOMAIN_BEGIN(SENSOR, 0x1000)
 *     CF_EVENT_DEFINE(RAIN_TIPPING, 0x01)
 *     CF_EVENT_DEFINE(WATER_CHANGED, 0x02)
 * CF_EVENT_DOMAIN_END(SENSOR)
 */
#define CF_EVENT_DOMAIN_BEGIN(name, domain_id) \
    enum { \
        CF_EVENT_DOMAIN_##name = (domain_id),

#define CF_EVENT_DEFINE(name, event_num) \
        CF_EVENT_##name = CF_EVENT_MAKE_ID(CF_EVENT_DOMAIN_##name, event_num),

#define CF_EVENT_DOMAIN_END(name) \
        CF_EVENT_DOMAIN_##name##_COUNT \
    };

//==============================================================================
// COMPILE-TIME ASSERTIONS
//==============================================================================

/**
 * @brief Validate domain ID at compile time
 */
#define CF_EVENT_DOMAIN_ASSERT_RANGE(domain) \
    _Static_assert((domain) >= CF_EVENT_DOMAIN_USER_START || \
                   (domain) >= CF_EVENT_DOMAIN_MANAGER_START, \
                   "Domain ID out of valid range")

#endif /* CF_EVENT_ENABLED */

#ifdef __cplusplus
}
#endif

#endif /* CF_EVENT_TYPES_H */
