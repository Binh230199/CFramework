/**
 * @file cf_event_domain_template.h
 * @brief Template for creating custom event domains
 * @version 1.0.0
 * @date 2025-11-01
 * @author CFramework Contributors
 *
 * @copyright Copyright (c) 2025 CFramework
 * Licensed under MIT License
 *
 * @note INSTRUCTIONS:
 *       1. Copy this file and rename it to your domain (e.g., cf_event_domain_sensor.h)
 *       2. Replace TEMPLATE with your domain name (e.g., SENSOR)
 *       3. Define your domain ID in the recommended range
 *       4. Define your events and data structures
 *       5. Document each event clearly
 *
 * @example Typical usage after customization:
 *       #include "event/cf_event_domain_sensor.h"
 *
 *       // Subscribe
 *       CF_EVENT_SUBSCRIBE_ASYNC(EVENT_SENSOR_RAIN_TIPPING, handler, &ctx);
 *
 *       // Publish
 *       sensor_rain_event_t data = {.count = 10};
 *       CF_EVENT_PUBLISH_TYPED(EVENT_SENSOR_RAIN_TIPPING, &data, sensor_rain_event_t);
 */

#ifndef CF_EVENT_DOMAIN_TEMPLATE_H
#define CF_EVENT_DOMAIN_TEMPLATE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "event/cf_event.h"
#include "event/cf_event_types.h"

//==============================================================================
// DOMAIN DEFINITION
//==============================================================================

/**
 * @brief Template domain ID
 *
 * IMPORTANT: Choose appropriate range:
 * - User applications:  0x0100 - 0x0FFF
 * - Hardware managers:  0x1000 - 0xFFFF
 *
 * Example ranges:
 * - Sensor manager:     0x1000
 * - Cellular manager:   0x2000
 * - LoRa manager:       0x3000
 * - FileSystem manager: 0x4000
 * - Power manager:      0x5000
 * - User App 1:         0x0100
 * - User App 2:         0x0200
 */
#define CF_EVENT_DOMAIN_TEMPLATE        0x1000

/**
 * @brief Validate domain range (optional compile-time check)
 */
CF_EVENT_DOMAIN_ASSERT_RANGE(CF_EVENT_DOMAIN_TEMPLATE);

//==============================================================================
// EVENT DEFINITIONS
//==============================================================================

/**
 * @brief Template domain events
 *
 * Naming convention: EVENT_<DOMAIN>_<ACTION>_<OBJECT>
 * Examples:
 * - EVENT_SENSOR_RAIN_TIPPING
 * - EVENT_CELL_SMS_RECEIVED
 * - EVENT_LORA_TX_COMPLETE
 */

/* Method 1: Manual event ID definition (more control) */
#define EVENT_TEMPLATE_EXAMPLE_1        CF_EVENT_MAKE_ID(CF_EVENT_DOMAIN_TEMPLATE, 0x0001)
#define EVENT_TEMPLATE_EXAMPLE_2        CF_EVENT_MAKE_ID(CF_EVENT_DOMAIN_TEMPLATE, 0x0002)
#define EVENT_TEMPLATE_EXAMPLE_3        CF_EVENT_MAKE_ID(CF_EVENT_DOMAIN_TEMPLATE, 0x0003)

/* Method 2: Using helper macros (cleaner, recommended) */
// CF_EVENT_DOMAIN_BEGIN(TEMPLATE, CF_EVENT_DOMAIN_TEMPLATE)
//     CF_EVENT_DEFINE(EXAMPLE_1, 0x0001)
//     CF_EVENT_DEFINE(EXAMPLE_2, 0x0002)
//     CF_EVENT_DEFINE(EXAMPLE_3, 0x0003)
// CF_EVENT_DOMAIN_END(TEMPLATE)

//==============================================================================
// EVENT DATA STRUCTURES
//==============================================================================

/**
 * @brief Example event data structure 1
 *
 * Best practices:
 * - Use descriptive names
 * - Add comments for each field
 * - Consider including cf_event_header_t for common fields
 * - Keep structures small (they will be copied for async events)
 * - Use fixed-size types (uint32_t, float, etc.)
 */
typedef struct {
    cf_event_header_t header;       /**< Common header (optional) */
    uint32_t counter;               /**< Example counter value */
    float value;                    /**< Example measurement value */
} template_event_1_t;

/**
 * @brief Example event data structure 2
 *
 * This structure demonstrates event with no common header
 */
typedef struct {
    uint8_t status;                 /**< Status code */
    uint16_t error_code;            /**< Error code if status indicates error */
    char message[64];               /**< Optional status message */
} template_event_2_t;

/**
 * @brief Example event data structure 3 (simple)
 *
 * For simple events, you can use basic types
 */
typedef struct {
    bool enabled;                   /**< Enable/disable flag */
} template_event_3_t;

//==============================================================================
// HELPER FUNCTIONS (Optional)
//==============================================================================

/**
 * @brief Initialize event data with default values
 *
 * Optional: Provide helper functions for common operations
 */
static inline void template_event_1_init(template_event_1_t* event) {
    if (event != NULL) {
        event->header.timestamp = HAL_GetTick();
        event->header.sequence = 0;
        event->header.priority = CF_EVENT_PRIORITY_NORMAL;
        event->counter = 0;
        event->value = 0.0f;
    }
}

//==============================================================================
// DOCUMENTATION
//==============================================================================

/**
 * @page template_events Template Domain Events
 *
 * @section template_overview Overview
 * Brief description of what this domain handles.
 *
 * @section template_events_list Events List
 *
 * @subsection EVENT_TEMPLATE_EXAMPLE_1
 * **Event ID:** 0x10000001
 * **Data Type:** template_event_1_t
 * **Published by:** Template Manager
 * **Description:** Triggered when example condition 1 occurs
 * **Frequency:** Up to 10 Hz
 *
 * @subsection EVENT_TEMPLATE_EXAMPLE_2
 * **Event ID:** 0x10000002
 * **Data Type:** template_event_2_t
 * **Published by:** Template Manager
 * **Description:** Status update event
 * **Frequency:** On change
 *
 * @section template_usage Usage Examples
 *
 * @subsection template_subscribe Subscribing to Events
 * @code
 * void my_handler(cf_event_id_t event_id, const void* data,
 *                 size_t data_size, void* user_data) {
 *     CF_EVENT_CAST_DATA(evt, data, data_size, template_event_1_t);
 *     if (evt == NULL) return;
 *
 *     printf("Counter: %lu, Value: %.2f\n", evt->counter, evt->value);
 * }
 *
 * void app_init(void) {
 *     CF_EVENT_SUBSCRIBE_ASYNC(EVENT_TEMPLATE_EXAMPLE_1, my_handler, NULL);
 * }
 * @endcode
 *
 * @subsection template_publish Publishing Events
 * @code
 * void template_manager_trigger_event(void) {
 *     template_event_1_t event;
 *     template_event_1_init(&event);
 *     event.counter = 42;
 *     event.value = 3.14f;
 *
 *     CF_EVENT_PUBLISH_TYPED(EVENT_TEMPLATE_EXAMPLE_1, &event, template_event_1_t);
 * }
 * @endcode
 */

#ifdef __cplusplus
}
#endif

#endif /* CF_EVENT_DOMAIN_TEMPLATE_H */
