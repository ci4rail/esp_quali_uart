#ifndef _ESP_QUALI_UART_TEST_METHODS_H_
#define _ESP_QUALI_UART_TEST_METHODS_H_

typedef struct {
    esp_err_t (*destroy)(void *hdl );
} EspQualiUartTestMethods;

#endif // _ESP_QUALI_UART_TEST_METHODS_H_