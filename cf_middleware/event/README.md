# CFramework Event System

## 📋 Table of Contents
- [🎯 Overview](#-overview)
- [✨ Features](#-features)
- [🚀 Quick Start](#-quick-start)
- [🏗️ Event Domains](#%EF%B8%8F-event-domains)
- [📖 API Reference](#-api-reference)
- [💡 Usage Examples](#-usage-examples)
- [⚙️ Configuration](#%EF%B8%8F-configuration)
- [🔧 Advanced Topics](#-advanced-topics)
- [❓ FAQ & Troubleshooting](#-faq--troubleshooting)

---

## 🎯 Overview

**CFramework Event System** is a lightweight, thread-safe **Publish-Subscribe** implementation designed for embedded systems running FreeRTOS. It enables loose coupling between components through event-driven communication.

### Why Use Event System?

✅ **Decoupling**: Publishers don't need to know who subscribes  
✅ **Scalability**: Easy to add/remove event listeners  
✅ **Modularity**: Clean separation between components  
✅ **Thread-Safety**: Built-in protection for multi-threaded environments  

---

## ✨ Features

| Feature | Description |
|---------|-------------|
| **📡 Pub-Sub Pattern** | Publishers broadcast events, subscribers receive them |
| **⚡ Sync/Async Modes** | Choose immediate execution or ThreadPool dispatch |
| **🔒 Thread-Safe** | Mutex protection for all operations |
| **🏗️ Event Domains** | 32-bit hierarchical event organization (16-bit domain + 16-bit event) |
| **🛡️ Type Safety** | Compile-time type checking with validation macros |
| **🌟 Wildcard Subscription** | Subscribe to ALL events with `event_id = 0` |
| **💾 Memory Efficient** | Static allocation with configurable limits |
| **⚙️ Zero-Copy SYNC** | No data copying for synchronous callbacks |

### Current Limitations

- **Global subscriber limit**: `CF_EVENT_MAX_SUBSCRIBERS` (default: 32, max: 64)
- **Static memory**: Fixed allocation, no dynamic allocation
- **FIFO processing**: No priority queuing for async events

---

## 🚀 Quick Start

### Step 1: Initialize

```c
#include "event/cf_event.h"

int main(void) {
    // Initialize ThreadPool first (required for async events)
    cf_threadpool_init();
    
    // Initialize event system
    if (cf_event_init() != CF_OK) {
        // Handle initialization error
        return -1;
    }
    
    // Your application code...
    
    return 0;
}
```

### Step 2: Define Events

```c
// Simple event IDs
#define EVENT_BUTTON_PRESSED    0x0001
#define EVENT_TIMER_EXPIRED     0x0002
#define EVENT_DATA_READY        0x0003

// Or use domains (recommended for larger projects)
#define EVENT_USER_LOGIN        CF_EVENT_MAKE_ID(0x1000, 0x0001)
#define EVENT_USER_LOGOUT       CF_EVENT_MAKE_ID(0x1000, 0x0002)
#define EVENT_NETWORK_CONNECTED CF_EVENT_MAKE_ID(0x2000, 0x0001)
```

### Step 3: Subscribe to Events

```c
// Event handler function
void my_event_handler(cf_event_id_t event_id, const void* data, 
                      size_t data_size, void* user_data) {
    switch (event_id) {
        case EVENT_BUTTON_PRESSED:
            printf("Button was pressed!\n");
            break;
        case EVENT_DATA_READY:
            printf("Data ready: %u bytes\n", (unsigned)data_size);
            break;
    }
}

void setup_subscriptions(void) {
    // Subscribe to specific events
    CF_EVENT_SUBSCRIBE(EVENT_BUTTON_PRESSED, my_event_handler, NULL);
    CF_EVENT_SUBSCRIBE_ASYNC(EVENT_DATA_READY, my_event_handler, NULL);
    
    // Subscribe to ALL events (wildcard) - useful for logging
    CF_EVENT_SUBSCRIBE_ASYNC(0, debug_logger, NULL);
}
```

### Step 4: Publish Events

```c
// Publish simple event (no data)
cf_event_publish(EVENT_BUTTON_PRESSED);

// Publish event with data
typedef struct {
    uint8_t buffer[256];
    size_t length;
    uint32_t timestamp;
} data_packet_t;

data_packet_t packet = {
    .length = 128,
    .timestamp = get_system_time()
};

// Method 1: Manual size
cf_event_publish_data(EVENT_DATA_READY, &packet, sizeof(packet));

// Method 2: Type-safe macro (recommended)
CF_EVENT_PUBLISH_TYPE(EVENT_DATA_READY, &packet, data_packet_t);
```

---

## 🏗️ Event Domains

Event Domains provide hierarchical organization for better scalability and conflict prevention.

### Event ID Structure (32-bit)

```
┌─────────────────┬─────────────────┐
│  Domain (16bit) │  Event (16bit)  │
│     0x1000      │     0x0001      │
└─────────────────┴─────────────────┘
         │                 │
    Module/Component   Specific Event
```

### Domain Allocation

| Range | Purpose | Examples |
|-------|---------|----------|
| `0x0000-0x00FF` | **Framework Reserved** | System, Debug |
| `0x0100-0x0FFF` | **Application Domains** | UI, Business Logic |
| `0x1000-0xFFFF` | **Component Domains** | Hardware, Protocols |

### Using Domains

```c
#include "event/cf_event_types.h"

// Define domains
#define DOMAIN_USER_INTERFACE   0x0100
#define DOMAIN_NETWORK         0x1000
#define DOMAIN_STORAGE         0x2000

// Create domain events
#define EVENT_UI_BUTTON_CLICK   CF_EVENT_MAKE_ID(DOMAIN_USER_INTERFACE, 0x0001)
#define EVENT_UI_MENU_CHANGE    CF_EVENT_MAKE_ID(DOMAIN_USER_INTERFACE, 0x0002)

#define EVENT_NET_CONNECTED     CF_EVENT_MAKE_ID(DOMAIN_NETWORK, 0x0001)
#define EVENT_NET_DISCONNECTED  CF_EVENT_MAKE_ID(DOMAIN_NETWORK, 0x0002)
#define EVENT_NET_DATA_RX       CF_EVENT_MAKE_ID(DOMAIN_NETWORK, 0x0003)

// Extract information from event IDs
void event_handler(cf_event_id_t event_id, const void* data, size_t size, void* user) {
    uint16_t domain = CF_EVENT_GET_DOMAIN(event_id);
    uint16_t event_num = CF_EVENT_GET_EVENT(event_id);
    
    if (CF_EVENT_IS_DOMAIN(event_id, DOMAIN_NETWORK)) {
        // Handle all network events
        handle_network_event(event_num, data, size);
    }
}
```

### Type-Safe Events

```c
// Define typed event data structures
typedef struct {
    cf_event_header_t header;  // Optional common header
    char username[32];
    uint32_t user_id;
    bool is_admin;
} user_login_event_t;

typedef struct {
    cf_event_header_t header;
    uint8_t* data;
    size_t data_length;
    char source_address[16];
} network_data_event_t;

// Publisher: Type-safe publishing
user_login_event_t login_data = {
    .user_id = 12345,
    .is_admin = false
};
strcpy(login_data.username, "john_doe");

CF_EVENT_PUBLISH_TYPED(EVENT_UI_USER_LOGIN, &login_data, user_login_event_t);

// Subscriber: Type-safe receiving with validation
void user_handler(cf_event_id_t id, const void* data, size_t size, void* user) {
    CF_EVENT_CAST_DATA(event, data, size, user_login_event_t);
    if (event == NULL) return;  // Invalid data size
    
    printf("User %s (ID: %lu) logged in\n", event->username, event->user_id);
}
```

---

## 📖 API Reference

### Initialization

#### `cf_event_init()`
```c
cf_status_t cf_event_init(void);
```
Initialize the event system. Must be called after `cf_threadpool_init()`.

**Returns:**
- `CF_OK`: Success
- `CF_ERROR_ALREADY_INITIALIZED`: Already initialized
- `CF_ERROR_NO_MEMORY`: Failed to create mutex

#### `cf_event_deinit()`
```c
void cf_event_deinit(void);
```
Cleanup event system and unsubscribe all subscribers.

### Subscription

#### `cf_event_subscribe()`
```c
cf_status_t cf_event_subscribe(cf_event_id_t event_id,
                               cf_event_callback_t callback,
                               void* user_data,
                               cf_event_mode_t mode,
                               cf_event_subscriber_t* handle);
```

**Parameters:**
- `event_id`: Event to subscribe to (`0` = wildcard, all events)
- `callback`: Handler function
- `user_data`: User context passed to callback
- `mode`: `CF_EVENT_SYNC` (immediate) or `CF_EVENT_ASYNC` (ThreadPool)
- `handle`: Optional output handle for unsubscribing

**Convenience Macros:**
```c
CF_EVENT_SUBSCRIBE(event_id, callback, user_data)         // SYNC mode
CF_EVENT_SUBSCRIBE_ASYNC(event_id, callback, user_data)   // ASYNC mode
```

#### Callback Function Signature
```c
void my_callback(cf_event_id_t event_id,
                 const void* data,
                 size_t data_size,  
                 void* user_data);
```

### Publishing

#### `cf_event_publish()`
```c
cf_status_t cf_event_publish(cf_event_id_t event_id);
```
Publish event without data (signal-only).

#### `cf_event_publish_data()`
```c
cf_status_t cf_event_publish_data(cf_event_id_t event_id,
                                  const void* data,
                                  size_t data_size);
```
Publish event with data payload.

**Type-safe macro:**
```c
CF_EVENT_PUBLISH_TYPE(event_id, data_ptr, data_type)  // Auto sizeof
```

### Unsubscription

#### `cf_event_unsubscribe()`
```c
cf_status_t cf_event_unsubscribe(cf_event_subscriber_t handle);
```
Unsubscribe specific subscriber using handle.

#### `cf_event_unsubscribe_all()`
```c
uint32_t cf_event_unsubscribe_all(cf_event_id_t event_id);
```
Unsubscribe ALL subscribers for specific event. Returns number unsubscribed.

### Query Functions

#### `cf_event_get_subscriber_count()`
```c
uint32_t cf_event_get_subscriber_count(void);
```
Get total number of active subscribers.

#### `cf_event_get_event_subscriber_count()`
```c
uint32_t cf_event_get_event_subscriber_count(cf_event_id_t event_id);
```
Get number of subscribers for specific event (including wildcard subscribers).

---

## 💡 Usage Examples

### Example 1: Button and LED System

```c
#include "event/cf_event.h"

#define EVENT_BUTTON_PRESSED    0x0001
#define EVENT_BUTTON_RELEASED   0x0002

// LED component subscribes to button events
void led_event_handler(cf_event_id_t event_id, const void* data, 
                       size_t data_size, void* user_data) {
    switch (event_id) {
        case EVENT_BUTTON_PRESSED:
            led_turn_on();
            break;
        case EVENT_BUTTON_RELEASED:
            led_turn_off();
            break;
    }
}

// Initialize LED component
void led_component_init(void) {
    CF_EVENT_SUBSCRIBE(EVENT_BUTTON_PRESSED, led_event_handler, NULL);
    CF_EVENT_SUBSCRIBE(EVENT_BUTTON_RELEASED, led_event_handler, NULL);
}

// Button driver publishes events
void button_gpio_interrupt(void) {
    if (button_is_pressed()) {
        cf_event_publish(EVENT_BUTTON_PRESSED);
    } else {
        cf_event_publish(EVENT_BUTTON_RELEASED);
    }
}
```

### Example 2: Data Processing Pipeline

```c
#define EVENT_RAW_DATA_AVAILABLE   0x0010
#define EVENT_PROCESSED_DATA_READY 0x0011
#define EVENT_SAVE_DATA_REQUEST    0x0012

typedef struct {
    uint8_t buffer[512];
    size_t length;
    uint32_t timestamp;
    uint8_t source_id;
} data_packet_t;

// Data processor subscribes to raw data
void data_processor_handler(cf_event_id_t event_id, const void* data,
                            size_t data_size, void* user_data) {
    if (event_id != EVENT_RAW_DATA_AVAILABLE) return;
    
    CF_EVENT_CAST_DATA(packet, data, data_size, data_packet_t);
    if (packet == NULL) return;
    
    // Process the data
    data_packet_t processed = *packet;
    apply_filters(&processed);
    apply_calibration(&processed);
    
    // Publish processed data
    CF_EVENT_PUBLISH_TYPED(EVENT_PROCESSED_DATA_READY, &processed, data_packet_t);
}

// Storage component subscribes to processed data  
void storage_handler(cf_event_id_t event_id, const void* data,
                     size_t data_size, void* user_data) {
    if (event_id != EVENT_PROCESSED_DATA_READY) return;
    
    CF_EVENT_CAST_DATA(packet, data, data_size, data_packet_t);
    if (packet == NULL) return;
    
    // Save to storage
    save_to_flash(packet->buffer, packet->length);
}

void setup_pipeline(void) {
    // Async processing to avoid blocking data source
    CF_EVENT_SUBSCRIBE_ASYNC(EVENT_RAW_DATA_AVAILABLE, data_processor_handler, NULL);
    CF_EVENT_SUBSCRIBE_ASYNC(EVENT_PROCESSED_DATA_READY, storage_handler, NULL);
}
```

### Example 3: Multi-Component System with Domains

```c
// Define domains
#define DOMAIN_SENSORS     0x1000
#define DOMAIN_NETWORK     0x2000
#define DOMAIN_USER_IF     0x0100

// Define events
#define EVENT_SENSOR_TEMP_UPDATE    CF_EVENT_MAKE_ID(DOMAIN_SENSORS, 0x0001)
#define EVENT_SENSOR_PRESSURE_UPDATE CF_EVENT_MAKE_ID(DOMAIN_SENSORS, 0x0002)
#define EVENT_NET_DATA_TX           CF_EVENT_MAKE_ID(DOMAIN_NETWORK, 0x0001)
#define EVENT_UI_DISPLAY_UPDATE     CF_EVENT_MAKE_ID(DOMAIN_USER_IF, 0x0001)

typedef struct {
    float temperature;
    float humidity;
    uint32_t timestamp;
} sensor_reading_t;

// Network component aggregates sensor data
void network_component_handler(cf_event_id_t event_id, const void* data,
                              size_t data_size, void* user_data) {
    if (!CF_EVENT_IS_DOMAIN(event_id, DOMAIN_SENSORS)) {
        return;  // Only handle sensor domain events
    }
    
    // Aggregate sensor data and send over network
    static sensor_reading_t readings[10];
    static uint8_t count = 0;
    
    CF_EVENT_CAST_DATA(reading, data, data_size, sensor_reading_t);
    if (reading != NULL) {
        readings[count++] = *reading;
        
        if (count >= 10) {
            // Send batch over network
            transmit_sensor_batch(readings, count);
            count = 0;
        }
    }
}

// UI component displays sensor data
void ui_component_handler(cf_event_id_t event_id, const void* data,
                         size_t data_size, void* user_data) {
    if (CF_EVENT_IS_DOMAIN(event_id, DOMAIN_SENSORS)) {
        CF_EVENT_CAST_DATA(reading, data, data_size, sensor_reading_t);
        if (reading != NULL) {
            update_display(reading->temperature, reading->humidity);
        }
    }
}

void setup_components(void) {
    // Network subscribes to all sensor events
    CF_EVENT_SUBSCRIBE_ASYNC(0, network_component_handler, NULL);
    
    // UI subscribes to all sensor events  
    CF_EVENT_SUBSCRIBE_ASYNC(0, ui_component_handler, NULL);
}
```

---

## ⚙️ Configuration

Configure the event system in your `cf_user_config.h`:

```c
// Enable event system (requires RTOS)
#define CF_EVENT_ENABLED            1
#define CF_RTOS_ENABLED             1

// Subscriber limits
#define CF_EVENT_MAX_SUBSCRIBERS    32    // Default: 32, Max: 64, Min: 4

// Dependencies for async events
#define CF_THREADPOOL_ENABLED       1
#define CF_THREADPOOL_THREAD_COUNT  4
#define CF_THREADPOOL_QUEUE_SIZE    20

// Optional: Enable logging for debugging
#define CF_LOG_ENABLED              1
```

### Memory Usage

```c
// Per subscriber: ~20 bytes
// Total static memory: CF_EVENT_MAX_SUBSCRIBERS * 20 bytes
// Default (32 subscribers): ~640 bytes

// Async event overhead per publish:
// Context: ~24 bytes + data_size (copied)
// Freed automatically after callback execution
```

---

## 🔧 Advanced Topics

### SYNC vs ASYNC Mode Comparison

| Aspect | SYNC Mode | ASYNC Mode |
|--------|-----------|------------|
| **Execution** | Immediate in publisher's thread | Delayed in ThreadPool worker |
| **Data Handling** | Zero-copy (pointer passed) | Copied to heap |
| **Performance** | Lowest latency | Higher latency |
| **Safety** | Can block publisher | Non-blocking |
| **Memory** | No additional allocation | Allocates context + data copy |
| **Use Cases** | ISR notifications, Critical events | Heavy processing, I/O operations |

### Best Practices

#### ✅ DO

```c
// Use domains for organization
#define EVENT_UI_BUTTON_CLICK  CF_EVENT_MAKE_ID(0x0100, 0x0001)

// Validate event data in handlers
void handler(cf_event_id_t id, const void* data, size_t size, void* user) {
    CF_EVENT_CAST_DATA(evt, data, size, my_event_t);
    if (evt == NULL) return;  // Invalid data
    
    // Safe to use evt->field
}

// Use ASYNC for heavy processing
CF_EVENT_SUBSCRIBE_ASYNC(EVENT_HEAVY_CALCULATION, handler, NULL);

// Keep event data structures small
typedef struct {
    uint32_t value;
    uint16_t id;
} lightweight_event_t;
```

#### ❌ DON'T

```c
// Don't publish from SYNC callbacks (deadlock risk)
void bad_handler(cf_event_id_t id, const void* data, size_t size, void* user) {
    cf_event_publish(ANOTHER_EVENT);  // DANGEROUS!
}

// Don't use large data structures for frequent events
typedef struct {
    uint8_t huge_buffer[4096];  // Will be copied for async!
} bad_event_t;

// Don't access event data after callback returns
static const my_event_t* saved_event;  // DANGEROUS!
void handler(..., const void* data, ...) {
    saved_event = (const my_event_t*)data;  // Data invalid after return!
}
```

### Performance Considerations

- **Subscriber Limit**: Global limit of 32-64 subscribers total (not per event)
- **Memory Pool**: Static allocation prevents fragmentation but limits flexibility
- **ThreadPool Dependency**: Async events require ThreadPool to be initialized
- **Mutex Overhead**: All operations are protected by mutex

---

## ❓ FAQ & Troubleshooting

### Q: Event handler not called?

**Check:**
1. Event system initialized? `cf_event_init()`
2. ThreadPool initialized for async events? `cf_threadpool_init()`
3. Correct event ID used in both subscribe and publish?
4. Reached subscriber limit? Check `cf_event_get_subscriber_count()`

```c
// Debug: Check subscriber count
uint32_t total = cf_event_get_subscriber_count();
uint32_t for_event = cf_event_get_event_subscriber_count(MY_EVENT);
printf("Total: %lu, For event: %lu\n", total, for_event);
```

### Q: Data corruption in event handler?

**Check:**
1. Event data size mismatch?
2. Using data after callback returns? (Only valid during callback)
3. Struct packing issues?

```c
// Use validation macros
void handler(cf_event_id_t id, const void* data, size_t size, void* user) {
    CF_EVENT_VALIDATE_DATA(data, size, my_event_t);  // Validates size
    // OR
    CF_EVENT_CAST_DATA(evt, data, size, my_event_t);  // Returns NULL if invalid
}
```

### Q: System hanging or deadlock?

**Check:**
1. Publishing from SYNC callback? (Mutex deadlock)
2. Heavy processing in SYNC handler? (Blocks publisher)
3. ThreadPool queue full? (Async events dropped)

```c
// Use ASYNC for heavy work
CF_EVENT_SUBSCRIBE_ASYNC(EVENT_HEAVY_WORK, handler, NULL);

// Don't publish from SYNC handler
void sync_handler(...) {
    // cf_event_publish(OTHER_EVENT);  // DON'T DO THIS
    
    // Instead, defer to async context
    schedule_delayed_publish(OTHER_EVENT);
}
```

### Q: Memory leaks with async events?

**Cause:** Framework automatically allocates and frees memory for async events. If you see leaks:

1. Check ThreadPool is properly processing tasks
2. Verify no infinite event loops
3. Monitor with heap debugging tools

### Q: How to unsubscribe all events for a component?

```c
// Method 1: Store handles during subscription
cf_event_subscriber_t handles[5];
cf_event_subscribe(EVENT_1, handler, NULL, CF_EVENT_ASYNC, &handles[0]);
cf_event_subscribe(EVENT_2, handler, NULL, CF_EVENT_ASYNC, &handles[1]);

// Cleanup
for (int i = 0; i < 5; i++) {
    cf_event_unsubscribe(handles[i]);
}

// Method 2: Unsubscribe all for specific events
cf_event_unsubscribe_all(EVENT_1);
cf_event_unsubscribe_all(EVENT_2);
```

---

**Version:** 1.0.0  
**License:** MIT  
**Dependencies:** cf_common, cf_mutex, cf_threadpool (for async), cf_log (optional)