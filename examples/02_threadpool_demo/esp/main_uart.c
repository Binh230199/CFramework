/*
* ESP32 CF Queue-Based UART LED Control Demo
* 
* CHUC NANG CUA VI DU:
* - Nhan lenh qua UART0 de dieu khien LED
* - Gui phan hoi qua UART0 ve trang thai LED
* - Su dung CF queue de giao tiep giua cac thanh phan
* - Su dung ThreadPool de xu ly cac task
* 
* CAC TASK CHINH:
* 1. uart_receive_task: Nhan du lieu tu UART0 va xu ly lenh
* 2. uart_send_task: Gui phan hoi qua UART0
* 3. led_control_task: Bat/tat LED theo lenh nhan duoc
* 4. uart_event_task: Xu ly UART events
* 
* UART COMMANDS:
* - "ON" hoac "on": Bat LED
* - "OFF" hoac "off": Tat LED
* - Phan hoi: "LED ON" hoac "LED OFF"
* 
* LUONG XU LY:
* UART Event -> Queue -> Main Loop -> ThreadPool -> Task -> Queue
*/

#include "cf.h"
#include "log_usb_sink.h"
#include "platform_usb.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "string.h"

//==============================================================================
// KHAI BAO HAM
//==============================================================================

// Task functions
void uart_receive_task(void *arg);     // Task nhan du lieu UART0
void uart_send_task(void *arg);        // Task gui du lieu UART0
void led_control_task(void *arg);      // Task dieu khien LED
void uart_event_task(void *arg);       // Task xu ly UART events

// Init functions
cf_status_t init_gpio(void);           // Khoi tao GPIO cho LED
cf_status_t init_uart(void);           // Khoi tao UART0
cf_status_t init_framework(void);      // Khoi tao CFramework va cac thanh phan

//==============================================================================
// DEFINES VA CONSTANTS
//==============================================================================

// GPIO pins
#define LED_GPIO        35   // GPIO cho LED

// UART settings
#define UART_PORT       UART_NUM_0
#define UART_TXD        43   // GPIO43 - UART0 TX (custom pins)
#define UART_RXD        44   // GPIO44 - UART0 RX (custom pins)  
#define UART_BAUD_RATE  115200
#define UART_BUF_SIZE   1024
#define UART_QUEUE_SIZE 20

// Queue settings
#define QUEUE_SIZE 10

// Cac loai message
typedef enum {
    MSG_UART_DATA_RECEIVED,  // Message du lieu UART nhan duoc
    MSG_UART_SEND_RESPONSE,  // Message gui phan hoi UART
    MSG_LED_ON,              // Message bat LED
    MSG_LED_OFF              // Message tat LED
} message_type_t;

typedef struct {
    message_type_t type;
    char data[64];           // Du lieu UART hoac phan hoi
    uint32_t timestamp;
} queue_message_t;

//==============================================================================
// BIEN TOAN CUC
//==============================================================================

// CF Framework objects
static cf_usb_sink_t usb_sink;
static cf_queue_t main_queue = NULL;

// LED control variables
static bool led_state = false;

// UART event queue
static QueueHandle_t uart_queue;

//==============================================================================
// CAC TASK FUNCTION
//==============================================================================

// Task nhan du lieu UART0
void uart_receive_task(void *arg)
{
    char* received_data = (char*)arg;
    CF_LOG_I("UART received: %s", received_data);
    
    // Xu ly lenh
    if (strcasecmp(received_data, "ON") == 0) {
        // Gui message bat LED
        queue_message_t msg = {
            .type = MSG_LED_ON,
            .timestamp = xTaskGetTickCount()
        };
        strcpy(msg.data, "ON");
        cf_queue_send(main_queue, &msg, CF_NO_WAIT);
        
    } else if (strcasecmp(received_data, "OFF") == 0) {
        // Gui message tat LED
        queue_message_t msg = {
            .type = MSG_LED_OFF,
            .timestamp = xTaskGetTickCount()
        };
        strcpy(msg.data, "OFF");
        cf_queue_send(main_queue, &msg, CF_NO_WAIT);
        
    } else {
        // Lenh khong hop le
        CF_LOG_W("Invalid UART command: %s", received_data);
        queue_message_t msg = {
            .type = MSG_UART_SEND_RESPONSE,
            .timestamp = xTaskGetTickCount()
        };
        strcpy(msg.data, "ERROR: Invalid command. Use ON or OFF\r\n");
        cf_queue_send(main_queue, &msg, CF_NO_WAIT);
    }
}

// Task gui du lieu UART0
void uart_send_task(void *arg)
{
    char* response_data = (char*)arg;
    
    // Gui du lieu qua UART0
    uart_write_bytes(UART_PORT, response_data, strlen(response_data));
    CF_LOG_I("UART sent: %s", response_data);
}

// Task dieu khien LED
void led_control_task(void *arg)
{
    char* command = (char*)arg;
    
    if (strcmp(command, "ON") == 0) {
        led_state = true;
        gpio_set_level(LED_GPIO, 1);
        CF_LOG_I("LED turned ON");
        
        // Gui phan hoi
        queue_message_t msg = {
            .type = MSG_UART_SEND_RESPONSE,
            .timestamp = xTaskGetTickCount()
        };
        strcpy(msg.data, "LED ON\r\n");
        cf_queue_send(main_queue, &msg, CF_NO_WAIT);
        
    } else if (strcmp(command, "OFF") == 0) {
        led_state = false;
        gpio_set_level(LED_GPIO, 0);
        CF_LOG_I("LED turned OFF");
        
        // Gui phan hoi
        queue_message_t msg = {
            .type = MSG_UART_SEND_RESPONSE,
            .timestamp = xTaskGetTickCount()
        };
        strcpy(msg.data, "LED OFF\r\n");
        cf_queue_send(main_queue, &msg, CF_NO_WAIT);
    }
}

// Task xu ly UART events (thay the cho ISR handler)
void uart_event_task(void *arg)
{
    uart_event_t event;
    char* dtmp = (char*) malloc(UART_BUF_SIZE);
    
    while (true) {
        // Waiting for UART event
        if (xQueueReceive(uart_queue, (void *)&event, portMAX_DELAY)) {
            switch (event.type) {
                case UART_DATA:
                    CF_LOG_D("UART_DATA event, size: %d", event.size);
                    // Doc du lieu tu UART buffer
                    int len = uart_read_bytes(UART_PORT, dtmp, event.size, portMAX_DELAY);
                    if (len > 0) {
                        dtmp[len] = '\0';
                        
                        // Loai bo \r\n
                        for (int i = 0; i < len; i++) {
                            if (dtmp[i] == '\r' || dtmp[i] == '\n') {
                                dtmp[i] = '\0';
                                break;
                            }
                        }
                        
                        if (strlen(dtmp) > 0) {
                            // Gui message vao CF queue
                            queue_message_t msg = {
                                .type = MSG_UART_DATA_RECEIVED,
                                .timestamp = xTaskGetTickCount()
                            };
                            strcpy(msg.data, dtmp);
                            cf_queue_send(main_queue, &msg, CF_NO_WAIT);
                        }
                    }
                    break;
                    
                case UART_FIFO_OVF:
                    CF_LOG_W("UART FIFO overflow");
                    uart_flush_input(UART_PORT);
                    xQueueReset(uart_queue);
                    break;
                    
                case UART_BUFFER_FULL:
                    CF_LOG_W("UART ring buffer full");
                    uart_flush_input(UART_PORT);
                    xQueueReset(uart_queue);
                    break;
                    
                case UART_BREAK:
                    CF_LOG_W("UART RX break");
                    break;
                    
                case UART_PARITY_ERR:
                    CF_LOG_E("UART parity error");
                    break;
                    
                case UART_FRAME_ERR:
                    CF_LOG_E("UART frame error");
                    break;
                    
                case UART_PATTERN_DET:
                    CF_LOG_D("UART pattern detected");
                    break;
                    
                default:
                    CF_LOG_W("Unknown UART event type: %d", event.type);
                    break;
            }
        }
    }
    free(dtmp);
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
    
    CF_LOG_I("GPIO initialized - LED: %d", LED_GPIO);
    return CF_OK;
}

cf_status_t init_uart(void)
{
    // UART config
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
        .flags = {
            .backup_before_sleep = 0
        }
    };
    
    // Install UART driver with event queue
    if (uart_driver_install(UART_PORT, UART_BUF_SIZE, UART_BUF_SIZE, UART_QUEUE_SIZE, &uart_queue, 0) != ESP_OK) {
        printf("UART driver install failed\n");
        return CF_ERROR;
    }
    
    // Configure UART parameters
    if (uart_param_config(UART_PORT, &uart_config) != ESP_OK) {
        printf("UART param config failed\n");
        return CF_ERROR;
    }
    
    // Set UART pins (su dung custom pins)
    if (uart_set_pin(UART_PORT, UART_TXD, UART_RXD, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE) != ESP_OK) {
        printf("UART set pin failed\n");
        return CF_ERROR;
    }
    
    CF_LOG_I("UART initialized - Port: %d, Baud: %d, TX: %d, RX: %d", 
            UART_PORT, UART_BAUD_RATE, UART_TXD, UART_RXD);
    return CF_OK;
}

cf_status_t init_framework(void)
{
    // Tao CF queue
    if (cf_queue_create(&main_queue, QUEUE_SIZE, sizeof(queue_message_t)) != CF_OK) {
        printf("Failed to create CF queue\n");
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
    
    // Init UART
    if (init_uart() != CF_OK) {
        CF_LOG_E("UART init failed");
        return CF_ERROR;
    }
    
    // Tao UART event task
    if (cf_threadpool_submit(uart_event_task, NULL, CF_THREADPOOL_PRIORITY_HIGH, CF_NO_WAIT) != CF_OK) {
        CF_LOG_E("Failed to create UART event task");
        return CF_ERROR;
    }
    
    CF_LOG_I("Framework initialized with UART LED control");
    return CF_OK;
}

//==============================================================================
// MAIN APPLICATION
//==============================================================================

void app_main(void)
{
    printf("ESP32 CF UART LED Control Demo\n");
    
    if (init_framework() != CF_OK) {
        printf("Init failed, stopping\n");
        return;
    }
    
    CF_LOG_I("Starting UART LED control demo...");
    CF_LOG_I("Send 'ON' or 'OFF' via UART0 (GPIO43/44) to control LED");
    CF_LOG_I("UART0 Settings: 115200 baud, 8N1");
    
    // Gui thong bao khoi dong qua UART
    const char* welcome_msg = "ESP32 LED Control Ready. Send ON/OFF commands.\r\n";
    uart_write_bytes(UART_PORT, welcome_msg, strlen(welcome_msg));
    
    CF_LOG_I("UART LED control system started");
    
    // Vong lap xu ly CF queue
    while (true) {
        queue_message_t received_msg;
        
        if (cf_queue_receive(main_queue, &received_msg, CF_WAIT_FOREVER) == CF_OK) {
            
            switch (received_msg.type) {
                case MSG_UART_DATA_RECEIVED:
                    CF_LOG_D("Processing UART data from queue");
                    cf_threadpool_submit(uart_receive_task, received_msg.data,
                                        CF_THREADPOOL_PRIORITY_HIGH, CF_NO_WAIT);
                    break;
                    
                case MSG_UART_SEND_RESPONSE:
                    CF_LOG_D("Processing UART send from queue");
                    cf_threadpool_submit(uart_send_task, received_msg.data,
                                        CF_THREADPOOL_PRIORITY_NORMAL, CF_NO_WAIT);
                    break;
                    
                case MSG_LED_ON:
                    CF_LOG_D("Processing LED ON from queue");
                    cf_threadpool_submit(led_control_task, received_msg.data,
                                        CF_THREADPOOL_PRIORITY_HIGH, CF_NO_WAIT);
                    break;
                    
                case MSG_LED_OFF:
                    CF_LOG_D("Processing LED OFF from queue");
                    cf_threadpool_submit(led_control_task, received_msg.data,
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