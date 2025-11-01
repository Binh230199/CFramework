# Quick Start Guide

## 1. Copy Framework vào Project

```
YourSTM32Project/
├── Core/
│   ├── Inc/
│   │   └── cf_user_config.h    ← Tạo file này
│   └── Src/
└── Lib/
    └── CFramework/              ← Copy toàn bộ framework vào đây
```

## 2. STM32CubeIDE Configuration

### Add Include Paths
**Properties → C/C++ Build → Settings → MCU GCC Compiler → Include paths**

Add:
```
../Lib/CFramework/cf_core/include
../Lib/CFramework/cf_middleware
```

### Add Preprocessor Define
**MCU GCC Compiler → Preprocessor**

Add:
```
CF_USER_CONFIG
```

### Link Source Folders
**Right click project → New → Folder → Advanced → Link to alternate location**

Link:
- `../Lib/CFramework/cf_core/src` → `CFramework_Core`
- `../Lib/CFramework/cf_middleware` → `CFramework_Middleware`

### Exclude Other Platforms
**Right click unused platform folders → Exclude from Build**

Ví dụ nếu dùng STM32L4:
- Keep: `cf_core/port/stm32l4/`
- Exclude others

## 3. Create cf_user_config.h

File: `Core/Inc/cf_user_config.h`

```c
#ifndef CF_USER_CONFIG_H
#define CF_USER_CONFIG_H

// REQUIRED: Select your platform
#define CF_PLATFORM_STM32L4

// Optional: Customize
#define CF_THREADPOOL_THREAD_COUNT   4
#define CF_EVENT_MAX_SUBSCRIBERS     32
#define CF_LOG_BUFFER_SIZE           512

#ifdef DEBUG
    #define CF_DEBUG  1
#else
    #define CF_DEBUG  0
#endif

#endif
```

## 4. CubeMX FreeRTOS Config

Enable trong CubeMX:
- ✅ FREERTOS enabled
- ✅ USE_TIMERS = Enabled
- ✅ USE_MUTEXES = Enabled
- ✅ TOTAL_HEAP_SIZE ≥ 15360

Generate code.

## 5. Update main.c

```c
/* USER CODE BEGIN Includes */
#include "cf.h"
/* USER CODE END Includes */

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_USART2_UART_Init();  // For logging
  
  /* USER CODE BEGIN 2 */
  
  // Init framework
  cf_log_init();
  
  // UART logging
  cf_log_uart_sink_t uart_sink;
  cf_log_uart_sink_init(&uart_sink, &huart2, CF_LOG_DEBUG);
  cf_log_add_sink(&uart_sink.base);
  
  CF_LOG_I("System starting...");
  
  // Init ThreadPool & Events
  cf_threadpool_init();
  cf_event_init();
  
  /* USER CODE END 2 */
  
  osKernelInitialize();
  MX_FREERTOS_Init();
  
  /* USER CODE BEGIN RTOS_THREADS */
  // Create your tasks here
  /* USER CODE END RTOS_THREADS */
  
  osKernelStart();
  
  while (1) {}
}
```

## 6. Build & Run

1. Build All (Ctrl+B)
2. Flash to board (F11)
3. Open Serial Terminal (115200 baud)
4. See logs! 🎉

## Example Task

```c
void my_app_task(void* arg)
{
    CF_LOG_I("App started!");
    
    while (1) {
        // Your code
        cf_task_delay(1000);
    }
}

// In USER CODE BEGIN RTOS_THREADS
cf_task_config_t cfg;
cf_task_config_default(&cfg);
cfg.name = "MyApp";
cfg.function = my_app_task;
cfg.stack_size = 2048;
cfg.priority = CF_TASK_PRIORITY_NORMAL;

cf_task_t task;
cf_task_create(&task, &cfg);
```

Done! Framework ready to use 🚀

See `INTEGRATION_GUIDE.md` for details.
