#ifndef _QUALI_UART_TEST_H_
#define _QUALI_UART_TEST_H_

#include "driver/uart.h"
#include "test_status_report.h"

typedef struct quali_uart_test_handle_t quali_uart_test_handle_t;

struct quali_uart_test_handle_t {
    esp_err_t (*destroy)(quali_uart_test_handle_t *hdl);
};

esp_err_t new_uart_test(quali_uart_test_handle_t **hdl_p, uart_port_t uart_num, test_status_report_handle_t *reporter);

#endif  // _QUALI_UART_TEST_H_
