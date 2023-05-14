#include "beamer.h"
#include "esp_log.h"
#include "watchdog.h"

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

void calc_crc(char *data, size_t length)
{
    char crc = 0;
    for (size_t i = 0; i < length - 1; ++i)
    {
        crc += data[i];
    }
    data[length - 1] = crc;
}

void update_power(struct homie_handle_s *handle, int node, int property)
{
    if (xSemaphoreTake(beamer_state.mutex, (portTickType)portMAX_DELAY) ==
        pdTRUE)
    {
        watchdog_reset(); // should be called every 5s

        // 60 after switch off to let beamer go to deep sleep
        if ((xTaskGetTickCount() -
             (beamer_state.state ? 30000 : 60000) / portTICK_PERIOD_MS) >
            beamer_state.last_change)
        {
            char cmd[] = {0x00, 0x85, 0x00, 0x00, 0x01, 0x01, 0x00};
            calc_crc(cmd, sizeof(cmd));
            uart_write(&uart, cmd, sizeof(cmd));
            uart_cycle(&uart);
            char value[100] = {0};
            size_t len = 99;
            uart_get_buffer(&uart, value, &len);
            ESP_LOGI(TAG, "pwr value %d", len);
            if (len > 0 && value[0] == 0x20 && value[1] == 0x85)
            {
                if (value[7] == 0x01)
                {
                    beamer_state.state = HOMIE_TRUE;
                }
                else
                {
                    // special case if beamer was switched of by remote
                    beamer_state.last_change = beamer_state.state
                                                   ? xTaskGetTickCount()
                                                   : beamer_state.last_change;
                    beamer_state.state = HOMIE_FALSE;
                }
                ESP_LOGI(TAG, "power status %d", beamer_state.state);
            }
        }
        else
        {
            ESP_LOGI(TAG, "skip power status request");
        }
        char value[100];
        sprintf(value, "%s",
                (beamer_state.state == HOMIE_FALSE) ? "false" : "true");
        ESP_LOGD(TAG, "power status %s", value);

        homie_publish_property_value(handle, node, property, value);
        xSemaphoreGive(beamer_state.mutex);
    }
    else
    {
        ESP_LOGI(TAG, "skip power status request, no semaphore");
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
            // const char cmd[] = "power on\r\n\0";
            char cmd[] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00};
            calc_crc(cmd, sizeof(cmd));
            ESP_LOGI(TAG, "set pwr on");
            uart_write(&uart, cmd, sizeof(cmd));
        }
        else
        {
            beamer_state.state = HOMIE_FALSE;
            char cmd[] = {0x02, 0x01, 0x00, 0x00, 0x00, 0x00};
            calc_crc(cmd, sizeof(cmd));
            ESP_LOGI(TAG, "set pwr off");
            uart_write(&uart, cmd, sizeof(cmd));
        }
        xSemaphoreGive(beamer_state.mutex);
    }
}

void update_source(struct homie_handle_s *handle, int node, int property)
{
    if (xSemaphoreTake(beamer_state.mutex, (portTickType)portMAX_DELAY) ==
        pdTRUE)
    {
        if ((xTaskGetTickCount() - 30000 / portTICK_PERIOD_MS) >
                beamer_state.last_change &&
            beamer_state.state == HOMIE_TRUE)
        {
            char cmd[] = {0x00, 0x85, 0x00, 0x00, 0x01, 0x02, 0x00};
            calc_crc(cmd, sizeof(cmd));
            uart_write(&uart, cmd, sizeof(cmd));

            uart_cycle(&uart);

            char value[100] = {0};
            size_t len = 99;
            uart_get_buffer(&uart, value, &len);
            ESP_LOGI(TAG, "source value %d", len);
            if (len > 0 && value[0] == 0x20 && value[1] == 0x85)
            {
                ESP_LOGD(TAG, "source 6 0x%x", value[6]);
                ESP_LOGD(TAG, "source 7 0x%x", value[7]);
                ESP_LOGD(TAG, "source 8 0x%x", value[8]);
                char source[100] = {0};
                switch (value[8])
                {
                case 0x01:
                    strcpy(source, "computer");
                    break;
                case 0x21:
                    switch (value[7])
                    {
                    case 0x01:
                        strcpy(source, "hdmi1");
                        break;
                    case 0x02:
                        strcpy(source, "hdmi2");
                        break;
                    };
                    break;
                case 0x27:
                    strcpy(source, "hdbaset");
                    break;
                case 0x07:
                    switch (value[7])
                    {
                    case 0x01:
                        strcpy(source, "usb-a");
                        break;
                    case 0x02:
                        strcpy(source, "lan");
                        break;
                    };
                    break;
                default:
                    strcpy(source, "UNKNOWN");
                    break;
                };
                homie_publish_property_value(handle, node, property, source);
            }
        }
        else
        {
            ESP_LOGI(TAG, "skip source request -> power off");
            char source[100] = {0};
            strcpy(source, "POWEROFF");
            homie_publish_property_value(handle, node, property, source);
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
            // 02h 03h 00h 00h 02h 01h <channel> <crc>
            //  hdmi1|hdmi2|computer|hdbaset|usb-a|lan
            char cmd[] = {0x02, 0x03, 0x00, 0x00, 0x02, 0x01, 0x00, 0x00};
            if (strncmp(data, "hdmi1", data_len) == 0)
            {
                cmd[6] = 0xa1;
            }
            else if (strncmp(data, "hdmi2", data_len) == 0)
            {
                cmd[6] = 0xa2;
            }
            else if (strncmp(data, "computer", data_len) == 0)
            {
                cmd[6] = 0x01;
            }
            else if (strncmp(data, "hdbaset", data_len) == 0)
            {
                cmd[6] = 0xbf;
            }
            else if (strncmp(data, "usb-a", data_len) == 0)
            {
                cmd[6] = 0x1f;
            }
            else if (strncmp(data, "lan", data_len) == 0)
            {
                cmd[6] = 0x20;
            }
            calc_crc(cmd, sizeof(cmd));
            ESP_LOGI(TAG, "set source got %d", sizeof(cmd));
            uart_write(&uart, cmd, sizeof(cmd));
        }
        xSemaphoreGive(beamer_state.mutex);
    }
}
