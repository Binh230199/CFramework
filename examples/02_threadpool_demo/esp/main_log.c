/*
 * ESP32 CF ThreadPool + Dual Logging Demo
 * 
 * CHUC NANG CUA VI DU:
 * - Su dung CF ThreadPool de chay cac task dong thoi
 * - Log ra ca UART va USB CDC cung luc (dual logging)
 * - Demo cac loai log khac nhau: INFO, WARNING, ERROR
 * - Theo doi trang thai ThreadPool (active, pending tasks)
 * - Test hieu suat va dong bo cua logging system
 * 
 * CAC TASK DEMO:
 * 1. hello_task: Task don gian in thong bao va delay
 * 2. counter_task: Task dem so voi tham so dau vao
 * 
 * LOGGING SYSTEM:
 * - UART Sink: Log ra cong serial (115200 baud)
 * - USB Sink: Log ra USB CDC (Virtual COM Port)
 * - Tat ca log se xuat hien tren ca 2 kenh dong thoi
 * 
 * LUONG XU LY:
 * Main Loop -> ThreadPool Submit -> Task Execute -> Log Output
 */

#include "cf.h"
#include "log_uart_sink.h"
#include "log_usb_sink.h"
#include "platform_uart.h"
#include "platform_usb.h"

//==============================================================================
// BIEN TOAN CUC
//==============================================================================

// Global sinks cho dual logging
static cf_uart_sink_t uart_sink;
static cf_usb_sink_t usb_sink;

//==============================================================================
// KHAI BAO HAM
//==============================================================================

// Task functions
void hello_task(void *arg);        // Task demo don gian
void counter_task(void *arg);      // Task dem so voi tham so

// Init functions  
cf_status_t init_framework(void);  // Khoi tao CFramework va dual logging

//==============================================================================
// MAIN APPLICATION
//==============================================================================

void app_main(void)
{
    printf("ESP32 ThreadPool + Dual Logging Test\n");
    
    // Initialize framework
    if (init_framework() != CF_OK) {
        printf("Init failed, stopping\n");
        return;
    }
    
    CF_LOG_I("Starting test loop with dual logging...");
    CF_LOG_I("Messages will appear on both UART and USB CDC");
    
    int count = 1;
    while (true) {
        // Submit hello task
        cf_threadpool_submit(hello_task, NULL,
                            CF_THREADPOOL_PRIORITY_NORMAL, CF_WAIT_FOREVER);
        
        // Submit counter task voi tham so
        cf_threadpool_submit(counter_task, (void*)count,
                            CF_THREADPOOL_PRIORITY_NORMAL, CF_WAIT_FOREVER);
        
        // Theo doi trang thai ThreadPool
        CF_LOG_I("Loop #%d - Active: %lu, Pending: %lu",
                 count,
                 cf_threadpool_get_active_count(),
                 cf_threadpool_get_pending_count());
        
        // Log cac loai message khac nhau
        if (count % 3 == 0) {
            CF_LOG_W("This is a warning message (loop %d)", count);
        }
        
        if (count % 5 == 0) {
            CF_LOG_E("This is an error message (loop %d)", count);
        }
        
        count++;
        vTaskDelay(pdMS_TO_TICKS(1000));  // Delay giua cac loop
    }
}

//==============================================================================
// CAC TASK FUNCTION
//==============================================================================

// Task demo don gian
void hello_task(void *arg)
{
    CF_LOG_I("Hello from ThreadPool!");
    vTaskDelay(pdMS_TO_TICKS(500));  // Gia lap xu ly
    CF_LOG_I("Hello task completed");
}

// Task dem so voi tham so
void counter_task(void *arg)
{
    int num = (int)(uintptr_t)arg;
    CF_LOG_I("Counter task #%d started", num);
    vTaskDelay(pdMS_TO_TICKS(300));  // Gia lap xu ly
    CF_LOG_I("Counter task #%d finished", num);
}

//==============================================================================
// INIT FUNCTIONS
//==============================================================================

cf_status_t init_framework(void)
{
    // 1. Init CFramework logger
    if (cf_log_init() != CF_OK) {
        printf("Log init failed\n");
        return CF_ERROR;
    }
    
    // 2. Initialize ESP32 UART platform
    if (platform_uart_init(UART_NUM_0, 115200) != CF_OK) {
        printf("Platform UART init failed\n");
        return CF_ERROR;
    }
    
    // 3. Initialize ESP32 USB platform  
    if (platform_usb_init() != CF_OK) {
        printf("Platform USB init failed\n");
        return CF_ERROR;
    }
    
    // 4. Create UART sink
    cf_uart_sink_config_t uart_config = {
        .uart = UART_NUM_0,
        .timeout_ms = 1000
    };
    
    if (cf_uart_sink_create(&uart_sink, &uart_config, CF_LOG_INFO) != CF_OK) {
        printf("UART sink create failed\n");
        return CF_ERROR;
    }
    
    // 5. Create USB sink
    cf_usb_sink_config_t usb_config = {
        .timeout_ms = 1000
    };
    
    if (cf_usb_sink_create(&usb_sink, &usb_config, CF_LOG_INFO) != CF_OK) {
        printf("USB sink create failed\n");
        return CF_ERROR;
    }
    
    // 6. Add both sinks to logger (dual output)
    if (cf_log_add_sink(&uart_sink.base) != CF_OK) {
        printf("Add UART sink failed\n");
        return CF_ERROR;
    }
    
    if (cf_log_add_sink(&usb_sink.base) != CF_OK) {
        printf("Add USB sink failed\n");
        return CF_ERROR;
    }
    
    // 7. Init ThreadPool
    if (cf_threadpool_init() != CF_OK) {
        CF_LOG_E("ThreadPool init failed");
        return CF_ERROR;
    }
    
    CF_LOG_I("Framework initialized with UART + USB logging");
    return CF_OK;
}