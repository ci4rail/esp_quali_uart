cmake_minimum_required(VERSION 3.5)

set(include_public
    "${COMPONENT_DIR}/include"
    )

set(include_private
    "${COMPONENT_DIR}/include_private"
    )

set(srcs
    "${COMPONENT_DIR}/src/uart_test.c"
    )

idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS "${include_public}"
                       REQUIRES driver esp_test_status_report)
