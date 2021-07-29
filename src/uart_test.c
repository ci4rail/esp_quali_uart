#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "quali_uart_test.h"
#include "test_status_report.h"

#define TASK_STACK_SIZE (2048)

typedef struct {
    quali_uart_test_handle_t methods;

    TaskHandle_t control_task;
    uart_port_t uart_num;
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
        uart_write_bytes(hdl->uart_num, (const char *) send_data, strlen(send_data));

        int len = uart_read_bytes(hdl->uart_num, recv_data, sizeof(recv_data), 200 / portTICK_RATE_MS);
        
        sprintf(report,"INF: read %d bytes", len);
        rep->report_status(rep, report);
        ESP_LOGI(hdl->tag, "read %d bytes", len);

        if( len != send_len ){
            sprintf(report,"ERR: read wrong number of bytes %d", len);
            rep->report_status(rep, report);
            ESP_LOGE(hdl->tag, "Wrong number of bytes read");
            uart_flush(hdl->uart_num);
        }
    }
    vTaskDelete(NULL);
}

static void uart_test_control(void *arg)
{
    uart_test_private_t *hdl = (uart_test_private_t*)arg;
    test_status_report_handle_t *rep = hdl->reporter;

    while(1) {
        TaskHandle_t task;

        rep->wait_for_start(rep);

        if(xTaskCreate(run_uart_test, hdl->tag, TASK_STACK_SIZE, (void*)hdl, 10, &task) != pdPASS){
            // tell reporter
            continue;
        }

        rep->wait_for_stop(rep);
        hdl->stop_flag = true;
    }
}

static esp_err_t destroy(uart_test_private_t *hdl)
{
    if( hdl->control_task)
        vTaskDelete(hdl->control_task);
    free(hdl);
    return ESP_OK;
}

esp_err_t new_uart_test(quali_uart_test_handle_t **hdl_p, uart_port_t uart_num, test_status_report_handle_t *reporter )
{
    uart_test_private_t *hdl;
    *hdl_p = NULL;
    char control_task_name[40];

    if( (hdl = (uart_test_private_t *)calloc(1, sizeof(*hdl))) == NULL){
        return ESP_ERR_NO_MEM;
    }
    hdl->methods.destroy = (int (*)(void *))destroy;

    hdl->uart_num = uart_num;
    hdl->reporter = reporter;
    sprintf(hdl->tag, "uart%d_test", uart_num);
    sprintf(control_task_name, "uart%d_test_control", uart_num);

    if(xTaskCreate(uart_test_control, control_task_name, TASK_STACK_SIZE, (void*)hdl, 10, &hdl->control_task) != pdPASS){
        return ESP_FAIL;
    }

    *hdl_p = &hdl->methods;
    return ESP_OK;
}