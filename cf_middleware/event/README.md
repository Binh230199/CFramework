# CFramework Event System

## 📋 Mục lục
- [Giới thiệu](#giới-thiệu)
- [Event Domains](#event-domains)
- [Quick Start](#quick-start)
- [Kiến trúc và Nguyên lý](#kiến-trúc-và-nguyên-lý)
- [Cấu hình](#cấu-hình)
- [API Reference](#api-reference)
- [Ví dụ sử dụng](#ví-dụ-sử-dụng)
- [Best Practices](#best-practices)
- [Performance](#performance)

---

## Giới thiệu

**CFramework Event System** là một hệ thống sự kiện (event) dựa trên mô hình **Publish-Subscribe** (Pub/Sub), được thiết kế để giảm sự phụ thuộc (decoupling) giữa các module trong hệ thống nhúng thời gian thực.

### ✨ Tính năng chính

- ✅ **Publish-Subscribe Pattern**: Decoupling giữa publisher và subscriber
- ✅ **Sync/Async Mode**: Chọn chế độ đồng bộ hoặc bất đồng bộ
- ✅ **Thread-Safe**: An toàn đa luồng với mutex protection
- ✅ **Event Data**: Hỗ trợ truyền dữ liệu kèm event
- ✅ **Event Domains**: Tổ chức events theo nhóm chức năng (NEW!)
- ✅ **Type-Safe**: Typed event data structures với validation macros (NEW!)
- ✅ **Wildcard Subscription**: Subscribe tất cả events với `event_id = 0`
- ✅ **Zero-Copy (Sync)**: Sync mode không copy dữ liệu
- ✅ **ThreadPool Integration**: Async mode tích hợp với ThreadPool

---

## Event Domains

**NEW in v1.1.0:** Event Domains giúp tổ chức events theo nhóm chức năng (sensor, cellular, lora, ...).

### 🎯 Tại sao cần Domains?

**Vấn đề khi project lớn:**
```c
// ❌ Without domains - Hard to maintain
#define EVENT_RAIN_TIP       0x0001
#define EVENT_WATER_HIGH     0x0002
#define EVENT_SMS_RECEIVED   0x0003
#define EVENT_LORA_RX        0x0004
// ... 50+ events, easy to conflict!
```

**Giải pháp với Domains:**
```c
// ✅ With domains - Clear organization
#define CF_EVENT_DOMAIN_SENSOR    0x1000
#define CF_EVENT_DOMAIN_CELLULAR  0x2000
#define CF_EVENT_DOMAIN_LORA      0x3000

#define EVENT_SENSOR_RAIN_TIP     CF_EVENT_MAKE_ID(0x1000, 0x01)  // 0x10000001
#define EVENT_SENSOR_WATER_HIGH   CF_EVENT_MAKE_ID(0x1000, 0x02)  // 0x10000002
#define EVENT_CELL_SMS_RECEIVED   CF_EVENT_MAKE_ID(0x2000, 0x01)  // 0x20000001
#define EVENT_LORA_RX             CF_EVENT_MAKE_ID(0x3000, 0x01)  // 0x30000001
```

### 📦 Framework cung cấp:

| File | Mô tả |
|------|-------|
| `cf_event_types.h` | Common types, macros helpers (CF_EVENT_MAKE_ID, CF_EVENT_PUBLISH_TYPED, ...) |
| `cf_event_domain_template.h` | Template để copy và customize cho domain của bạn |
| `domains/cf_event_domain_sensor.h` | Example: Sensor domain (rain, water, temp, humidity) |
| `domains/cf_event_domain_cellular.h` | Example: Cellular domain (SMS, calls, network) |
| `domains/cf_event_domain_system.h` | Example: System domain (boot, error, power) |
| `DOMAIN_GUIDE.md` | **Step-by-step guide** chi tiết cách tạo domain |

### 🚀 Quick Start: Tạo Domain của bạn

**Bước 1:** Copy template
```bash
cp CFramework/cf_middleware/event/cf_event_domain_template.h \
   YourProject/Manager/MgrSensor/mgr_sensor_events.h
```

**Bước 2:** Define domain và events
```c
#include "event/cf_event.h"
#include "event/cf_event_types.h"

// Domain ID (chọn từ range 0x1000-0xFFFF cho managers)
#define CF_EVENT_DOMAIN_SENSOR  0x1000

// Define events
#define EVENT_SENSOR_RAIN_TIPPING   CF_EVENT_MAKE_ID(0x1000, 0x01)
#define EVENT_SENSOR_WATER_CHANGED  CF_EVENT_MAKE_ID(0x1000, 0x02)

// Define typed data structures
typedef struct {
    cf_event_header_t header;
    uint32_t tipping_count;
    float rainfall_mm;
} sensor_rain_event_t;
```

**Bước 3:** Publish từ Manager
```c
#include "mgr_sensor_events.h"

void mgr_sensor_on_rain_tip(void) {
    sensor_rain_event_t event = {
        .tipping_count = g_count++,
        .rainfall_mm = g_count * 0.2794f
    };

    CF_EVENT_PUBLISH_TYPED(EVENT_SENSOR_RAIN_TIPPING, &event, sensor_rain_event_t);
}
```

**Bước 4:** Subscribe từ App
```c
#include "mgr_sensor_events.h"

void rain_handler(cf_event_id_t id, const void* data, size_t size, void* user) {
    CF_EVENT_CAST_DATA(evt, data, size, sensor_rain_event_t);
    if (evt == NULL) return;

    printf("Rain: %lu tips, %.2f mm\n", evt->tipping_count, evt->rainfall_mm);
}

void app_init(void) {
    CF_EVENT_SUBSCRIBE_ASYNC(EVENT_SENSOR_RAIN_TIPPING, rain_handler, NULL);
}
```

📖 **Xem chi tiết:** [DOMAIN_GUIDE.md](DOMAIN_GUIDE.md)

---

## Quick Start

### Cài đặt cơ bản

```c
// 1. Initialize event system
cf_event_init();

// 2. Subscribe to events
CF_EVENT_SUBSCRIBE_ASYNC(EVENT_SENSOR_RAIN_TIPPING, my_handler, NULL);

// 3. Publish events
sensor_rain_event_t data = {.tipping_count = 10};
CF_EVENT_PUBLISH_TYPED(EVENT_SENSOR_RAIN_TIPPING, &data, sensor_rain_event_t);
```

### 🎯 Khi nào nên dùng?

✅ **Nên dùng khi:**
- Module A cần thông báo cho nhiều module khác mà không biết họ là ai
- Cần tách biệt business logic khỏi hardware layer
- Cần xử lý event bất đồng bộ để không block caller
- Muốn thêm/xóa listener động mà không ảnh hưởng code cũ

❌ **Không nên dùng khi:**
- Chỉ có 1 listener duy nhất → dùng callback trực tiếp
- Cần đảm bảo thứ tự thực thi nghiêm ngặt → dùng queue
- Performance critical path → overhead của event system có thể không chấp nhận được

---

## Kiến trúc và Nguyên lý

### 🏗️ Kiến trúc tổng quan

```
┌──────────────────────────────────────────────────────────────┐
│                     Event System Core                         │
│  ┌────────────────────────────────────────────────────────┐  │
│  │  Subscriber Table (CF_EVENT_MAX_SUBSCRIBERS = 32)     │  │
│  │  ┌──────────┬──────────┬──────────┬─────────────────┐ │  │
│  │  │ Slot 0   │ Slot 1   │ Slot 2   │  ...            │ │  │
│  │  │ event_id │ event_id │ event_id │                 │ │  │
│  │  │ callback │ callback │ callback │                 │ │  │
│  │  │ mode     │ mode     │ mode     │                 │ │  │
│  │  │ active   │ active   │ active   │                 │ │  │
│  │  └──────────┴──────────┴──────────┴─────────────────┘ │  │
│  │                                                        │  │
│  │  Protected by: cf_mutex_t                             │  │
│  └────────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────────┘
        ▲                                    │
        │ Subscribe                          │ Publish
        │                                    ▼
┌───────────────┐                  ┌──────────────────┐
│   Publisher   │                  │   Subscribers    │
│  (Any module) │                  │  (Event handlers)│
└───────────────┘                  └──────────────────┘
```

### 🔄 Flow hoạt động

#### **1. Initialization**
```c
cf_event_init()
  ├─ Allocate subscriber table
  ├─ Create mutex for thread-safety
  └─ Mark as initialized
```

#### **2. Subscription Flow**
```c
cf_event_subscribe(event_id, callback, user_data, mode, &handle)
  ├─ Lock mutex
  ├─ Find free subscriber slot (linear search)
  ├─ Register: event_id, callback, user_data, mode
  ├─ Increment subscriber_count
  ├─ Unlock mutex
  └─ Return handle (pointer to subscriber struct)
```

**Subscriber Structure:**
```c
typedef struct {
    bool active;                  // Is this slot active?
    cf_event_id_t event_id;      // Event to listen (0 = wildcard)
    cf_event_callback_t callback;// Function to call
    void* user_data;             // User context data
    cf_event_mode_t mode;        // CF_EVENT_SYNC or CF_EVENT_ASYNC
} cf_event_subscriber_s;
```

#### **3. Publish Flow**

##### **A. SYNC Mode (Đồng bộ)**
```
cf_event_publish(event_id)
  ├─ Lock mutex
  ├─ Loop through all subscribers:
  │   ├─ Match event_id OR wildcard (0)
  │   └─ If mode == CF_EVENT_SYNC:
  │       └─ callback(event_id, data, size, user_data)  ← Call trực tiếp
  ├─ Unlock mutex
  └─ Return
```

**Timeline:**
```
Publisher Thread:
  [cf_event_publish] → [Lock] → [Callback1] → [Callback2] → [Unlock] → [Return]
                                     ▲             ▲
                               Xử lý ngay    Xử lý ngay
```

**Đặc điểm:**
- ✅ Thực thi ngay lập tức trong context của publisher
- ✅ Zero-copy: truyền pointer trực tiếp
- ✅ Đảm bảo callback chạy xong trước khi publish return
- ⚠️ **Block caller**: nếu callback chậm sẽ block thread gọi publish
- ⚠️ **Stack risk**: callback chạy trên stack của publisher

##### **B. ASYNC Mode (Bất đồng bộ)**
```
cf_event_publish(event_id, data, size)
  ├─ Lock mutex
  ├─ Loop through all subscribers:
  │   ├─ Match event_id OR wildcard (0)
  │   └─ If mode == CF_EVENT_ASYNC:
  │       ├─ Allocate dispatch_context (pvPortMalloc)
  │       ├─ Copy event data (pvPortMalloc + memcpy)
  │       └─ cf_threadpool_submit(event_dispatch_task, context)
  ├─ Unlock mutex
  └─ Return immediately (không đợi callback)

ThreadPool Worker (later):
  event_dispatch_task(context)
    ├─ callback(event_id, data, size, user_data)
    ├─ vPortFree(data)
    └─ vPortFree(context)
```

**Timeline:**
```
Publisher Thread (Tmr Svc):
  [cf_event_publish] → [Alloc ctx] → [Copy data] → [Submit ThreadPool] → [Return]
                                                           │
                                                           ▼
ThreadPool Worker Thread:
                            [Queue pickup] → [Callback] → [Free memory]
                            (Sau một khoảng thời gian)
```

**Đặc điểm:**
- ✅ **Non-blocking**: publisher return ngay không đợi callback
- ✅ **Thread-safe**: callback chạy trên worker thread riêng
- ✅ **Parallel**: nhiều callback có thể chạy song song trên nhiều workers
- ⚠️ **Memory overhead**: cần allocate context + copy data
- ⚠️ **Latency**: có độ trễ từ lúc publish đến lúc callback thực thi
- ⚠️ **Order**: không đảm bảo thứ tự nếu có nhiều workers

---

## Cấu hình

### File: `cf_user_config.h`

```c
// Enable event system
#define CF_EVENT_ENABLED                1

// Maximum number of subscribers (global pool)
#define CF_EVENT_MAX_SUBSCRIBERS        32

// RTOS must be enabled for event system
#define CF_RTOS_ENABLED                 1

// ThreadPool required for async events
#define CF_THREADPOOL_ENABLED           1
```

### Dependency Graph
```
cf_event
  ├─ REQUIRES: cf_mutex (thread-safety)
  ├─ REQUIRES: cf_threadpool (async mode)
  └─ OPTIONAL: cf_log (debug logging)
```

---

## API Reference

### 📌 Initialization

#### `cf_event_init()`
```c
cf_status_t cf_event_init(void);
```
- **Mô tả**: Khởi tạo event system
- **Return**:
  - `CF_OK`: Thành công
  - `CF_ERROR_ALREADY_INITIALIZED`: Đã init rồi
  - `CF_ERROR_NO_MEMORY`: Lỗi tạo mutex
- **Lưu ý**: Phải gọi sau `cf_threadpool_init()` nếu dùng async

#### `cf_event_deinit()`
```c
void cf_event_deinit(void);
```
- **Mô tả**: Dọn dẹp event system
- **Side-effect**: Tất cả subscribers bị unsubscribe

---

### 📌 Subscribe / Unsubscribe

#### `cf_event_subscribe()`
```c
cf_status_t cf_event_subscribe(
    cf_event_id_t event_id,           // Event ID to listen (0 = all)
    cf_event_callback_t callback,     // Handler function
    void* user_data,                  // Context data
    cf_event_mode_t mode,             // CF_EVENT_SYNC or CF_EVENT_ASYNC
    cf_event_subscriber_t* handle     // [OUT] Handle (optional, can be NULL)
);
```

**Parameters:**
- `event_id`: Event cần subscribe
  - `1, 2, 3, ...`: Subscribe event cụ thể
  - `0`: **Wildcard** - nhận TẤT CẢ events
- `callback`: Function signature:
  ```c
  void my_callback(cf_event_id_t event_id,
                   const void* data,
                   size_t data_size,
                   void* user_data);
  ```
- `user_data`: Con trỏ tới context (VD: struct của module)
- `mode`:
  - `CF_EVENT_SYNC`: Callback chạy ngay trong context của publisher
  - `CF_EVENT_ASYNC`: Callback chạy trên ThreadPool worker
- `handle`: [Optional] Nhận handle để unsubscribe sau

**Return:**
- `CF_OK`: Thành công
- `CF_ERROR_NULL_POINTER`: callback NULL
- `CF_ERROR_NOT_INITIALIZED`: Chưa init
- `CF_ERROR_NO_MEMORY`: Đã đạt `CF_EVENT_MAX_SUBSCRIBERS`

**Macros tiện lợi:**
```c
// Sync mode, no handle
CF_EVENT_SUBSCRIBE(event_id, callback, user_data);

// Async mode, no handle
CF_EVENT_SUBSCRIBE_ASYNC(event_id, callback, user_data);
```

#### `cf_event_unsubscribe()`
```c
cf_status_t cf_event_unsubscribe(cf_event_subscriber_t handle);
```
- **Mô tả**: Hủy đăng ký một subscriber
- **Parameter**: Handle nhận được từ `cf_event_subscribe()`
- **Return**: `CF_OK` hoặc `CF_ERROR_NOT_FOUND`

#### `cf_event_unsubscribe_all()`
```c
uint32_t cf_event_unsubscribe_all(cf_event_id_t event_id);
```
- **Mô tả**: Hủy TẤT CẢ subscribers của một event
- **Return**: Số lượng subscribers đã hủy

---

### 📌 Publish Events

#### `cf_event_publish()`
```c
cf_status_t cf_event_publish(cf_event_id_t event_id);
```
- **Mô tả**: Publish event không có dữ liệu
- **Use case**: Event dạng "signal" (VD: button pressed, timeout)

**Ví dụ:**
```c
#define EVENT_BUTTON_PRESSED   0x0001
cf_event_publish(EVENT_BUTTON_PRESSED);
```

#### `cf_event_publish_data()`
```c
cf_status_t cf_event_publish_data(
    cf_event_id_t event_id,
    const void* data,
    size_t data_size
);
```
- **Mô tả**: Publish event kèm dữ liệu
- **Lưu ý**:
  - **SYNC mode**: data không được copy, truyền pointer trực tiếp (zero-copy)
  - **ASYNC mode**: data được copy vào heap, callback nhận pointer tới bản copy

**Ví dụ:**
```c
typedef struct {
    uint16_t temperature;
    uint8_t humidity;
} sensor_data_t;

sensor_data_t data = {25, 60};
cf_event_publish_data(EVENT_SENSOR_UPDATE, &data, sizeof(data));
```

**Macro tiện lợi:**
```c
// Tự động lấy sizeof()
#define EVENT_SENSOR_UPDATE 0x0100
sensor_data_t data = {25, 60};
CF_EVENT_PUBLISH_TYPE(EVENT_SENSOR_UPDATE, &data, sensor_data_t);
```

---

### 📌 Query Functions

#### `cf_event_get_subscriber_count()`
```c
uint32_t cf_event_get_subscriber_count(void);
```
- **Return**: Tổng số subscribers đang active

#### `cf_event_get_event_subscriber_count()`
```c
uint32_t cf_event_get_event_subscriber_count(cf_event_id_t event_id);
```
- **Return**: Số subscribers của event cụ thể (bao gồm cả wildcard)

---

## Ví dụ sử dụng

### 🎯 Ví dụ 1: Simple Button Event (Sync)

```c
#include "event/cf_event.h"

#define EVENT_BUTTON_1_PRESSED   0x0001
#define EVENT_BUTTON_1_RELEASED  0x0002

// Module LED - subscribe button events
void led_button_handler(cf_event_id_t event_id,
                        const void* data,
                        size_t data_size,
                        void* user_data)
{
    (void)data;
    (void)data_size;

    switch(event_id) {
        case EVENT_BUTTON_1_PRESSED:
            HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
            break;
        case EVENT_BUTTON_1_RELEASED:
            HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
            break;
    }
}

void app_init(void)
{
    cf_event_init();

    // Subscribe SYNC - callback chạy ngay trong GPIO interrupt
    CF_EVENT_SUBSCRIBE(EVENT_BUTTON_1_PRESSED, led_button_handler, NULL);
    CF_EVENT_SUBSCRIBE(EVENT_BUTTON_1_RELEASED, led_button_handler, NULL);
}

// Module Button - publish events from GPIO interrupt
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == BUTTON_1_Pin) {
        bool pressed = (HAL_GPIO_ReadPin(BUTTON_1_GPIO_Port, BUTTON_1_Pin) == GPIO_PIN_RESET);

        if (pressed) {
            cf_event_publish(EVENT_BUTTON_1_PRESSED);
        } else {
            cf_event_publish(EVENT_BUTTON_1_RELEASED);
        }
    }
}
```

---

### 🎯 Ví dụ 2: Sensor Data with Async Processing

```c
#define EVENT_TEMPERATURE_UPDATE  0x0100
#define EVENT_HUMIDITY_UPDATE     0x0101

typedef struct {
    float value;
    uint32_t timestamp;
} sensor_reading_t;

// Module Logger - subscribe và xử lý nặng trên worker thread
void logger_sensor_handler(cf_event_id_t event_id,
                           const void* data,
                           size_t data_size,
                           void* user_data)
{
    const sensor_reading_t* reading = (const sensor_reading_t*)data;

    // Safe to do heavy processing here - running on worker thread
    char filename[32];
    snprintf(filename, sizeof(filename), "sensor_%lu.csv", reading->timestamp);

    FILE* f = fopen(filename, "a");
    fprintf(f, "%lu,%.2f\n", reading->timestamp, reading->value);
    fclose(f);

    CF_LOG_I("Logged sensor reading: %.2f", reading->value);
}

void app_init(void)
{
    cf_threadpool_init();  // Required for async
    cf_event_init();

    // Subscribe ASYNC - callback chạy trên worker thread
    CF_EVENT_SUBSCRIBE_ASYNC(EVENT_TEMPERATURE_UPDATE, logger_sensor_handler, NULL);
    CF_EVENT_SUBSCRIBE_ASYNC(EVENT_HUMIDITY_UPDATE, logger_sensor_handler, NULL);
}

// ADC complete callback (ISR context)
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    sensor_reading_t reading = {
        .value = read_temperature(),
        .timestamp = HAL_GetTick()
    };

    // Publish from ISR - OK vì async mode không block
    cf_event_publish_data(EVENT_TEMPERATURE_UPDATE, &reading, sizeof(reading));
}
```

---

### 🎯 Ví dụ 3: Wildcard Subscriber (Debug Monitor)

```c
// Module Monitor - listen ALL events for debugging
void monitor_all_events(cf_event_id_t event_id,
                        const void* data,
                        size_t data_size,
                        void* user_data)
{
    CF_LOG_D("[MONITOR] Event 0x%04X published, data_size=%u", event_id, data_size);
}

void app_init(void)
{
    cf_event_init();

    // event_id = 0 → Subscribe ALL events
    CF_EVENT_SUBSCRIBE_ASYNC(0, monitor_all_events, NULL);
}
```

---

### 🎯 Ví dụ 4: Module Context with User Data

```c
typedef struct {
    uint32_t led_toggle_count;
    bool enabled;
} led_module_ctx_t;

void led_event_handler(cf_event_id_t event_id,
                       const void* data,
                       size_t data_size,
                       void* user_data)
{
    led_module_ctx_t* ctx = (led_module_ctx_t*)user_data;

    if (!ctx->enabled) {
        return;  // Module disabled
    }

    switch(event_id) {
        case EVENT_LED_TOGGLE:
            HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
            ctx->led_toggle_count++;
            CF_LOG_D("LED toggled %lu times", ctx->led_toggle_count);
            break;
    }
}

void app_init(void)
{
    static led_module_ctx_t led_ctx = { .led_toggle_count = 0, .enabled = true };

    cf_event_init();
    CF_EVENT_SUBSCRIBE_ASYNC(EVENT_LED_TOGGLE, led_event_handler, &led_ctx);
}
```

---

### 🎯 Ví dụ 5: Unsubscribe với Handle

```c
void app_example(void)
{
    cf_event_subscriber_t handle;

    // Subscribe và lưu handle
    cf_event_subscribe(EVENT_TEMP_HIGH,
                       temp_alarm_handler,
                       NULL,
                       CF_EVENT_ASYNC,
                       &handle);

    // ... sau một thời gian ...

    // Unsubscribe
    cf_event_unsubscribe(handle);
}
```

---

## Best Practices

### ✅ DO (Nên làm)

#### 1. **Dùng SYNC cho critical path, ASYNC cho heavy processing**
```c
// ✅ GOOD - Critical action, cần chạy ngay
CF_EVENT_SUBSCRIBE(EVENT_EMERGENCY_STOP, emergency_handler, NULL);

// ✅ GOOD - Heavy processing, không cần chạy ngay
CF_EVENT_SUBSCRIBE_ASYNC(EVENT_LOG_DATA, logging_handler, NULL);
```

#### 2. **Dùng named constants cho event IDs**
```c
// ✅ GOOD
#define EVENT_MOTOR_START    0x1001
#define EVENT_MOTOR_STOP     0x1002

// ❌ BAD
cf_event_publish(123);  // Magic number!
```

#### 3. **Validate data trong callback**
```c
void my_handler(cf_event_id_t event_id, const void* data,
                size_t data_size, void* user_data)
{
    // ✅ GOOD - Check trước khi dùng
    if (data == NULL || data_size != sizeof(my_data_t)) {
        CF_LOG_E("Invalid event data");
        return;
    }

    const my_data_t* d = (const my_data_t*)data;
    // Safe to use d
}
```

#### 4. **Cleanup khi shutdown**
```c
void app_cleanup(void)
{
    cf_event_deinit();  // Auto unsubscribe all
    cf_threadpool_deinit();
}
```

#### 5. **Dùng user_data cho module context**
```c
// ✅ GOOD - Stateful module
typedef struct {
    uint32_t counter;
    bool active;
} module_state_t;

static module_state_t g_state;

CF_EVENT_SUBSCRIBE(EVENT_ID, handler, &g_state);
```

---

### ❌ DON'T (Không nên làm)

#### 1. **KHÔNG block quá lâu trong SYNC callback**
```c
// ❌ BAD - Block mọi subscriber khác
void slow_sync_handler(cf_event_id_t event_id, ...) {
    osDelay(1000);  // DON'T DO THIS IN SYNC!
    // Dùng ASYNC nếu cần delay
}
```

#### 2. **KHÔNG dereference con trỏ data trong ASYNC sau khi callback return**
```c
// ❌ BAD
void async_handler(cf_event_id_t event_id, const void* data, ...) {
    // Data đã được copy sẵn, nhưng chỉ tồn tại trong scope của callback này
    // KHÔNG lưu pointer này để dùng sau!
    static const my_data_t* saved_data = (const my_data_t*)data;  // DANGER!
}
```

#### 3. **KHÔNG publish từ callback cùng event (recursive)**
```c
// ❌ BAD - Vòng lặp vô hạn
void bad_handler(cf_event_id_t event_id, ...) {
    if (event_id == EVENT_A) {
        cf_event_publish(EVENT_A);  // Recursive! Mutex deadlock nếu SYNC!
    }
}
```

#### 4. **KHÔNG quên kiểm tra return value**
```c
// ❌ BAD
cf_event_subscribe(...);  // Có thể fail nếu hết slot!

// ✅ GOOD
if (cf_event_subscribe(...) != CF_OK) {
    CF_LOG_E("Failed to subscribe");
    // Handle error
}
```

#### 5. **KHÔNG dùng event cho inter-task communication cần response**
```c
// ❌ BAD - Event không có response mechanism
cf_event_publish(EVENT_GET_TEMPERATURE);  // Làm sao lấy kết quả?

// ✅ GOOD - Dùng message queue hoặc function call trực tiếp
float temp = get_temperature();
```

---

## Performance

### 📊 Memory Usage

#### **Static Memory**
```c
sizeof(cf_event_system_t) =
    4 (bool initialized)
  + 4 (mutex handle)
  + sizeof(cf_event_subscriber_s) * CF_EVENT_MAX_SUBSCRIBERS
  + 4 (subscriber_count)
  + 4 (total_published)

sizeof(cf_event_subscriber_s) =
    4 (bool active)
  + 4 (event_id)
  + 4 (callback pointer)
  + 4 (user_data pointer)
  + 4 (mode)
  = 20 bytes

Total static = 16 + (20 * 32) = 656 bytes (with 32 subscribers)
```

#### **Dynamic Memory (per ASYNC event)**
```c
sizeof(cf_event_dispatch_ctx_t) =
    4 (event_id)
  + 4 (callback)
  + 4 (user_data)
  + 4 (data pointer)
  + 4 (data_size)
  = 20 bytes

Total per async event = 20 + data_size
```

**Ví dụ:**
- Event không có data: 20 bytes
- Event với data 16 bytes: 20 + 16 = 36 bytes

### ⏱️ Timing Analysis

#### **Sync Mode**
```c
Overhead =
    Lock mutex        : ~10-50 CPU cycles
  + Loop subscribers  : ~10 * subscriber_count cycles
  + Unlock mutex      : ~10-50 CPU cycles
  + Callback execution: <depends on callback>

Total: ~100 cycles + callback_time (at 80MHz: ~1.25µs + callback)
```

#### **Async Mode**
```c
Overhead =
    Lock mutex        : ~10-50 cycles
  + Loop subscribers  : ~10 * subscriber_count cycles
  + pvPortMalloc(ctx) : ~500-2000 cycles
  + memcpy data       : ~1 cycle/byte
  + ThreadPool submit : ~200-500 cycles
  + Unlock mutex      : ~10-50 cycles

Total: ~1500-3000 cycles (at 80MHz: ~20-40µs)
Plus latency cho worker pickup: 0-10ms depending on load
```

### 🔥 Worst-Case Scenarios

#### **Max Subscribers**
- 32 subscribers SYNC: ~32 * callback_time
- Nếu mỗi callback 100µs → 3.2ms total
- **Mitigation**: Dùng ASYNC cho heavy callbacks

#### **Memory Exhaustion**
- Async events cần heap allocation
- Nếu publish nhanh hơn worker xử lý → heap fragmentation
- **Mitigation**: Monitor heap, giới hạn publish rate

#### **Mutex Contention**
- Nhiều thread publish cùng lúc → serialize bởi mutex
- **Mitigation**: Publish từ ít thread hơn, hoặc dùng lock-free queue

---

## Thread-Safety Analysis

### 🔒 Protected Sections

**Mutex bảo vệ:**
- ✅ Subscriber table read/write
- ✅ Subscriber count
- ✅ Event publish iteration

**Không cần mutex:**
- ✅ Async callback (chạy trên worker thread riêng)
- ✅ Sync callback (caller đã hold mutex)

### ⚠️ Deadlock Prevention

**Tình huống nguy hiểm:**
```c
// Thread A:
cf_mutex_lock(my_mutex);
cf_event_publish(EVENT_X);  // Chờ event mutex
cf_mutex_unlock(my_mutex);

// Thread B (SYNC callback):
void handler(...) {
    cf_mutex_lock(my_mutex);  // Chờ my_mutex → DEADLOCK!
}
```

**Giải pháp:**
1. Dùng ASYNC mode để tránh nested locking
2. Định nghĩa lock hierarchy rõ ràng
3. Timeout cho mutex lock

---

## FAQs

**Q: Event ID = 0 có ý nghĩa gì?**
A: Wildcard - subscriber nhận TẤT CẢ events. Dùng cho logging/monitoring.

**Q: Có giới hạn số subscribers cho 1 event?**
A: Không, chỉ giới hạn tổng số subscribers toàn hệ thống (CF_EVENT_MAX_SUBSCRIBERS = 32).

**Q: Publish từ ISR có an toàn không?**
A: Có, nhưng chỉ nên dùng với ASYNC mode. SYNC mode sẽ block ISR.

**Q: Data trong callback có tồn tại sau khi callback return?**
A: KHÔNG. Chỉ valid trong callback. Cần copy nếu muốn lưu.

**Q: Thứ tự callback có đảm bảo không?**
A: SYNC: đảm bảo thứ tự subscribe. ASYNC: không đảm bảo (do ThreadPool parallel).

**Q: Có thể unsubscribe trong callback không?**
A: Có, nhưng nên dùng handle. Không được unsubscribe chính callback đang chạy (undefined behavior).

**Q: Project lớn với hơn 32 subscribers phải làm sao?**
A: Xem phần [Scaling Event System](#scaling-event-system-for-large-projects) bên dưới.

---

## Scaling Event System for Large Projects

### ⚠️ Design Limitation

**CFramework Event System là SINGLETON:**
- Chỉ có **1 global instance** (`g_event_system`)
- Cố định **32 subscriber slots** (CF_EVENT_MAX_SUBSCRIBERS)
- **1 mutex toàn cục** → contention cao khi nhiều module publish đồng thời

**Vấn đề khi project phức tạp:**
```
Project lớn có:
- 10 modules hardware (GPIO, UART, SPI, I2C, ADC, ...)  : ~15 subscribers
- 5 modules protocol (Modbus, LoRa, BLE, MQTT, ...)     : ~10 subscribers
- 3 modules application (UI, Logger, State Machine, ...) : ~8 subscribers
────────────────────────────────────────────────────────────────
Total: 33+ subscribers → KHÔNG ĐỦ 32 SLOTS!
```

### 🔧 Workarounds

#### **Option 1: Tăng CF_EVENT_MAX_SUBSCRIBERS (Quick Fix)**

```c
// In cf_user_config.h
#define CF_EVENT_MAX_SUBSCRIBERS    64   // hoặc 128
```

**Pros:**
- ✅ Đơn giản, chỉ sửa config
- ✅ Không cần thay đổi code

**Cons:**
- ❌ Tăng RAM usage: `20 bytes * 64 = 1.28KB`
- ❌ Mutex contention vẫn cao
- ❌ Linear search chậm hơn khi có nhiều subscribers

**Phù hợp:** Project vừa (50-100 subscribers)

---

#### **Option 2: Event Namespace / Domain Separation**

Chia events theo domain, mỗi module chỉ subscribe events liên quan:

```c
// Define event domains
#define EVENT_DOMAIN_HARDWARE    0x1000   // 0x1000 - 0x1FFF
#define EVENT_DOMAIN_PROTOCOL    0x2000   // 0x2000 - 0x2FFF
#define EVENT_DOMAIN_APPLICATION 0x3000   // 0x3000 - 0x3FFF

// Hardware events
#define EVENT_GPIO_CHANGE        (EVENT_DOMAIN_HARDWARE | 0x01)
#define EVENT_UART_RX            (EVENT_DOMAIN_HARDWARE | 0x02)

// Protocol events
#define EVENT_LORA_RX            (EVENT_DOMAIN_PROTOCOL | 0x01)
#define EVENT_MODBUS_REQUEST     (EVENT_DOMAIN_PROTOCOL | 0x02)

// Application events
#define EVENT_UI_BUTTON          (EVENT_DOMAIN_APPLICATION | 0x01)
#define EVENT_STATE_CHANGE       (EVENT_DOMAIN_APPLICATION | 0x02)
```

**Architecture:**
```
┌───────────────────────────────────────────────────────────┐
│         Single Event System (32 slots)                    │
├───────────────────────────────────────────────────────────┤
│  Domain HARDWARE  │  Domain PROTOCOL  │  Domain APP      │
│  (10 events)      │  (8 events)       │  (6 events)      │
├───────────────────┴───────────────────┴──────────────────┤
│  Unused: 8 slots                                          │
└───────────────────────────────────────────────────────────┘
```

**Benefits:**
- ✅ Rõ ràng, dễ quản lý
- ✅ Tránh conflict event IDs
- ✅ Giảm subscribers mỗi module (không subscribe wildcard)

**Cons:**
- ❌ Vẫn bị giới hạn 32 subscribers toàn cục

**Phù hợp:** Project có sự phân tách domain rõ ràng

---

#### **Option 3: Event Aggregator Pattern**

Thay vì mỗi sub-module subscribe trực tiếp, dùng 1 module trung gian:

```c
// ❌ BAD: Mỗi sensor subscribe riêng (10 subscribers)
CF_EVENT_SUBSCRIBE(EVENT_ADC_COMPLETE, sensor1_handler, &sensor1);
CF_EVENT_SUBSCRIBE(EVENT_ADC_COMPLETE, sensor2_handler, &sensor2);
// ... 8 sensors nữa ...

// ✅ GOOD: 1 sensor manager subscribe (1 subscriber)
typedef struct {
    sensor_t sensors[10];
} sensor_manager_t;

void sensor_manager_handler(cf_event_id_t event_id, ...) {
    // Dispatch internally to all sensors
    for (int i = 0; i < 10; i++) {
        if (sensors[i].enabled) {
            sensor_process(&sensors[i], event_id, data);
        }
    }
}

CF_EVENT_SUBSCRIBE_ASYNC(EVENT_ADC_COMPLETE, sensor_manager_handler, &manager);
```

**Architecture:**
```
Event System (32 slots)
    ↓
Aggregator (1 subscriber)
    ├→ Sub-module 1
    ├→ Sub-module 2
    ├→ Sub-module 3
    └→ ... (không chiếm slot event system)
```

**Benefits:**
- ✅ Tiết kiệm subscribers drastically
- ✅ Module manager có thể implement custom logic (priority, filter, ...)
- ✅ Dễ debug (chỉ 1 điểm vào)

**Cons:**
- ❌ Thêm 1 lớp indirection
- ❌ Aggregator phải dispatch thủ công

**Phù hợp:** Nhiều sub-modules giống nhau (sensors, actuators, channels, ...)

---

#### **Option 4: Custom Multi-Instance Event System (Advanced)**

Nếu thực sự cần nhiều event systems độc lập, có thể fork CFramework và sửa:

**Changes needed:**
```c
// cf_event.h
typedef struct cf_event_system_s* cf_event_system_t;

cf_status_t cf_event_system_create(cf_event_system_t* system, uint32_t max_subscribers);
cf_status_t cf_event_system_destroy(cf_event_system_t system);

cf_status_t cf_event_subscribe_ex(cf_event_system_t system,
                                   cf_event_id_t event_id,
                                   cf_event_callback_t callback,
                                   ...);

cf_status_t cf_event_publish_ex(cf_event_system_t system,
                                 cf_event_id_t event_id);
```

**Usage:**
```c
// Create separate event systems
cf_event_system_t hw_events;
cf_event_system_t protocol_events;
cf_event_system_t app_events;

cf_event_system_create(&hw_events, 16);      // 16 slots for hardware
cf_event_system_create(&protocol_events, 12); // 12 slots for protocols
cf_event_system_create(&app_events, 8);      // 8 slots for app

// Subscribe to specific system
cf_event_subscribe_ex(hw_events, EVENT_GPIO, handler, ...);
cf_event_subscribe_ex(protocol_events, EVENT_LORA_RX, handler, ...);

// Publish to specific system
cf_event_publish_ex(hw_events, EVENT_GPIO);
```

**Benefits:**
- ✅ Isolation hoàn toàn giữa các domains
- ✅ Giảm mutex contention (mỗi system có mutex riêng)
- ✅ Dynamic sizing
- ✅ Scalable không giới hạn

**Cons:**
- ❌ Phải fork và maintain CFramework
- ❌ API phức tạp hơn (thêm tham số system)
- ❌ Memory overhead (mỗi system có mutex riêng)

**Phù hợp:** Project rất lớn (industrial, automotive) cần isolation cao

---

### 📊 So sánh các giải pháp

| Giải pháp | Độ khó | RAM | Performance | Scalability | Khi nào dùng |
|-----------|--------|-----|-------------|-------------|--------------|
| **Option 1: Tăng MAX_SUBSCRIBERS** | ⭐ Rất dễ | ⚠️ Tăng linear | ⚠️ Giảm khi >100 | ⭐⭐ Vừa | Project 50-100 subscribers |
| **Option 2: Event Domains** | ⭐⭐ Dễ | ✅ Không đổi | ✅ Tốt | ⭐⭐ Vừa | Architecture rõ ràng |
| **Option 3: Aggregator Pattern** | ⭐⭐⭐ Vừa | ✅ Ít nhất | ✅ Tốt | ⭐⭐⭐ Cao | Nhiều sub-modules giống nhau |
| **Option 4: Multi-Instance** | ⭐⭐⭐⭐⭐ Khó | ⚠️ Nhiều | ✅ Tốt nhất | ⭐⭐⭐⭐⭐ Rất cao | Project enterprise-grade |

---

### 💡 Khuyến nghị

**Cho project của bạn (RTOS Framework):**

```c
// 1. Tăng MAX_SUBSCRIBERS (immediate fix)
#define CF_EVENT_MAX_SUBSCRIBERS    64

// 2. Implement Event Domains
#define EVENT_DOMAIN_LORA           0x1000
#define EVENT_DOMAIN_GSM            0x2000
#define EVENT_DOMAIN_SENSOR         0x3000
#define EVENT_DOMAIN_CONTROL        0x4000

// 3. Dùng Aggregator cho sensors/actuators
typedef struct {
    sensor_t* sensors[MAX_SENSORS];
    uint8_t count;
} sensor_aggregator_t;

void sensor_aggregator_handler(cf_event_id_t event_id,
                                const void* data,
                                size_t data_size,
                                void* user_data) {
    sensor_aggregator_t* agg = (sensor_aggregator_t*)user_data;
    for (uint8_t i = 0; i < agg->count; i++) {
        sensor_process(agg->sensors[i], event_id, data, data_size);
    }
}

// Only 1 subscriber slot used!
CF_EVENT_SUBSCRIBE_ASYNC(EVENT_DOMAIN_SENSOR | 0,
                         sensor_aggregator_handler,
                         &g_sensor_agg);
```

**Kết quả:**
- ✅ 64 slots thay vì 32 → đủ cho project vừa/lớn
- ✅ Event domains rõ ràng → dễ maintain
- ✅ Aggregator cho sensors → tiết kiệm slots
- ✅ Không phải fork CFramework → dễ upgrade

---

## 📚 Related Documentation

- [ThreadPool README](../threadpool/README.md) - Async event backend
- [Mutex API](../../cf_core/os/README.md) - Thread-safety mechanism
- [Logger System](../../cf_core/utils/README.md) - Debugging events

---

## 🐛 Debugging Tips

### Enable Debug Logging
```c
// In cf_user_config.h
#define CF_LOG_ENABLED  1
#define CF_LOG_LEVEL    CF_LOG_DEBUG
```

### Check Subscriber Count
```c
uint32_t count = cf_event_get_subscriber_count();
CF_LOG_D("Active subscribers: %lu", count);
```

### Monitor Events
```c
// Subscribe wildcard để track all events
void debug_monitor(cf_event_id_t id, const void* data, size_t size, void* user) {
    CF_LOG_D("[EVENT] 0x%04X published, size=%u", id, size);
}
CF_EVENT_SUBSCRIBE_ASYNC(0, debug_monitor, NULL);
```

### Check Heap Usage (ASYNC events)
```c
size_t free_heap = xPortGetFreeHeapSize();
CF_LOG_D("Free heap: %u bytes", free_heap);
```

---

**Version:** 1.0.0
**Last Updated:** 2025-11-01
**License:** MIT
