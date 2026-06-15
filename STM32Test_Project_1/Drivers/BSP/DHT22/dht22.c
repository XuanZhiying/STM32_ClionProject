/**
 ****************************************************************************************************
 * @file        dht22.c
 * @author      Sisyphus
 * @version     V1.0
 * @date        2026-06-15
 * @brief       DHT22 温湿度传感器驱动实现
 * @note        基于 ESP32 版本移植, 使用 delay_us() 做微秒延时, taskENTER_CRITICAL 保护时序
 ****************************************************************************************************
 */

#include "./BSP/DHT22/dht22.h"
#include "./SYSTEM/delay/delay.h"
#include "FreeRTOS.h"
#include "task.h"

/* 内部函数: 切换 GPIO 模式 */
static void dht22_gpio_output(void);
static void dht22_gpio_input(void);
static uint8_t dht22_read_byte(void);

/**
 * @brief       DHT22 GPIO 初始化
 * @note        上电初始化为推挽输出, 拉高总线空闲状态
 * @param       无
 * @retval      无
 */
void dht22_init(void)
{
    GPIO_InitTypeDef gpio_init_struct;

    DHT22_GPIO_CLK_ENABLE();

    gpio_init_struct.Pin   = DHT22_GPIO_PIN;
    gpio_init_struct.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio_init_struct.Pull  = GPIO_NOPULL;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DHT22_GPIO_PORT, &gpio_init_struct);

    HAL_GPIO_WritePin(DHT22_GPIO_PORT, DHT22_GPIO_PIN, GPIO_PIN_SET);
}

/**
 * @brief       切换 GPIO 为推挽输出模式 (用于主机发送起始信号)
 */
static void dht22_gpio_output(void)
{
    GPIO_InitTypeDef gpio_init_struct;

    gpio_init_struct.Pin   = DHT22_GPIO_PIN;
    gpio_init_struct.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio_init_struct.Pull  = GPIO_NOPULL;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DHT22_GPIO_PORT, &gpio_init_struct);
}

/**
 * @brief       切换 GPIO 为浮空输入模式 (用于读取传感器响应)
 */
static void dht22_gpio_input(void)
{
    GPIO_InitTypeDef gpio_init_struct;

    gpio_init_struct.Pin   = DHT22_GPIO_PIN;
    gpio_init_struct.Mode  = GPIO_MODE_INPUT;
    gpio_init_struct.Pull  = GPIO_PULLUP;
    HAL_GPIO_Init(DHT22_GPIO_PORT, &gpio_init_struct);
}

/**
 * @brief       读取 DHT22 的一个字节 (8 位)
 * @note        电平采样法: 每位起始后延时 40us 采样,
 *              高电平 > 45us 为 1, < 45us 为 0
 */
static uint8_t dht22_read_byte(void)
{
    uint8_t byte = 0;

    for (int i = 0; i < 8; i++)
    {
        /* 等待引脚变高 (数据位的起始) */
        while (!HAL_GPIO_ReadPin(DHT22_GPIO_PORT, DHT22_GPIO_PIN));

        /* 延时 40us 后采样 */
        delay_us(40);

        byte <<= 1;
        if (HAL_GPIO_ReadPin(DHT22_GPIO_PORT, DHT22_GPIO_PIN))
        {
            byte |= 1;      /* 1: 高电平持续 ~70us, 40us 时仍为高 */
        }
        /* else: 0: 高电平持续 ~27us, 40us 时已变低 */

        /* 等待引脚变低 (该数据位结束) */
        while (HAL_GPIO_ReadPin(DHT22_GPIO_PORT, DHT22_GPIO_PIN));
    }

    return byte;
}

/**
 * @brief       读取 DHT22 温湿度数据 (核心函数)
 * @param       temperature: 输出温度值 (℃)
 * @param       humidity:    输出湿度值 (%)
 * @param       raw_data:    原始 5 字节数据 (可为 NULL)
 * @retval      DHT_OK / DHT_VALUE_ERROR / DHT_CHECKSUM_ERROR / DHT_TIMEOUT_ERROR
 * @note        最多重试 3 次 (参考 ESP32 项目), 需要 FreeRTOS 环境
 */
int dht22_read(float *temperature, float *humidity, uint8_t *raw_data)
{
    uint8_t  data[5];
    uint32_t timeout;
    int      result = DHT_TIMEOUT_ERROR;

    if (temperature == NULL || humidity == NULL)
    {
        return DHT_TIMEOUT_ERROR;
    }

    for (int retry = 0; retry < 3; retry++)
    {
        for (int i = 0; i < 5; i++) data[i] = 0;

        taskENTER_CRITICAL();

        /* --- 步骤 1: 发送起始信号 --- */
        dht22_gpio_output();
        HAL_GPIO_WritePin(DHT22_GPIO_PORT, DHT22_GPIO_PIN, GPIO_PIN_RESET);
        delay_us(1000);
        HAL_GPIO_WritePin(DHT22_GPIO_PORT, DHT22_GPIO_PIN, GPIO_PIN_SET);
        delay_us(30);

        /* --- 步骤 2: 切换输入, 等待传感器响应 --- */
        dht22_gpio_input();

        timeout = 200;
        while (HAL_GPIO_ReadPin(DHT22_GPIO_PORT, DHT22_GPIO_PIN) && --timeout)
            delay_us(1);
        if (timeout == 0) { result = DHT_TIMEOUT_ERROR; goto retry_wait; }

        timeout = 200;
        while (!HAL_GPIO_ReadPin(DHT22_GPIO_PORT, DHT22_GPIO_PIN) && --timeout)
            delay_us(1);
        if (timeout == 0) { result = DHT_TIMEOUT_ERROR; goto retry_wait; }

        timeout = 200;
        while (HAL_GPIO_ReadPin(DHT22_GPIO_PORT, DHT22_GPIO_PIN) && --timeout)
            delay_us(1);
        if (timeout == 0) { result = DHT_TIMEOUT_ERROR; goto retry_wait; }

        /* --- 步骤 3: 读取 40 位数据 (5 字节) --- */
        for (int i = 0; i < 5; i++)
            data[i] = dht22_read_byte();

        taskEXIT_CRITICAL();

        /* --- 步骤 4: 校验和验证 --- */
        uint8_t checksum = (uint8_t)(data[0] + data[1] + data[2] + data[3]);
        if (checksum != data[4])
        {
            result = DHT_CHECKSUM_ERROR;
            goto retry_wait;
        }

        /* --- 步骤 5: 解析温湿度 --- */
        *humidity    = (float)(data[0] * 256 + data[1]) / 10.0f;
        int16_t temp_raw = (int16_t)((data[2] & 0x7F) * 256 + data[3]);
        *temperature = (float)temp_raw / 10.0f;
        if (data[2] & 0x80)
            *temperature *= -1.0f;

        /* --- 步骤 6: 数值合理性检查 --- */
        if (*humidity < 0.0f || *humidity > 100.0f
         || *temperature < -40.0f || *temperature > 80.0f)
        {
            result = DHT_VALUE_ERROR;
            goto retry_wait;
        }

        /* 成功: 输出原始数据 */
        if (raw_data != NULL)
        {
            raw_data[0] = data[0];
            raw_data[1] = data[1];
            raw_data[2] = data[2];
            raw_data[3] = data[3];
            raw_data[4] = data[4];
        }

        return DHT_OK;

    retry_wait:
        taskEXIT_CRITICAL();
        if (retry < 2)
            vTaskDelay(pdMS_TO_TICKS(500));
    }

    /* 所有重试均失败, raw_data 输出最后一次读取的原始数据 (供调试) */
    if (raw_data != NULL)
    {
        raw_data[0] = data[0];
        raw_data[1] = data[1];
        raw_data[2] = data[2];
        raw_data[3] = data[3];
        raw_data[4] = data[4];
    }

    return result;
}
