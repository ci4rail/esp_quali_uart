#ifndef _ESP_QUALI_UART_TEST_H_
#define _ESP_QUALI_UART_TEST_H_

#include "esp_quali_uart_test_methods.h"

esp_err_t new_uart_test(EspQualiUartTestMethods **hdl_p, uart_port_t uart_num, /*TODO: Replace with real structure*/ void *reporter );


#endif // _ESP_QUALI_UART_TEST_H_