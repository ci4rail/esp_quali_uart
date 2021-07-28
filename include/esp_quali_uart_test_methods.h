#ifndef _ESP_QUALI_UART_TEST_METHODS_H_
#define _ESP_QUALI_UART_TEST_METHODS_H_

typedef struct {
    int (*destroy)(void *hdl );
} EspQualiUartTestMethods;

#endif // _ESP_QUALI_UART_TEST_METHODS_H_