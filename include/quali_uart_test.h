#ifndef _QUALI_UART_TEST_H_
#define _QUALI_UART_TEST_H_

#include "driver/uart.h"
#include "test_status_report.h"

typedef struct quali_uart_test_handle_t quali_uart_test_handle_t;

struct quali_uart_test_handle_t {
    esp_err_t (*destroy)(quali_uart_test_handle_t *hdl);
};

typedef struct {
    uart_port_t uart_num;
    const uart_config_t *uart_config;
    int rx_buf_size;
    int tx_buf_size;
    int gpio_tx;
    int gpio_rx;
    int gpio_rts;
    int gpio_cts;
    test_status_report_handle_t *reporter;
} quali_uart_test_config_t;

esp_err_t new_uart_test(quali_uart_test_handle_t **hdl_p, quali_uart_test_config_t *test_config);

#endif  // _QUALI_UART_TEST_H_
