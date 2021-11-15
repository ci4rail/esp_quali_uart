#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "quali_uart_test.h"
#include "test_status_report.h"
#include "driver/gpio.h"

#define TASK_STACK_SIZE (2048)
#define INTR_ALLOC_FLAGS (0)  // or ESP_INTR_FLAG_IRAM

typedef struct {
    quali_uart_test_handle_t methods;

    TaskHandle_t control_task;
    uart_port_t uart_num;
    int gpio_rts;
    int gpio_cts;
    test_status_report_handle_t *reporter;
    char tag[30];
    bool stop_flag;
} uart_test_private_t;


static void run_uart_test(void *arg)
{
    uart_test_private_t *hdl = (uart_test_private_t*)arg;
    char *send_data = "HELLO";
    int send_len = strlen(send_data);
    char recv_data[20];
    char report[80];

    test_status_report_handle_t *rep = hdl->reporter;

    ESP_LOGI(hdl->tag, "Starting test");
    while(! hdl->stop_flag) {
        uart_flush(hdl->uart_num); // discard anything in receive buffer

        uart_write_bytes(hdl->uart_num, (const char *) send_data, strlen(send_data));

        int len = uart_read_bytes(hdl->uart_num, recv_data, sizeof(recv_data), 100 / portTICK_RATE_MS);

        if( len != send_len ){
            sprintf(report,"ERR: read wrong number of bytes %d\n", len);
            rep->report_status(rep, report);
            ESP_LOGE(hdl->tag, "%s", report);
        }
        else if( memcmp(send_data, recv_data, send_len) != 0){
            sprintf(report,"ERR: wrong data received\n");
            rep->report_status(rep, report);
            ESP_LOGE(hdl->tag, "%s", report);
        }
        else {
            sprintf(report,"INF: read %d bytes ok\n", len);
            rep->report_status(rep, report);
            ESP_LOGI(hdl->tag, "%s", report);
        }
    }
    vTaskDelete(NULL);
}

static void uart_test_rts_cts_pins(void *arg)
{
    uart_test_private_t *hdl = (uart_test_private_t*)arg;
    int rts_pin_state = 0;
    int cts_pin_state = 0;
    char report[80];

    test_status_report_handle_t *rep = hdl->reporter;

    /* initialize rts pin state */
    rts_pin_state = gpio_get_level(hdl->gpio_rts);

    ESP_LOGI(hdl->tag, "Starting RTS/CTS Pin Test");
    while(! hdl->stop_flag) {
        if ((cts_pin_state = gpio_get_level(hdl->gpio_cts)) != (!rts_pin_state)) {
            sprintf(report,"ERR: wrong cts/rts pin state (set rts: %d, read cts: %d)\n", !rts_pin_state, cts_pin_state);
            rep->report_status(rep, report);
            ESP_LOGE(hdl->tag, "wrong cts pin state (set rts: %d, read cts: %d)", !rts_pin_state, cts_pin_state);
        }
        rts_pin_state ^= 1;
        uart_set_rts(hdl->uart_num, rts_pin_state);
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    vTaskDelete(NULL);
}

static void uart_test_control(void *arg)
{
    uart_test_private_t *hdl = (uart_test_private_t*)arg;
    test_status_report_handle_t *rep = hdl->reporter;

    while(1) {
        TaskHandle_t task;
        ESP_LOGI(hdl->tag, "test_control wait for start");
        vTaskDelay(pdMS_TO_TICKS(100));

        rep->wait_for_start(rep);
        hdl->stop_flag = false;
        if(xTaskCreate(run_uart_test, hdl->tag, TASK_STACK_SIZE, (void*)hdl, 10, &task) != pdPASS){
            rep->report_status(rep, "ERR: can't create run_uart_test task");
            continue;
        }

        if(xTaskCreate(uart_test_rts_cts_pins, hdl->tag, TASK_STACK_SIZE, (void*)hdl, 10, &task) != pdPASS){
            rep->report_status(rep, "ERR: can't create uart_test_rts_cts_pins task");
            continue;
        }

        ESP_LOGI(hdl->tag, "test_control wait for stop");
        rep->wait_for_stop(rep);
        hdl->stop_flag = true;
    }
}

static esp_err_t destroy(uart_test_private_t *hdl)
{
    return ESP_ERR_NOT_SUPPORTED; 
}

esp_err_t new_uart_test(quali_uart_test_handle_t **hdl_p, quali_uart_test_config_t *test_config)
{
    uart_test_private_t *hdl;
    *hdl_p = NULL;
    char control_task_name[40];

    if( (hdl = (uart_test_private_t *)calloc(1, sizeof(*hdl))) == NULL){
        return ESP_ERR_NO_MEM;
    }
    hdl->methods.destroy = (esp_err_t (*)(quali_uart_test_handle_t *))destroy;

    hdl->uart_num = test_config->uart_num;
    hdl->gpio_rts = test_config->gpio_rts;
    hdl->gpio_cts = test_config->gpio_cts;
    hdl->reporter = test_config->reporter;

    sprintf(hdl->tag, "uart%d_test", hdl->uart_num);

    if(test_config->uart_config->flow_ctrl != UART_HW_FLOWCTRL_DISABLE)
    {
        ESP_LOGE(hdl->tag, "Error flow control was not set to disable");
        return ESP_FAIL;
    }

    /* setup uart driver */
    if (uart_driver_install(test_config->uart_num, test_config->rx_buf_size, test_config->tx_buf_size, 0, NULL, INTR_ALLOC_FLAGS) != ESP_OK) {
        ESP_LOGE(hdl->tag, "Error could not install uart driver");
        return ESP_FAIL;
    }

    if (uart_param_config(test_config->uart_num, test_config->uart_config) != ESP_OK) {
        ESP_LOGE(hdl->tag, "Error could not configure uart driver");
        return ESP_FAIL;
    }

    if (uart_set_pin(test_config->uart_num, test_config->gpio_tx, test_config->gpio_rx, test_config->gpio_rts, test_config->gpio_cts)!= ESP_OK) {
        ESP_LOGE(hdl->tag, "Error could not set uart pins");
        return ESP_FAIL;
    }

    sprintf(control_task_name, "uart%d_test_control", hdl->uart_num);

    if(xTaskCreate(uart_test_control, control_task_name, TASK_STACK_SIZE, (void*)hdl, 10, &hdl->control_task) != pdPASS){
        ESP_LOGE(hdl->tag, "Error could not start uart test control task");
        return ESP_FAIL;
    }

    *hdl_p = &hdl->methods;
    return ESP_OK;
}