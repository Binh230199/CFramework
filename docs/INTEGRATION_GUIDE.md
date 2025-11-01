# CFramework Integration Guide for STM32CubeIDE

## Prerequisites

- STM32CubeIDE installed
- STM32 project created with CubeMX (với FreeRTOS enabled)
- Target MCU: STM32L4, STM32L1, STM32F4, hoặc STM32F1

---

## Step 1: Add CFramework to Project

### Option A: Copy Framework Files

1. Copy thư mục `CFramework` vào workspace của bạn:
   ```
   YourWorkspace/
   ├── YourProject/
   └── CFramework/          ← Copy toàn bộ framework vào đây
   ```

2. Hoặc dùng Git Submodule (recommended):
   ```bash
   cd YourWorkspace/YourProject
   git submodule add <framework-repo-url> lib/CFramework
   ```

---

## Step 2: Configure STM32CubeIDE Project

### 2.1 Add Include Paths

Right click Project → **Properties** → **C/C++ Build** → **Settings** → **Tool Settings** → **MCU GCC Compiler** → **Include paths**

Add các path sau:

```
../CFramework/cf_core/include
../CFramework/cf_middleware
```

Hoặc nếu dùng thư mục `lib`:
```
../lib/CFramework/cf_core/include
../lib/CFramework/cf_middleware
```

### 2.2 Add Source Files

**Method 1: Link to Folder (Recommended)**

Right click Project → **New** → **Folder** → **Advanced** → Check "Link to alternate location"

Add 2 folders:
- `CFramework_Core` → Link to `CFramework/cf_core/src`
- `CFramework_Middleware` → Link to `CFramework/cf_middleware`

**Method 2: Add to Source Locations**

Properties → **C/C++ General** → **Paths and Symbols** → **Source Location** → **Add Folder**

Add:
- `CFramework/cf_core/src`
- `CFramework/cf_middleware`

### 2.3 Exclude Platform Files

CFramework có nhiều platform ports. Bạn chỉ cần build port cho MCU của mình.

Right click các thư mục platform KHÔNG dùng → **Resource Configurations** → **Exclude from Build** → Check cả Debug và Release

Ví dụ nếu dùng **STM32L4**:
- ✅ Keep: `cf_core/port/stm32l4/`
- ❌ Exclude: `cf_core/port/stm32l1/`
- ❌ Exclude: `cf_core/port/stm32f1/`
- ❌ Exclude: `cf_core/port/stm32f4/`
- ❌ Exclude: `cf_core/port/esp32/`

---

## Step 3: Create User Configuration

Tạo file `cf_user_config.h` trong thư mục `Core/Inc/` của project:

```c
/**
 * @file cf_user_config.h
 * @brief User configuration for CFramework
 */

#ifndef CF_USER_CONFIG_H
#define CF_USER_CONFIG_H

//==============================================================================
// PLATFORM SELECTION (REQUIRED)
//==============================================================================

// Uncomment platform của bạn:
#define CF_PLATFORM_STM32L4
// #define CF_PLATFORM_STM32L1
// #define CF_PLATFORM_STM32F4
// #define CF_PLATFORM_STM32F1

//==============================================================================
// RTOS CONFIGURATION
//==============================================================================

#define CF_RTOS_ENABLED              1
#define CF_RTOS_FREERTOS             1

//==============================================================================
// THREADPOOL CONFIGURATION (Optional overrides)
//==============================================================================

#define CF_THREADPOOL_ENABLED        1
#define CF_THREADPOOL_THREAD_COUNT   4     // Số worker threads
#define CF_THREADPOOL_QUEUE_SIZE     20    // Kích thước queue
#define CF_THREADPOOL_STACK_SIZE     2048  // Stack size mỗi worker

//==============================================================================
// EVENT SYSTEM CONFIGURATION (Optional overrides)
//==============================================================================

#define CF_EVENT_ENABLED             1
#define CF_EVENT_MAX_SUBSCRIBERS     32    // Số subscriber tối đa

//==============================================================================
// LOGGER CONFIGURATION (Optional overrides)
//==============================================================================

#define CF_LOG_ENABLED               1
#define CF_LOG_MAX_SINKS             4
#define CF_LOG_BUFFER_SIZE           512

//==============================================================================
// DEBUG CONFIGURATION
//==============================================================================

#ifdef DEBUG
    #define CF_DEBUG                 1
#else
    #define CF_DEBUG                 0
#endif

#define CF_ASSERT_ENABLED            CF_DEBUG

//==============================================================================
// HAL CONFIGURATION (Optional overrides)
//==============================================================================

#define CF_HAL_GPIO_MAX_HANDLES      16
#define CF_HAL_UART_MAX_HANDLES      4

#endif /* CF_USER_CONFIG_H */
```

---

## Step 4: Add Compiler Define

Properties → **C/C++ Build** → **Settings** → **MCU GCC Compiler** → **Preprocessor**

Add define:
```
CF_USER_CONFIG
```

---

## Step 5: Configure FreeRTOS in CubeMX

### 5.1 Enable FreeRTOS

CubeMX → **Middleware** → **FREERTOS** → Enable

### 5.2 Configure FreeRTOS Settings

**FREERTOS → Config Parameters:**

```
USE_PREEMPTION              = Enabled
CPU_CLOCK_HZ               = (auto from clock config)
TICK_RATE_HZ               = 1000
MAX_PRIORITIES             = 7
MINIMAL_STACK_SIZE         = 128
TOTAL_HEAP_SIZE            = 15360 (hoặc lớn hơn tùy RAM)
```

**FREERTOS → Advanced settings:**

Enable các feature sau:
- ✅ `USE_TIMERS` = Enabled (cho cf_timer)
- ✅ `USE_MUTEXES` = Enabled
- ✅ `USE_COUNTING_SEMAPHORES` = Enabled
- ✅ `TIMER_TASK_STACK_DEPTH` = 256
- ✅ `TIMER_QUEUE_LENGTH` = 10

### 5.3 Generate Code

Click **Generate Code** trong CubeMX

---

## Step 6: Update main.c

### 6.1 Include CFramework

Trong `Core/Src/main.c`, thêm:

```c
/* USER CODE BEGIN Includes */
#include "cf.h"  // CFramework master include
/* USER CODE END Includes */
```

### 6.2 Initialize Framework

```c
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/
  HAL_Init();
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();  // Nếu dùng UART cho logging
  /* USER CODE BEGIN 2 */

  // Initialize CFramework Logger
  cf_log_init();
  
  // Create UART sink for logging
  cf_log_uart_sink_t uart_sink;
  cf_log_uart_sink_init(&uart_sink, &huart2, CF_LOG_DEBUG);
  cf_log_add_sink(&uart_sink.base);
  
  CF_LOG_I("=== System Starting ===");
  CF_LOG_I("Framework Version: %s", CF_VERSION_STRING);
  
  // Initialize ThreadPool
  cf_status_t status = cf_threadpool_init();
  if (status != CF_OK) {
      CF_LOG_E("ThreadPool init failed: %d", status);
      Error_Handler();
  }
  
  // Initialize Event System
  status = cf_event_init();
  if (status != CF_OK) {
      CF_LOG_E("Event system init failed: %d", status);
      Error_Handler();
  }
  
  CF_LOG_I("CFramework initialized successfully");

  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();  /* Call init function for freertos objects (in freertos.c) */
  MX_FREERTOS_Init();
  
  /* USER CODE BEGIN RTOS_THREADS */
  // Tạo các application tasks của bạn ở đây
  /* USER CODE END RTOS_THREADS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */
  while (1)
  {
  }
}
```

### 6.3 Create Application Task

Option 1: Dùng CubeMX tạo task, rồi thêm code vào `app_freertos.c`

Option 2: Tạo task thủ công trong `USER CODE BEGIN RTOS_THREADS`:

```c
/* USER CODE BEGIN RTOS_THREADS */

// Application task function
void app_task(void* arg)
{
    CF_LOG_I("Application task started");
    
    // Your application code
    while (1) {
        // Do something
        cf_task_delay(1000);
    }
}

// Create task
cf_task_config_t task_cfg;
cf_task_config_default(&task_cfg);
task_cfg.name = "AppTask";
task_cfg.function = app_task;
task_cfg.stack_size = 2048;
task_cfg.priority = CF_TASK_PRIORITY_NORMAL;

cf_task_t app_task_handle;
cf_task_create(&app_task_handle, &task_cfg);

/* USER CODE END RTOS_THREADS */
```

---

## Step 7: Build & Flash

1. **Build Project**: Project → Build All (Ctrl+B)

2. **Fix any errors**:
   - Missing includes → Check include paths
   - Undefined reference → Check source files added
   - Platform mismatch → Check `CF_PLATFORM_xxx` define

3. **Flash**: Run → Debug (F11) hoặc Run (Ctrl+F11)

4. **View Logs**: 
   - Mở Serial Terminal (115200 baud, 8N1)
   - Connect đến UART2 (hoặc UART bạn config)

---

## Example: Simple LED Blink with Events

Tạo file `app_main.c` trong `Core/Src/`:

```c
#include "cf.h"
#include "main.h"  // For GPIO defines from CubeMX

#define EVENT_LED_TOGGLE    0x00001000

// LED toggle task
void led_task(void* arg)
{
    CF_LOG_I("LED task started");
    
    // Subscribe to LED toggle event
    cf_event_subscribe(EVENT_LED_TOGGLE, led_event_handler, NULL, 
                      CF_EVENT_SYNC, NULL);
    
    while (1) {
        // Publish toggle event every second
        cf_event_publish(EVENT_LED_TOGGLE);
        cf_task_delay(1000);
    }
}

// LED event handler
void led_event_handler(cf_event_id_t event_id, const void* data, 
                       size_t data_size, void* user_data)
{
    HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);  // Toggle LED
    CF_LOG_D("LED toggled");
}

// Initialize application
void app_init(void)
{
    CF_LOG_I("Initializing application...");
    
    // Create LED task
    cf_task_config_t task_cfg;
    cf_task_config_default(&task_cfg);
    task_cfg.name = "LEDTask";
    task_cfg.function = led_task;
    task_cfg.stack_size = 1024;
    task_cfg.priority = CF_TASK_PRIORITY_NORMAL;
    
    cf_task_t led_task_handle;
    cf_status_t status = cf_task_create(&led_task_handle, &task_cfg);
    if (status != CF_OK) {
        CF_LOG_E("Failed to create LED task");
    }
}
```

Gọi `app_init()` từ `main.c` trong section `USER CODE BEGIN RTOS_THREADS`.

---

## Troubleshooting

### Build Errors

**Error: `cf_config.h: No such file`**
- Fix: Check include paths đã add đúng chưa

**Error: `CF_PLATFORM_xxx not defined`**
- Fix: Tạo `cf_user_config.h` và define platform

**Error: `undefined reference to cf_xxx`**
- Fix: Check source files đã add vào build chưa

**Error: `multiple definition of cf_xxx`**
- Fix: Có thể .c file bị add 2 lần, check source locations

### Runtime Errors

**HardFault khi start scheduler**
- Fix: Tăng `TOTAL_HEAP_SIZE` trong FreeRTOS config

**No log output**
- Fix: Check UART đã init chưa, check baud rate, check TX pin

**Tasks không chạy**
- Fix: Check stack size đủ lớn chưa, check priority conflicts

---

## Memory Requirements

### Minimum RAM Requirements:

- **STM32L4** (64KB RAM): OK ✅
- **STM32L1** (32KB RAM): OK (cần optimize) ⚠️
- **STM32F4** (128KB+ RAM): Perfect ✅

### Typical Memory Usage:

```
ThreadPool (4 workers):
  - Workers: 4 × 2KB stack = 8KB
  - Queues: 4 × 20 items × 12 bytes ≈ 1KB
  
Event System:
  - Subscribers: 32 × 32 bytes = 1KB
  
Logger:
  - Buffer: 512 bytes
  
Total Framework: ~10-12KB RAM
```

### Optimization Tips:

Nếu RAM ít, giảm config trong `cf_user_config.h`:

```c
#define CF_THREADPOOL_THREAD_COUNT   2     // 4 → 2 workers
#define CF_THREADPOOL_QUEUE_SIZE     10    // 20 → 10
#define CF_THREADPOOL_STACK_SIZE     1536  // 2048 → 1536
#define CF_EVENT_MAX_SUBSCRIBERS     16    // 32 → 16
#define CF_LOG_BUFFER_SIZE           256   // 512 → 256
```

---

## Next Steps

1. ✅ Framework integrated
2. ✅ Basic example running
3. → Build your IoT application
4. → Add sensors (I2C/SPI)
5. → Add network (WiFi/LoRa)
6. → Implement business logic

---

## Support

- Check `copilot-instructions.md` for architecture details
- See `examples/` for code patterns
- Framework follows strict naming: `cf_*` prefix

Good luck! 🚀
