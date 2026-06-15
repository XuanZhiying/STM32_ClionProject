/**
 ****************************************************************************************************
 * @file        dht22_task.c
 * @author      Sisyphus
 * @version     V1.0
 * @date        2026-06-15
 * @brief       DHT22 FreeRTOS 读取任务, 带时间戳和原始字节日志
 ****************************************************************************************************
 */

#include "./BSP/DHT22/dht22.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>

void DHT22_Task(void *pvParameters)
{
    float    temperature = 0.0f;
    float    humidity    = 0.0f;
    uint8_t  raw[5]      = {0};
    int      ret;
    uint32_t tick, ms;

    (void)pvParameters;

    vTaskDelay(pdMS_TO_TICKS(2000));

    while (1)
    {
        ret = dht22_read(&temperature, &humidity, raw);

        /* 时间戳: uptime 秒.毫秒 */
        tick = xTaskGetTickCount();
        ms   = tick * portTICK_PERIOD_MS;

        /* 原始字节 + 校验和 (ESP32 DHT22.c:190 参考) */
        printf("[%lu.%03lu] DHT[0]=%02X DHT[1]=%02X DHT[2]=%02X DHT[3]=%02X DHT[4]=%02X check=%02X\r\n",
               ms / 1000, ms % 1000,
               raw[0], raw[1], raw[2], raw[3], raw[4],
               (uint8_t)(raw[0] + raw[1] + raw[2] + raw[3]));

        switch (ret)
        {
            case DHT_OK:
                printf("[%lu.%03lu] Humidity=%.1f%%  Temperature=%.1f°C\r\n",
                       ms / 1000, ms % 1000, humidity, temperature);
                break;

            case DHT_CHECKSUM_ERROR:
                printf("[%lu.%03lu] DHT22: Checksum error!\r\n", ms / 1000, ms % 1000);
                break;

            case DHT_TIMEOUT_ERROR:
                printf("[%lu.%03lu] DHT22: Sensor no response!\r\n", ms / 1000, ms % 1000);
                break;

            case DHT_VALUE_ERROR:
                printf("[%lu.%03lu] DHT22: Value out of range! (H=%.1f T=%.1f)\r\n",
                       ms / 1000, ms % 1000, humidity, temperature);
                break;

            default:
                printf("[%lu.%03lu] DHT22: Unknown error (%d)\r\n", ms / 1000, ms % 1000, ret);
                break;
        }

        vTaskDelay(pdMS_TO_TICKS(1000*15));
    }
}
