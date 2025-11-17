/*
* ESP32 LED Blink Demo
* 
* CHUC NANG CUA VI DU:
* - LED nhap nhay voi toc do co the thay doi
* - Bam nut BOOT de thay doi toc do LED (1000ms -> 500ms -> 200ms -> 100ms -> 50ms)
* - Su dung ThreadPool de xu ly cac task

* CAC TASK CHINH:
* 1. led_toggle_task: Bat/tat LED va delay theo toc do hien tai
* 2. speed_change_task: Thay doi toc do nhap nhay LED
* 3. button_debounce_task: Xu ly debounce nut bam va xac nhan bam nut
* 
* LUONG XU LY:
* ISR -> Queue -> Main Loop -> ThreadPool -> Task -> Queue
*/

#include "cf.h"
#include "log_usb_sink.h"
#include "platform_usb.h"
#include "driver/gpio.h"

//==============================================================================
// DEFINES VA CONSTANTS
//==============================================================================

// GPIO pins
#define BUTTON_GPIO     0    // GPIO cho nut BOOT
#define LED_GPIO        35   // GPIO cho LED

// Debounce va queue settings
#define DEBOUNCE_TIME_MS    50   // Thoi gian debounce nut bam
#define DEBOUNCE_STABLE_MS  20   // Thoi gian on dinh sau debounce
#define QUEUE_SIZE 10            // Kich thuoc queue

// Cac loai message
typedef enum {
    MSG_LED_TOGGLE,      // Message bat/tat LED
    MSG_SPEED_CHANGE,    // Message thay doi toc do
    MSG_BUTTON_PRESSED,  // Message nut duoc bam
    MSG_LED_BLINK_TIMER  // Message timer cho LED nhap nhay
} message_type_t;

typedef struct {
    message_type_t type;
    uint32_t timestamp;
} queue_message_t;

//==============================================================================
// BIEN TOAN CUC
//==============================================================================

// CF Framework objects
static cf_usb_sink_t usb_sink;
static cf_mutex_t speed_mutex = NULL;
static cf_queue_t main_queue = NULL;

// LED control variables
static bool led_state = false;
static uint32_t blink_speed_ms = 1000;
static const uint32_t blink_speeds[] = {1000, 500, 200, 100, 50};
static uint8_t speed_index = 0;

// Button debounce variables
static volatile uint32_t last_button_time = 0;
static volatile bool button_pressed = false;

//==============================================================================
// KHAI BAO HAM
//==============================================================================

// Task functions
void led_toggle_task(void *arg);        // Task bat/tat LED va gui message tiep theo
void speed_change_task(void *arg);      // Task thay doi toc do LED
void button_debounce_task(void *arg);   // Task xu ly debounce nut bam

// Init functions
cf_status_t init_gpio(void);           // Khoi tao GPIO cho LED va nut bam
cf_status_t init_framework(void);      // Khoi tao CFramework va cac thanh phan

//==============================================================================
// GPIO INTERRUPT HANDLER
//==============================================================================

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t current_time = xTaskGetTickCountFromISR();
    
    if (!button_pressed && (current_time - last_button_time) > pdMS_TO_TICKS(DEBOUNCE_TIME_MS)) {
        last_button_time = current_time;
        button_pressed = true;
        
        queue_message_t msg = {
            .type = MSG_BUTTON_PRESSED,
            .timestamp = current_time
        };
        cf_queue_send(main_queue, &msg, CF_NO_WAIT);
    }
}

//==============================================================================
// MAIN APPLICATION
//==============================================================================

void app_main(void)
{
    printf("ESP32 CF Queue-Based LED Blink Demo\n");
    
    if (init_framework() != CF_OK) {
        printf("Init failed, stopping\n");
        return;
    }
    
    CF_LOG_I("Starting CF queue LED demo...");
    CF_LOG_I("Press BOOT button (GPIO %d) to change LED speed", BUTTON_GPIO);
    CF_LOG_I("Available speeds: 1000ms, 500ms, 200ms, 100ms, 50ms");
    
    // Gui message dau tien
    queue_message_t initial_msg = {
        .type = MSG_LED_BLINK_TIMER,
        .timestamp = xTaskGetTickCount()
    };
    cf_queue_send(main_queue, &initial_msg, CF_NO_WAIT);
    
    CF_LOG_I("CF queue system started");
    
    // Vong lap xu ly queue
    while (true) {
        queue_message_t received_msg;
        
        if (cf_queue_receive(main_queue, &received_msg, CF_WAIT_FOREVER) == CF_OK) {
            
            switch (received_msg.type) {
                case MSG_LED_BLINK_TIMER:
                    CF_LOG_D("Processing LED blink timer from CF queue");
                    cf_threadpool_submit(led_toggle_task, NULL,
                                        CF_THREADPOOL_PRIORITY_NORMAL, CF_NO_WAIT);
                    break;
                    
                case MSG_SPEED_CHANGE:
                    CF_LOG_D("Processing speed change from CF queue");
                    cf_threadpool_submit(speed_change_task, NULL,
                                        CF_THREADPOOL_PRIORITY_HIGH, CF_NO_WAIT);
                    break;
                    
                case MSG_BUTTON_PRESSED:
                    CF_LOG_D("Processing button press from CF queue");
                    cf_threadpool_submit(button_debounce_task, NULL,
                                        CF_THREADPOOL_PRIORITY_HIGH, CF_NO_WAIT);
                    break;
                    
                default:
                    CF_LOG_W("Unknown message type: %d", received_msg.type);
                    break;
            }
        } else {
            CF_LOG_E("Failed to receive from CF queue");
        }
    }
}

//==============================================================================
// INIT FUNCTIONS
//==============================================================================

cf_status_t init_gpio(void)
{
    // Config LED GPIO
    gpio_config_t led_config = {
        .pin_bit_mask = (1ULL << LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    
    if (gpio_config(&led_config) != ESP_OK) {
        printf("LED GPIO config failed\n");
        return CF_ERROR;
    }
    
    gpio_set_level(LED_GPIO, 0);
    led_state = false;
    
    // Config Button GPIO
    gpio_config_t button_config = {
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE
    };
    
    if (gpio_config(&button_config) != ESP_OK) {
        printf("Button GPIO config failed\n");
        return CF_ERROR;
    }
    
    if (gpio_install_isr_service(0) != ESP_OK) {
        printf("GPIO ISR service install failed\n");
        return CF_ERROR;
    }
    
    if (gpio_isr_handler_add(BUTTON_GPIO, gpio_isr_handler, NULL) != ESP_OK) {
        printf("GPIO ISR handler add failed\n");
        return CF_ERROR;
    }
    
    last_button_time = 0;
    button_pressed = false;
    
    CF_LOG_I("GPIO initialized - Button: %d, LED: %d", BUTTON_GPIO, LED_GPIO);
    return CF_OK;
}

cf_status_t init_framework(void)
{
    // Tao CF queue
    if (cf_queue_create(&main_queue, QUEUE_SIZE, sizeof(queue_message_t)) != CF_OK) {
        printf("Failed to create CF queue\n");
        return CF_ERROR;
    }
    
    // Tao mutex
    if (cf_mutex_create(&speed_mutex) != CF_OK) {
        printf("Failed to create speed mutex\n");
        return CF_ERROR;
    }
    
    // Init logger
    if (cf_log_init() != CF_OK) {
        printf("Log init failed\n");
        return CF_ERROR;
    }
    
    // Init USB platform
    if (platform_usb_init() != CF_OK) {
        printf("Platform USB init failed\n");
        return CF_ERROR;
    }
    
    // Tao USB sink
    cf_usb_sink_config_t usb_config = {.timeout_ms = 1000};
    if (cf_usb_sink_create(&usb_sink, &usb_config, CF_LOG_INFO) != CF_OK) {
        printf("USB sink create failed\n");
        return CF_ERROR;
    }
    
    // Add USB sink vao logger
    if (cf_log_add_sink(&usb_sink.base) != CF_OK) {
        printf("Add USB sink failed\n");
        return CF_ERROR;
    }
    
    // Init ThreadPool
    if (cf_threadpool_init() != CF_OK) {
        CF_LOG_E("ThreadPool init failed");
        return CF_ERROR;
    }
    
    // Init GPIO
    if (init_gpio() != CF_OK) {
        CF_LOG_E("GPIO init failed");
        return CF_ERROR;
    }
    
    CF_LOG_I("Framework initialized with CF queue-based messaging");
    return CF_OK;
}

//==============================================================================
// CAC TASK FUNCTION
//==============================================================================

// Task bat tat LED
void led_toggle_task(void *arg)
{
    led_state = !led_state;
    gpio_set_level(LED_GPIO, led_state);
    
    uint32_t current_speed;
    if (cf_mutex_lock(speed_mutex, 10) == CF_OK) {
        current_speed = blink_speed_ms;
        cf_mutex_unlock(speed_mutex);
    } else {
        current_speed = 1000;
    }
    
    CF_LOG_I("LED %s (Speed: %lu ms)", led_state ? "ON" : "OFF", current_speed);
    
    // Delay trong task
    vTaskDelay(pdMS_TO_TICKS(current_speed));
    
    // Gui message tiep theo
    queue_message_t msg = {
        .type = MSG_LED_BLINK_TIMER,
        .timestamp = xTaskGetTickCount()
    };
    cf_queue_send(main_queue, &msg, CF_NO_WAIT);
}

// Task thay doi toc do
void speed_change_task(void *arg)
{
    if (cf_mutex_lock(speed_mutex, 100) == CF_OK) {
        speed_index = (speed_index + 1) % (sizeof(blink_speeds) / sizeof(blink_speeds[0]));
        blink_speed_ms = blink_speeds[speed_index];
        
        CF_LOG_I("Speed changed to %lu ms (%s)", 
                blink_speed_ms,
                blink_speed_ms >= 1000 ? "SLOW" :
                blink_speed_ms >= 200 ? "MEDIUM" :
                blink_speed_ms >= 100 ? "FAST" : "VERY FAST");
        
        cf_mutex_unlock(speed_mutex);
    } else {
        CF_LOG_W("Failed to acquire speed mutex");
    }
}

// Task debounce nut bam
void button_debounce_task(void *arg)
{
    vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_TIME_MS));
    
    int button_level = gpio_get_level(BUTTON_GPIO);
    
    if (button_level == 0) {
        vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_STABLE_MS));
        button_level = gpio_get_level(BUTTON_GPIO);
        
        if (button_level == 0) {
            CF_LOG_D("Button debounce confirmed");
            
            queue_message_t msg = {
                .type = MSG_SPEED_CHANGE,
                .timestamp = xTaskGetTickCount()
            };
            cf_queue_send(main_queue, &msg, CF_NO_WAIT);
        } else {
            CF_LOG_D("Button debounce failed - button released too early");
        }
    } else {
        CF_LOG_D("Button debounce failed - button not pressed");
    }
    
    button_pressed = false;
}

