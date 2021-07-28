#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_quali_uart_test.h"

#define TASK_STACK_SIZE (4096)

typedef struct {
    EspQualiUartTestMethods methods;

    TaskHandle_t *task;
    uart_port_t uart_num;
    void *reporter;
    char tag[30];
} UartTestHandle;


static void run_uart_test(void *arg)
{
    UartTestHandle *hdl = (UartTestHandle*)arg;
    char *send_data = "HELLO";
    int send_len = strlen(send_data);
    char recv_data[20];

    ESP_LOGI(hdl->tag, "Starting test");
    while(1) {
        uart_write_bytes(hdl->uart_num, (const char *) send_data, strlen(send_data));

        int len = uart_read_bytes(hdl->uart_num, recv_data, sizeof(recv_data), 200 / portTICK_RATE_MS);
        ESP_LOGI(hdl->tag, "read %d bytes", len);

        if( len != send_len ){
            ESP_LOGE(hdl->tag, "Wrong number of bytes read");
            uart_flush(hdl->uart_num);
        }
    }

}

static int destroy(UartTestHandle *hdl)
{
    if( hdl->task)
        vTaskDelete(hdl->task);
    free(hdl);
    return ESP_OK;
}

int new_uart_test(EspQualiUartTestMethods **hdl_p, uart_port_t uart_num, /*TODO: Replace with real structure*/ void *reporter )
{
    UartTestHandle *hdl;
    *hdl_p = NULL;

    if( (hdl = (UartTestHandle *)calloc(1, sizeof(*hdl))) == NULL){
        return ESP_ERR_NO_MEM;
    }
    hdl->methods.destroy = (int (*)(void *))destroy;

    hdl->uart_num = uart_num;
    hdl->reporter = reporter;
    sprintf(hdl->tag, "uart%d_test", uart_num);

    if(xTaskCreate(run_uart_test, hdl->tag, TASK_STACK_SIZE, (void*)UART_NUM_1, 10, &hdl->task) != pdPASS){
        return ESP_FAIL;
    }

    *hdl_p = hdl;
    return ESP_OK;
}