# Event Domain Development Guide

## 📋 Mục lục
- [Giới thiệu](#giới-thiệu)
- [Kiến trúc Domain](#kiến-trúc-domain)
- [Bước 1: Lập kế hoạch Domain](#bước-1-lập-kế-hoạch-domain)
- [Bước 2: Tạo Domain Header](#bước-2-tạo-domain-header)
- [Bước 3: Define Events và Data Structures](#bước-3-define-events-và-data-structures)
- [Bước 4: Implement Publisher (Manager)](#bước-4-implement-publisher-manager)
- [Bước 5: Implement Subscribers (Apps)](#bước-5-implement-subscribers-apps)
- [Best Practices](#best-practices)
- [Troubleshooting](#troubleshooting)

---

## Giới thiệu

**Event Domains** là cách tổ chức events theo nhóm chức năng (sensor, cellular, lora, ...) để:
- ✅ Dễ maintain và mở rộng
- ✅ Tránh conflict event IDs
- ✅ Type-safe với typed data structures
- ✅ Apps chỉ include domains cần dùng

**Framework cung cấp:**
- `cf_event_types.h` - Common types và macros
- `cf_event_domain_template.h` - Template để copy
- `domains/` - Example domains (sensor, cellular, system)

**Developer cần làm:**
- Tạo domain headers cho managers/apps của họ
- Define events và data structures
- Implement publisher/subscriber code

---

## Kiến trúc Domain

```
Project Structure:
├── CFramework/
│   └── cf_middleware/
│       └── event/
│           ├── cf_event.h                      ← Core event system
│           ├── cf_event_types.h                ← Common types & macros
│           ├── cf_event_domain_template.h      ← Template for users
│           └── domains/                        ← Example domains
│               ├── cf_event_domain_sensor.h
│               ├── cf_event_domain_cellular.h
│               └── cf_event_domain_system.h
│
└── YourProject/
    ├── Manager/                                ← Publishers
    │   ├── MgrSensor/
    │   │   ├── mgr_sensor.h
    │   │   ├── mgr_sensor.c
    │   │   └── mgr_sensor_events.h            ← YOUR domain header
    │   └── MgrCell/
    │       ├── mgr_cell.h
    │       ├── mgr_cell.c
    │       └── mgr_cell_events.h              ← YOUR domain header
    │
    └── Application/                            ← Subscribers
        ├── DataMonitor/
        │   ├── app_data_monitor.c             ← Subscribe to events
        │   └── app_data_monitor.h
        └── WarnSender/
            ├── app_warn_sender.c              ← Subscribe to events
            └── app_warn_sender.h
```

---

## Bước 1: Lập kế hoạch Domain

### 1.1. Xác định Domain cần tạo

Mỗi **Manager** hoặc **Module lớn** nên có domain riêng:

| Domain | ID Range | Mô tả |
|--------|----------|-------|
| Sensor Manager | 0x1000 | Rain, water level, temperature, humidity sensors |
| Cellular Manager | 0x2000 | SMS, calls, network status |
| LoRa Manager | 0x3000 | LoRa RX/TX, join, RSSI |
| Power Manager | 0x5000 | Battery level, charging status |
| FileSystem Manager | 0x4000 | File operations, SD card events |
| App DataMonitor | 0x0100 | Application-specific events |

### 1.2. Chọn Domain ID

**Quy tắc:**
- `0x0000 - 0x00FF`: Reserved by framework (DO NOT USE)
- `0x0100 - 0x0FFF`: User application domains
- `0x1000 - 0xFFFF`: Hardware manager domains

**Recommended ranges:**
```c
// Hardware Managers (0x1000+)
#define CF_EVENT_DOMAIN_SENSOR      0x1000
#define CF_EVENT_DOMAIN_CELLULAR    0x2000
#define CF_EVENT_DOMAIN_LORA        0x3000
#define CF_EVENT_DOMAIN_FS          0x4000
#define CF_EVENT_DOMAIN_POWER       0x5000

// User Applications (0x0100+)
#define CF_EVENT_DOMAIN_APP_DTMNT   0x0100
#define CF_EVENT_DOMAIN_APP_WARN    0x0200
```

### 1.3. Liệt kê Events cần thiết

Ví dụ cho Sensor Manager:

| Event Name | Event ID | Frequency | Data Type | Description |
|------------|----------|-----------|-----------|-------------|
| RAIN_TIPPING | 0x1001 | Variable | sensor_rain_event_t | Rain sensor tip detected |
| WATER_CHANGED | 0x1002 | 1 Hz | sensor_water_event_t | Water level changed |
| TEMP_UPDATED | 0x1003 | 0.1 Hz | sensor_temp_event_t | Temperature reading |
| RH_UPDATED | 0x1004 | 0.1 Hz | sensor_rh_event_t | Humidity reading |
| SENSOR_ERROR | 0x10FF | On error | sensor_error_event_t | Sensor malfunction |

---

## Bước 2: Tạo Domain Header

### 2.1. Copy Template

```bash
cd YourProject/Manager/MgrSensor/
cp CFramework/cf_middleware/event/cf_event_domain_template.h mgr_sensor_events.h
```

### 2.2. Customize Header

```c
/**
 * @file mgr_sensor_events.h
 * @brief Sensor Manager Event Domain
 */

#ifndef MGR_SENSOR_EVENTS_H
#define MGR_SENSOR_EVENTS_H

#include "event/cf_event.h"
#include "event/cf_event_types.h"

//==============================================================================
// DOMAIN DEFINITION
//==============================================================================

/**
 * @brief Sensor manager domain ID
 */
#define CF_EVENT_DOMAIN_SENSOR      0x1000

// Validate range (optional but recommended)
CF_EVENT_DOMAIN_ASSERT_RANGE(CF_EVENT_DOMAIN_SENSOR);

//==============================================================================
// EVENT DEFINITIONS
//==============================================================================

#define EVENT_SENSOR_RAIN_TIPPING   CF_EVENT_MAKE_ID(CF_EVENT_DOMAIN_SENSOR, 0x0001)
#define EVENT_SENSOR_WATER_CHANGED  CF_EVENT_MAKE_ID(CF_EVENT_DOMAIN_SENSOR, 0x0002)
#define EVENT_SENSOR_TEMP_UPDATED   CF_EVENT_MAKE_ID(CF_EVENT_DOMAIN_SENSOR, 0x0003)
#define EVENT_SENSOR_RH_UPDATED     CF_EVENT_MAKE_ID(CF_EVENT_DOMAIN_SENSOR, 0x0004)
#define EVENT_SENSOR_ERROR          CF_EVENT_MAKE_ID(CF_EVENT_DOMAIN_SENSOR, 0x00FF)

// ... (continue in next step)
#endif
```

---

## Bước 3: Define Events và Data Structures

### 3.1. Design Event Data Structure

**Best practices:**
- Use fixed-size types (`uint32_t`, `float`, `int8_t`)
- Keep structures small (will be copied for async events)
- Include `cf_event_header_t` if you want common fields
- Add clear comments for each field

```c
//==============================================================================
// EVENT DATA STRUCTURES
//==============================================================================

/**
 * @brief Rain sensor tipping event data
 *
 * Published by: mgr_sensor_update_rain_value()
 * Frequency: Variable (depends on rainfall)
 * Subscribers: DataMonitor, Logger, ServerLog apps
 */
typedef struct {
    cf_event_header_t header;       /**< Common header (timestamp, priority, etc.) */
    uint32_t tipping_count;         /**< Total tips since boot */
    uint32_t interval_ms;           /**< Time since last tip (ms) */
    float rainfall_mm;              /**< Calculated rainfall (mm) */
} sensor_rain_event_t;

/**
 * @brief Water level changed event data
 *
 * Published by: mgr_sensor_update_water_level()
 * Frequency: 1 Hz (continuous monitoring)
 * Subscribers: DataMonitor, WarnSender (for alarms)
 */
typedef struct {
    cf_event_header_t header;
    float water_level_m;            /**< Current level (meters) */
    float prev_level_m;             /**< Previous level (meters) */
    float rate_of_change;           /**< Change rate (m/s) */
    bool alarm_triggered;           /**< True if > threshold */
} sensor_water_event_t;

/**
 * @brief Temperature updated event data
 */
typedef struct {
    cf_event_header_t header;
    float temperature_c;            /**< Temperature in Celsius */
    int8_t sensor_id;               /**< Sensor ID (if multiple) */
} sensor_temp_event_t;

/**
 * @brief Relative humidity updated event data
 */
typedef struct {
    cf_event_header_t header;
    float humidity_percent;         /**< RH 0-100% */
    int8_t sensor_id;               /**< Sensor ID */
} sensor_rh_event_t;

/**
 * @brief Sensor error event data
 */
typedef struct {
    cf_event_header_t header;
    uint8_t sensor_type;            /**< 1=rain, 2=water, 3=temp, 4=rh */
    uint16_t error_code;            /**< Error code */
    char description[64];           /**< Error message */
} sensor_error_event_t;
```

### 3.2. Add Helper Functions (Optional)

```c
//==============================================================================
// HELPER FUNCTIONS
//==============================================================================

/**
 * @brief Initialize rain event with default values
 */
static inline void sensor_rain_event_init(sensor_rain_event_t* event) {
    if (event != NULL) {
        event->header.timestamp = HAL_GetTick();
        event->header.sequence = 0;
        event->header.priority = CF_EVENT_PRIORITY_NORMAL;
        event->tipping_count = 0;
        event->interval_ms = 0;
        event->rainfall_mm = 0.0f;
    }
}

/**
 * @brief Initialize water event with default values
 */
static inline void sensor_water_event_init(sensor_water_event_t* event) {
    if (event != NULL) {
        event->header.timestamp = HAL_GetTick();
        event->header.priority = CF_EVENT_PRIORITY_HIGH;  // Water alarm is critical
        event->water_level_m = 0.0f;
        event->prev_level_m = 0.0f;
        event->rate_of_change = 0.0f;
        event->alarm_triggered = false;
    }
}
```

---

## Bước 4: Implement Publisher (Manager)

### 4.1. Manager Code Structure

```c
// mgr_sensor.c
#include "mgr_sensor.h"
#include "mgr_sensor_events.h"  // ← Include your domain header
#include "event/cf_event.h"

// Internal state
static uint32_t g_tipping_count = 0;
static uint32_t g_last_tip_time = 0;

/**
 * @brief Called from GPIO IRQ when rain sensor tips
 */
void mgr_sensor_on_rain_tip_irq(void) {
    // Update internal state
    uint32_t now = HAL_GetTick();
    uint32_t interval = now - g_last_tip_time;
    g_tipping_count++;
    g_last_tip_time = now;

    // Prepare event data
    sensor_rain_event_t event;
    sensor_rain_event_init(&event);
    event.tipping_count = g_tipping_count;
    event.interval_ms = interval;
    event.rainfall_mm = g_tipping_count * 0.2794f;  // 0.2794mm per tip

    // Publish event to all subscribers
    cf_status_t status = CF_EVENT_PUBLISH_TYPED(EVENT_SENSOR_RAIN_TIPPING,
                                                 &event,
                                                 sensor_rain_event_t);

    if (status != CF_OK) {
        MGR_LOGE("Failed to publish rain event: %d", status);
    }
}
```

### 4.2. Publishing Patterns

#### **Pattern A: Publish from ISR** (Fast, no blocking)
```c
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == RAIN_SENSOR_Pin) {
        // Publish from ISR - use ASYNC mode
        sensor_rain_event_t event = {
            .tipping_count = ++g_count,
            .interval_ms = HAL_GetTick() - g_last_tick
        };

        CF_EVENT_PUBLISH_TYPED(EVENT_SENSOR_RAIN_TIPPING, &event, sensor_rain_event_t);
    }
}
```

#### **Pattern B: Publish from Task** (Can do heavy processing)
```c
void task_sensor_update_value(void* arg) {
    // Read sensor
    float water_level = adc_read_water_level();

    // Process data
    static float prev_level = 0.0f;
    float rate = (water_level - prev_level) / 1.0f;  // m/s
    bool alarm = (water_level > THRESHOLD);

    // Prepare event
    sensor_water_event_t event;
    sensor_water_event_init(&event);
    event.water_level_m = water_level;
    event.prev_level_m = prev_level;
    event.rate_of_change = rate;
    event.alarm_triggered = alarm;

    // Publish
    CF_EVENT_PUBLISH_TYPED(EVENT_SENSOR_WATER_CHANGED, &event, sensor_water_event_t);

    prev_level = water_level;
}
```

---

## Bước 5: Implement Subscribers (Apps)

### 5.1. App Code Structure

```c
// app_data_monitor.c
#include "app_data_monitor.h"
#include "mgr_sensor_events.h"  // ← Include domain header
#include "event/cf_event.h"

// App context
typedef struct {
    uint32_t total_rainfall_mm;
    float last_water_level;
} app_dtmnt_ctx_t;

static app_dtmnt_ctx_t g_ctx = {0};

/**
 * @brief Event handler for rain tipping
 */
static void dtmnt_on_rain_tipping(cf_event_id_t event_id,
                                   const void* data,
                                   size_t data_size,
                                   void* user_data)
{
    // Cast and validate data
    CF_EVENT_CAST_DATA(event, data, data_size, sensor_rain_event_t);
    if (event == NULL) return;

    app_dtmnt_ctx_t* ctx = (app_dtmnt_ctx_t*)user_data;

    // Process event
    ctx->total_rainfall_mm = event->rainfall_mm;

    LOGI("[DTMNT] Rain tipping #%lu, total: %.2f mm",
         event->tipping_count, event->rainfall_mm);

    // Save to SD card (non-blocking)
    save_rain_data_async(event);

    // Send to server (non-blocking)
    send_to_server_async(event);
}

/**
 * @brief Event handler for water level
 */
static void dtmnt_on_water_changed(cf_event_id_t event_id,
                                    const void* data,
                                    size_t data_size,
                                    void* user_data)
{
    CF_EVENT_CAST_DATA(event, data, data_size, sensor_water_event_t);
    if (event == NULL) return;

    app_dtmnt_ctx_t* ctx = (app_dtmnt_ctx_t*)user_data;
    ctx->last_water_level = event->water_level_m;

    // Check alarm
    if (event->alarm_triggered) {
        LOGW("[DTMNT] ALARM! Water level: %.2f m", event->water_level_m);
        trigger_alarm();
    }
}

/**
 * @brief App initialization - Subscribe to events
 */
void app_dtmnt_init(void) {
    // Subscribe to RAIN events (ASYNC mode)
    cf_status_t status = CF_EVENT_SUBSCRIBE_ASYNC(EVENT_SENSOR_RAIN_TIPPING,
                                                   dtmnt_on_rain_tipping,
                                                   &g_ctx);
    if (status != CF_OK) {
        LOGE("[DTMNT] Failed to subscribe RAIN: %d", status);
    }

    // Subscribe to WATER events (ASYNC mode)
    status = CF_EVENT_SUBSCRIBE_ASYNC(EVENT_SENSOR_WATER_CHANGED,
                                      dtmnt_on_water_changed,
                                      &g_ctx);
    if (status != CF_OK) {
        LOGE("[DTMNT] Failed to subscribe WATER: %d", status);
    }

    LOGI("[DTMNT] Initialized, subscribed to sensor events");
}

/**
 * @brief App cleanup (optional)
 */
void app_dtmnt_deinit(void) {
    // Unsubscribe all (if needed)
    cf_event_unsubscribe_all(EVENT_SENSOR_RAIN_TIPPING);
    cf_event_unsubscribe_all(EVENT_SENSOR_WATER_CHANGED);
}
```

### 5.2. Subscription Patterns

#### **Pattern A: Simple Handler**
```c
static void my_handler(cf_event_id_t id, const void* data,
                       size_t size, void* user) {
    CF_EVENT_CAST_DATA(evt, data, size, sensor_rain_event_t);
    if (evt == NULL) return;

    printf("Rain: %lu tips\n", evt->tipping_count);
}

void app_init(void) {
    CF_EVENT_SUBSCRIBE_ASYNC(EVENT_SENSOR_RAIN_TIPPING, my_handler, NULL);
}
```

#### **Pattern B: Multi-Event Handler**
```c
static void sensor_event_handler(cf_event_id_t id, const void* data,
                                  size_t size, void* user) {
    switch(id) {
        case EVENT_SENSOR_RAIN_TIPPING: {
            CF_EVENT_CAST_DATA(evt, data, size, sensor_rain_event_t);
            handle_rain(evt);
            break;
        }
        case EVENT_SENSOR_WATER_CHANGED: {
            CF_EVENT_CAST_DATA(evt, data, size, sensor_water_event_t);
            handle_water(evt);
            break;
        }
    }
}

void app_init(void) {
    CF_EVENT_SUBSCRIBE_ASYNC(EVENT_SENSOR_RAIN_TIPPING, sensor_event_handler, NULL);
    CF_EVENT_SUBSCRIBE_ASYNC(EVENT_SENSOR_WATER_CHANGED, sensor_event_handler, NULL);
}
```

#### **Pattern C: Wildcard Logger**
```c
// Subscribe to ALL events in sensor domain
static void logger_all_sensor_events(cf_event_id_t id, const void* data,
                                      size_t size, void* user) {
    // Check if event belongs to sensor domain
    if (CF_EVENT_IS_DOMAIN(id, CF_EVENT_DOMAIN_SENSOR)) {
        LOGD("[LOGGER] Sensor event 0x%04X, size=%u", id, size);
        log_to_file(id, data, size);
    }
}

void app_logger_init(void) {
    // event_id = 0 means subscribe to ALL events
    CF_EVENT_SUBSCRIBE_ASYNC(0, logger_all_sensor_events, NULL);
}
```

---

## Best Practices

### ✅ DO (Nên làm)

#### 1. **Use meaningful names**
```c
// ✅ GOOD
#define EVENT_SENSOR_RAIN_TIPPING    0x10001001
typedef struct sensor_rain_event_t { ... };

// ❌ BAD
#define EVENT_123    0x10001001
typedef struct evt_t { ... };
```

#### 2. **Document events thoroughly**
```c
/**
 * @brief Water level changed event
 *
 * Published by: mgr_sensor_update_water_level()
 * Frequency: 1 Hz (continuous)
 * Subscribers: DataMonitor (logging), WarnSender (alarms)
 *
 * @note This event triggers alarm if water_level_m > THRESHOLD
 */
#define EVENT_SENSOR_WATER_CHANGED  CF_EVENT_MAKE_ID(0x1000, 0x0002)
```

#### 3. **Validate data in handler**
```c
// ✅ GOOD - Always validate
void handler(cf_event_id_t id, const void* data, size_t size, void* user) {
    CF_EVENT_CAST_DATA(evt, data, size, sensor_rain_event_t);
    if (evt == NULL) return;  // Invalid data

    // Safe to use evt
}
```

#### 4. **Use async mode for heavy processing**
```c
// ✅ GOOD - Heavy work in async mode
CF_EVENT_SUBSCRIBE_ASYNC(EVENT_SENSOR_RAIN_TIPPING, handler, NULL);

// ❌ BAD - SYNC blocks publisher
CF_EVENT_SUBSCRIBE(EVENT_SENSOR_RAIN_TIPPING, slow_handler, NULL);
```

#### 5. **Group related events in same domain**
```c
// ✅ GOOD - Sensor domain groups all sensor events
#define CF_EVENT_DOMAIN_SENSOR  0x1000
#define EVENT_SENSOR_RAIN       CF_EVENT_MAKE_ID(0x1000, 0x01)
#define EVENT_SENSOR_WATER      CF_EVENT_MAKE_ID(0x1000, 0x02)
#define EVENT_SENSOR_TEMP       CF_EVENT_MAKE_ID(0x1000, 0x03)
```

### ❌ DON'T (Không nên làm)

#### 1. **Don't use reserved domain IDs**
```c
// ❌ BAD - Reserved by framework
#define CF_EVENT_DOMAIN_MY_SENSOR  0x0001
```

#### 2. **Don't make event data too large**
```c
// ❌ BAD - Will be copied for async events
typedef struct {
    char log_buffer[4096];  // Too large!
} huge_event_t;

// ✅ GOOD - Pass pointer or index instead
typedef struct {
    uint32_t log_buffer_index;  // Index to shared buffer
} small_event_t;
```

#### 3. **Don't publish from SYNC callback**
```c
// ❌ BAD - Can cause deadlock
void handler(cf_event_id_t id, const void* data, size_t size, void* user) {
    cf_event_publish(EVENT_SENSOR_RAIN_TIPPING);  // DANGER!
}
```

#### 4. **Don't dereference data after callback**
```c
// ❌ BAD - Data only valid during callback
static const sensor_rain_event_t* saved_event;

void handler(cf_event_id_t id, const void* data, size_t size, void* user) {
    saved_event = (const sensor_rain_event_t*)data;  // DANGER!
}

void later() {
    printf("%lu", saved_event->tipping_count);  // CRASH!
}
```

---

## Troubleshooting

### ❓ Problem: Events not received

**Check:**
1. Did you initialize event system?
   ```c
   cf_event_init();
   ```

2. Did you subscribe correctly?
   ```c
   cf_status_t status = CF_EVENT_SUBSCRIBE_ASYNC(EVENT_ID, handler, NULL);
   if (status != CF_OK) {
       LOGE("Subscribe failed: %d", status);
   }
   ```

3. Is event ID correct?
   ```c
   // Check if event belongs to expected domain
   uint16_t domain = CF_EVENT_GET_DOMAIN(event_id);
   uint16_t event = CF_EVENT_GET_EVENT(event_id);
   ```

4. Reached max subscribers (32)?
   ```c
   uint32_t count = cf_event_get_subscriber_count();
   LOGI("Subscribers: %lu / 32", count);
   ```

### ❓ Problem: Data corruption

**Check:**
1. Data size mismatch
   ```c
   // In handler
   if (data_size != sizeof(my_event_t)) {
       LOGE("Size mismatch: expected %u, got %u",
            sizeof(my_event_t), data_size);
       return;
   }
   ```

2. Struct alignment issues
   ```c
   // Add packed attribute if needed
   typedef struct __attribute__((packed)) {
       uint8_t x;
       uint32_t y;
   } my_event_t;
   ```

### ❓ Problem: Events delayed

**Check:**
1. ThreadPool queue full?
   - Increase `CF_THREADPOOL_QUEUE_SIZE`

2. Too many async events?
   - Use SYNC mode for critical events
   - Increase worker threads

3. Callback taking too long?
   - Profile callback execution time
   - Move heavy work to separate task

---

**Version:** 1.0.0
**Last Updated:** 2025-11-01
**License:** MIT
