#include "beamer.h"
#include "esp_log.h"

static const char *TAG = "beamer-control";

beamer_state_t beamer_state = {
    .state = HOMIE_FALSE,
    .last_change = 0,
};

uart_handle_t uart = {
    .configRx = {.baud_rate = 38400,
                 .data_bits = UART_DATA_8_BITS,
                 .parity = UART_PARITY_DISABLE,
                 .stop_bits = UART_STOP_BITS_1,
                 .flow_ctrl = UART_HW_FLOWCTRL_DISABLE},
    .configTx = {.baud_rate = 38400,
                 .data_bits = UART_DATA_8_BITS,
                 .parity = UART_PARITY_DISABLE,
                 .stop_bits = UART_STOP_BITS_1,
                 .flow_ctrl = UART_HW_FLOWCTRL_DISABLE},
    .wait_ticks = 5000 / portTICK_PERIOD_MS,
};

void update_power(struct homie_handle_s *handle, int node, int property)
{
    if (xSemaphoreTake(beamer_state.mutex, (portTickType)portMAX_DELAY) ==
        pdTRUE)
    {
        if ((xTaskGetTickCount() - 25000 / portTICK_PERIOD_MS) >
            beamer_state.last_change)
        {
            const char cmd[] = "power\r\n\0";
            uart_write(&uart, cmd, strlen(cmd));

            uart_cycle(&uart);

            char value[100] = {0};
            size_t len = 99;
            uart_get_buffer(&uart, value, &len);
            char status[100] = {0};
            ESP_LOGI(TAG, "pwr value %d %s", len, value);
            if (len > 0 && sscanf(value, "power %s", status) == 1)
            {
                beamer_state.state =
                    (strcmp(status, "on") == 0) ? HOMIE_TRUE : HOMIE_FALSE;
                ESP_LOGI(TAG, "power status %s", status);
            }
        }
        else
        {
            ESP_LOGD(TAG, "skip power status request");
        }
        char value[100];
        sprintf(value, "%s",
                (beamer_state.state == HOMIE_FALSE) ? "false" : "true");
        ESP_LOGD(TAG, "power status %s", value);

        homie_publish_property_value(handle, node, property, value);
        xSemaphoreGive(beamer_state.mutex);
    }
}

void write_power(struct homie_handle_s *handle, int node, int property,
                 const char *data, int data_len)
{
    if (xSemaphoreTake(beamer_state.mutex, (portTickType)portMAX_DELAY) ==
        pdTRUE)
    {
        beamer_state.last_change = xTaskGetTickCount();
        if (strncmp(data, "true", data_len) == 0)
        {
            beamer_state.state = HOMIE_TRUE;
            const char cmd[] = "power on\r\n\0";
            ESP_LOGI(TAG, "set pwr got %d %s", strlen(cmd), cmd);
            uart_write(&uart, cmd, strlen(cmd));
        }
        else
        {
            beamer_state.state = HOMIE_FALSE;
            const char cmd[] = "power off\r\n\0";
            ESP_LOGI(TAG, "set pwr got %d %s", strlen(cmd), cmd);
            uart_write(&uart, cmd, strlen(cmd));
        }
        xSemaphoreGive(beamer_state.mutex);
    }
}

void update_source(struct homie_handle_s *handle, int node, int property)
{
    if (xSemaphoreTake(beamer_state.mutex, (portTickType)portMAX_DELAY) ==
        pdTRUE)
    {
        if ((xTaskGetTickCount() - 25000 / portTICK_PERIOD_MS) >
                beamer_state.last_change &&
            beamer_state.state == HOMIE_TRUE)
        {
            const char cmd[] = "input\r\n\0";
            uart_write(&uart, cmd, strlen(cmd));

            uart_cycle(&uart);

            char value[100] = {0};
            size_t len = 99;
            uart_get_buffer(&uart, value, &len);
            char source[100] = {0};
            ESP_LOGD(TAG, "source value %d %s", len, value);
            if (len > 0 && sscanf(value, "input %s", source) == 1)
            {
                ESP_LOGI(TAG, "source %s", source);
                homie_publish_property_value(handle, node, property, source);
            }
        }
        else
        {
            ESP_LOGD(TAG, "skip source request -> power off");
        }
        xSemaphoreGive(beamer_state.mutex);
    }
}

void write_source(struct homie_handle_s *handle, int node, int property,
                  const char *data, int data_len)
{
    if (xSemaphoreTake(beamer_state.mutex, (portTickType)portMAX_DELAY) ==
        pdTRUE)
    {
        if (data_len > 0)
        {
            char cmd[100] = {0};
            snprintf(cmd, 99, "source %s\r\n", data);
            ESP_LOGI(TAG, "set source got %d %s", strlen(cmd), cmd);
            uart_write(&uart, cmd, strlen(cmd));
        }
        xSemaphoreGive(beamer_state.mutex);
    }
}
