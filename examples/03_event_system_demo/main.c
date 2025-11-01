/**
 * @file main.c
 * @brief Event System + Timer demonstration
 *
 * This example shows a complete IoT sensor node architecture:
 * - Event-driven communication between modules
 * - Periodic sensor readings with timers
 * - Async event processing with ThreadPool
 * - Pub-sub pattern for loose coupling
 */

#include "cf.h"

//==============================================================================
// EVENT DEFINITIONS
//==============================================================================

#define EVENT_SENSOR_DATA_READY     0x00001000
#define EVENT_SENSOR_ERROR          0x00001001
#define EVENT_BATTERY_LOW           0x00002000
#define EVENT_NETWORK_CONNECTED     0x00003000
#define EVENT_NETWORK_DISCONNECTED  0x00003001
#define EVENT_DATA_UPLOADED         0x00004000

//==============================================================================
// DATA STRUCTURES
//==============================================================================

typedef struct {
    uint32_t sensor_id;
    float temperature;
    float humidity;
    uint32_t timestamp;
} sensor_data_t;

typedef struct {
    uint8_t level;
    uint16_t voltage_mv;
} battery_status_t;

//==============================================================================
// MODULE: SENSOR READER
//==============================================================================

static cf_timer_t g_sensor_timer;
static uint32_t g_sensor_reading_count = 0;

/**
 * @brief Simulate sensor reading
 */
static void sensor_read_callback(cf_timer_t timer, void* arg)
{
    (void)timer;
    (void)arg;

    g_sensor_reading_count++;

    // Simulate sensor reading
    sensor_data_t data = {
        .sensor_id = 1,
        .temperature = 20.0f + (float)(g_sensor_reading_count % 10),
        .humidity = 60.0f + (float)(g_sensor_reading_count % 20),
        .timestamp = xTaskGetTickCount()
    };

    CF_LOG_I("[Sensor] Reading #%lu: Temp=%.1f°C, Hum=%.1f%%",
             g_sensor_reading_count, data.temperature, data.humidity);

    // Publish event with data
    cf_status_t status = CF_EVENT_PUBLISH_TYPE(EVENT_SENSOR_DATA_READY, &data, sensor_data_t);
    if (status != CF_OK) {
        CF_LOG_E("[Sensor] Failed to publish event: %d", status);
        cf_event_publish(EVENT_SENSOR_ERROR);
    }
}

/**
 * @brief Initialize sensor module
 */
static void sensor_module_init(void)
{
    cf_timer_config_t timer_cfg;
    cf_timer_config_default(&timer_cfg);
    timer_cfg.name = "SensorTimer";
    timer_cfg.period_ms = 2000;  // Read every 2 seconds
    timer_cfg.type = CF_TIMER_PERIODIC;
    timer_cfg.callback = sensor_read_callback;
    timer_cfg.auto_start = true;

    cf_status_t status = cf_timer_create(&g_sensor_timer, &timer_cfg);
    if (status == CF_OK) {
        CF_LOG_I("[Sensor] Module initialized (2s period)");
    } else {
        CF_LOG_E("[Sensor] Failed to initialize: %d", status);
    }
}

//==============================================================================
// MODULE: DATA PROCESSOR
//==============================================================================

/**
 * @brief Process sensor data (async)
 */
static void process_sensor_data(cf_event_id_t event_id,
                                const void* data,
                                size_t data_size,
                                void* user_data)
{
    (void)event_id;
    (void)user_data;

    if (data == NULL || data_size != sizeof(sensor_data_t)) {
        CF_LOG_W("[Processor] Invalid sensor data");
        return;
    }

    const sensor_data_t* sensor_data = (const sensor_data_t*)data;

    CF_LOG_I("[Processor] Processing sensor %lu data...", sensor_data->sensor_id);

    // Simulate processing time
    cf_task_delay(100);

    // Check for anomalies
    if (sensor_data->temperature > 25.0f) {
        CF_LOG_W("[Processor] High temperature detected: %.1f°C", sensor_data->temperature);
    }

    if (sensor_data->humidity < 40.0f) {
        CF_LOG_W("[Processor] Low humidity detected: %.1f%%", sensor_data->humidity);
    }

    CF_LOG_I("[Processor] Data processed successfully");
}

/**
 * @brief Handle sensor errors (sync)
 */
static void handle_sensor_error(cf_event_id_t event_id,
                                const void* data,
                                size_t data_size,
                                void* user_data)
{
    (void)event_id;
    (void)data;
    (void)data_size;
    (void)user_data;

    CF_LOG_E("[Processor] Sensor error detected - initiating recovery");
}

/**
 * @brief Initialize data processor module
 */
static void processor_module_init(void)
{
    // Subscribe to sensor data (async processing)
    CF_EVENT_SUBSCRIBE_ASYNC(EVENT_SENSOR_DATA_READY, process_sensor_data, NULL);

    // Subscribe to sensor errors (sync handling)
    CF_EVENT_SUBSCRIBE(EVENT_SENSOR_ERROR, handle_sensor_error, NULL);

    CF_LOG_I("[Processor] Module initialized");
}

//==============================================================================
// MODULE: DATA UPLOADER
//==============================================================================

/**
 * @brief Upload sensor data to cloud (async)
 */
static void upload_sensor_data(cf_event_id_t event_id,
                               const void* data,
                               size_t data_size,
                               void* user_data)
{
    (void)event_id;
    (void)user_data;

    if (data == NULL || data_size != sizeof(sensor_data_t)) {
        return;
    }

    const sensor_data_t* sensor_data = (const sensor_data_t*)data;

    CF_LOG_I("[Uploader] Uploading data to cloud...");

    // Simulate network transmission
    cf_task_delay(150);

    CF_LOG_I("[Uploader] Data uploaded (Sensor %lu, Temp %.1f°C)",
             sensor_data->sensor_id, sensor_data->temperature);

    // Notify upload complete
    cf_event_publish(EVENT_DATA_UPLOADED);
}

/**
 * @brief Initialize uploader module
 */
static void uploader_module_init(void)
{
    // Subscribe to sensor data
    CF_EVENT_SUBSCRIBE_ASYNC(EVENT_SENSOR_DATA_READY, upload_sensor_data, NULL);

    CF_LOG_I("[Uploader] Module initialized");
}

//==============================================================================
// MODULE: BATTERY MONITOR
//==============================================================================

static cf_timer_t g_battery_timer;

/**
 * @brief Check battery level
 */
static void battery_check_callback(cf_timer_t timer, void* arg)
{
    (void)timer;
    (void)arg;

    // Simulate battery reading
    static uint8_t battery_level = 100;
    battery_level = (battery_level > 5) ? (battery_level - 5) : 100;

    CF_LOG_D("[Battery] Level: %u%%", battery_level);

    if (battery_level < 20) {
        battery_status_t status = {
            .level = battery_level,
            .voltage_mv = 3300 + (battery_level * 10)
        };

        CF_LOG_W("[Battery] Low battery warning!");
        CF_EVENT_PUBLISH_TYPE(EVENT_BATTERY_LOW, &status, battery_status_t);
    }
}

/**
 * @brief Handle battery low event
 */
static void handle_battery_low(cf_event_id_t event_id,
                               const void* data,
                               size_t data_size,
                               void* user_data)
{
    (void)event_id;
    (void)user_data;

    if (data == NULL || data_size != sizeof(battery_status_t)) {
        return;
    }

    const battery_status_t* status = (const battery_status_t*)data;

    CF_LOG_W("[System] Battery low action: Level=%u%%, Voltage=%umV",
             status->level, status->voltage_mv);

    // Could reduce sampling rate, disable features, etc.
}

/**
 * @brief Initialize battery monitor module
 */
static void battery_module_init(void)
{
    // Subscribe to battery events
    CF_EVENT_SUBSCRIBE(EVENT_BATTERY_LOW, handle_battery_low, NULL);

    // Create periodic battery check timer
    cf_timer_config_t timer_cfg;
    cf_timer_config_default(&timer_cfg);
    timer_cfg.name = "BatteryTimer";
    timer_cfg.period_ms = 5000;  // Check every 5 seconds
    timer_cfg.type = CF_TIMER_PERIODIC;
    timer_cfg.callback = battery_check_callback;
    timer_cfg.auto_start = true;

    cf_status_t status = cf_timer_create(&g_battery_timer, &timer_cfg);
    if (status == CF_OK) {
        CF_LOG_I("[Battery] Module initialized (5s period)");
    }
}

//==============================================================================
// MODULE: LOGGER (subscribes to ALL events)
//==============================================================================

/**
 * @brief Log all events
 */
static void log_all_events(cf_event_id_t event_id,
                           const void* data,
                           size_t data_size,
                           void* user_data)
{
    (void)data;
    (void)user_data;

    const char* event_name = "UNKNOWN";

    switch (event_id) {
        case EVENT_SENSOR_DATA_READY:     event_name = "SENSOR_DATA_READY"; break;
        case EVENT_SENSOR_ERROR:          event_name = "SENSOR_ERROR"; break;
        case EVENT_BATTERY_LOW:           event_name = "BATTERY_LOW"; break;
        case EVENT_NETWORK_CONNECTED:     event_name = "NETWORK_CONNECTED"; break;
        case EVENT_NETWORK_DISCONNECTED:  event_name = "NETWORK_DISCONNECTED"; break;
        case EVENT_DATA_UPLOADED:         event_name = "DATA_UPLOADED"; break;
    }

    CF_LOG_D("[EventLog] Event: %s (0x%08lX, %u bytes)",
             event_name, event_id, (unsigned)data_size);
}

/**
 * @brief Initialize event logger module
 */
static void event_logger_init(void)
{
    // Subscribe to ALL events (event_id = 0)
    CF_EVENT_SUBSCRIBE(0, log_all_events, NULL);

    CF_LOG_I("[EventLog] Module initialized (monitoring all events)");
}

//==============================================================================
// MAIN APPLICATION
//==============================================================================

/**
 * @brief Main application task
 */
static void app_main_task(void* arg)
{
    (void)arg;

    CF_LOG_I("=== CFramework Event System Demo ===");
    CF_LOG_I("Framework Version: %s", CF_VERSION_STRING);

    // Initialize ThreadPool (required for async events)
    cf_status_t status = cf_threadpool_init();
    if (status != CF_OK) {
        CF_LOG_E("ThreadPool init failed: %d", status);
        return;
    }
    CF_LOG_I("ThreadPool initialized");

    // Initialize Event System
    status = cf_event_init();
    if (status != CF_OK) {
        CF_LOG_E("Event system init failed: %d", status);
        return;
    }
    CF_LOG_I("Event system initialized");

    cf_task_delay(500);

    // Initialize all modules
    CF_LOG_I("--- Initializing Modules ---");
    event_logger_init();
    sensor_module_init();
    processor_module_init();
    uploader_module_init();
    battery_module_init();

    CF_LOG_I("--- System Running ---");
    CF_LOG_I("Subscribers: %lu", cf_event_get_subscriber_count());

    // Run for a while
    cf_task_delay(20000);

    // Statistics
    CF_LOG_I("--- System Statistics ---");
    CF_LOG_I("Total sensor readings: %lu", g_sensor_reading_count);
    CF_LOG_I("Active subscribers: %lu", cf_event_get_subscriber_count());
    CF_LOG_I("ThreadPool active tasks: %lu", cf_threadpool_get_active_count());
    CF_LOG_I("ThreadPool pending tasks: %lu", cf_threadpool_get_pending_count());

    // Cleanup
    CF_LOG_I("Shutting down...");
    cf_timer_delete(g_sensor_timer, 100);
    cf_timer_delete(g_battery_timer, 100);
    cf_event_deinit();
    cf_threadpool_deinit(true);

    CF_LOG_I("=== Demo completed ===");

    while (1) {
        cf_task_delay(1000);
    }
}

//==============================================================================
// UART CONFIGURATION
//==============================================================================

#if defined(CF_PLATFORM_STM32L4) || defined(CF_PLATFORM_STM32L1)
    extern UART_HandleTypeDef huart2;
    #define LOG_UART_HANDLE &huart2
#elif defined(CF_PLATFORM_ESP32)
    #define LOG_UART_PORT UART_NUM_0
#endif

//==============================================================================
// MAIN ENTRY POINT
//==============================================================================

int main(void)
{
    // Hardware initialization (platform-specific)
#if defined(CF_PLATFORM_STM32L4) || defined(CF_PLATFORM_STM32L1)
    HAL_Init();
    SystemClock_Config();  // Implement this
#endif

    // Initialize logger
    cf_log_init();

#if defined(CF_PLATFORM_STM32L4) || defined(CF_PLATFORM_STM32L1)
    cf_log_uart_sink_t uart_sink;
    cf_log_uart_sink_init(&uart_sink, LOG_UART_HANDLE, CF_LOG_DEBUG);
    cf_log_add_sink(&uart_sink.base);
#elif defined(CF_PLATFORM_ESP32)
    cf_log_uart_sink_t uart_sink;
    cf_log_uart_sink_init(&uart_sink, LOG_UART_PORT, CF_LOG_DEBUG);
    cf_log_add_sink(&uart_sink.base);
#endif

    CF_LOG_I("System starting...");

    // Create main application task
    cf_task_config_t task_config;
    cf_task_config_default(&task_config);
    task_config.name = "AppMain";
    task_config.function = app_main_task;
    task_config.stack_size = 4096;
    task_config.priority = CF_TASK_PRIORITY_NORMAL;

    cf_task_t app_task;
    cf_status_t status = cf_task_create(&app_task, &task_config);
    if (status != CF_OK) {
        CF_LOG_E("Failed to create app task: %d", status);
        while (1);
    }

    // Start FreeRTOS scheduler
    CF_LOG_I("Starting RTOS scheduler...");
    cf_task_start_scheduler();

    return 0;
}
