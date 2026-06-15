/**
 ****************************************************************************************************
 * @file        delay.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.1
 * @date        2023-08-01
 * @brief       使用SysTick的普通模式对延迟进行管理(支持FreeRTOS)
 *              提供delay_init初始化, delay_us和delay_ms延时函数
 *              [已适配 FreeRTOS: vTaskDelay / vTaskSuspendAll / xTaskResumeAll]
 * @license     Copyright (c) 2022-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:正点原子 M48Z-M3小系统板STM32F103
 * 技术视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 * 公司网址:www.alientek.com
 * 购买地址:openedv.taobao.com
 *
 ****************************************************************************************************
 */

#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/delay/delay.h"

static uint32_t g_fac_us = 0;       /* us延时倍乘数 */

#if SYS_SUPPORT_OS
/* FreeRTOS 延时适配层
 * 3个宏定义:
 *   delay_osrunning    : 表示OS当前是否正在运行, 以决定是否可以使用相关函数
 *   delay_ostickspersec: 表示OS设定的时钟节拍数, delay_init将根据此参数来初始化systick
 *   delay_osintnesting : 表示OS中断嵌套级别, 因为中断里面不可以调度, delay_ms使用该参数来选择
 * 3个函数:
 *   delay_osschedlock  : 用于锁定OS任务调度, 禁止调度
 *   delay_osschedunlock: 用于解锁OS任务调度, 重新开启调度
 *   delay_ostimedly    : 用于OS延时, 调用OS自带的延时函数.
 */

static uint16_t g_fac_ms = 0;

/* FreeRTOS 适配宏: 调度器状态 → OS是否运行 */
#define delay_osrunning     (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)

/* FreeRTOS 适配宏: 系统时钟节拍 (来自 FreeRTOSConfig.h) */
#define delay_ostickspersec configTICK_RATE_HZ

/* FreeRTOS 适配宏: 是否在中断中 (ICSR.VECTACTIVE != 0 表示在中断服务例程中) */
#define delay_osintnesting  ((SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) != 0)

/**
 * @brief     us延时时, 关闭任务调度(防止打断us级延迟)
 * @param     无
 * @retval    无
 */
static void delay_osschedlock(void)
{
    vTaskSuspendAll();                          /* FreeRTOS: 挂起所有任务 */
}

/**
 * @brief     us延时时, 恢复任务调度
 * @param     无
 * @retval    无
 */
static void delay_osschedunlock(void)
{
    xTaskResumeAll();                           /* FreeRTOS: 恢复任务调度 */
}

/**
 * @brief     ms延时时, 调用OS自带延时 (释放CPU)
 * @param     ticks: 延时的节拍数
 * @retval    无
 */
static void delay_ostimedly(uint32_t ticks)
{
    vTaskDelay(ticks);                          /* FreeRTOS: 任务延时释放CPU */
}
#endif  /* SYS_SUPPORT_OS */

/**
 * @brief     初始化延迟函数
 * @note      FreeRTOS 模式下不配置 SysTick (由 FreeRTOS 管理)
 * @param     sysclk: 系统时钟频率, 即CPU频率, 72MHz
 * @retval    无
 */
void delay_init(uint16_t sysclk)
{
    g_fac_us = sysclk;                          /* 因HAL_Init已对systick做了配置，此处无需再次配置 */

#if SYS_SUPPORT_OS
    g_fac_ms = 1000 / delay_ostickspersec;      /* 每个tick对应的ms数 */
    /* FreeRTOS 自行管理 SysTick, 此处不干预寄存器 */
#endif
}

/**
 * @brief     延时 nus
 * @note      无论是否使用OS, 都用死等法进行us延时
 * @param     nus: 要延时的us数
 * @retval    无
 */
void delay_us(uint32_t nus)
{
    uint32_t ticks;
    uint32_t told, tnow, tcnt = 0;
    uint32_t reload = SysTick->LOAD;            /* LOAD的值 */

    ticks = nus * g_fac_us;                     /* 需要的节拍数 */

#if SYS_SUPPORT_OS
    delay_osschedlock();                        /* 阻止OS调度，防止打断us延时 */
#endif

    told = SysTick->VAL;                        /* 刚进入时的计数器值 */
    while (1)
    {
        tnow = SysTick->VAL;
        if (tnow != told)
        {
            if (tnow < told)
            {
                tcnt += told - tnow;
            }
            else
            {
                tcnt += reload - tnow + told;
            }
            told = tnow;
            if (tcnt >= ticks)
            {
                break;                          /* 时间超过/等于要延迟的时间, 则退出 */
            }
        }
    }

#if SYS_SUPPORT_OS
    delay_osschedunlock();                      /* 恢复OS调度 */
#endif
}

/**
 * @brief     延时 nms
 * @param     nms: 要延时的ms数
 * @retval    无
 */
void delay_ms(uint16_t nms)
{
#if SYS_SUPPORT_OS
    /* OS已运行 且 不在中断中 (中断内不可调度) */
    if (delay_osrunning && delay_osintnesting == 0)
    {
        if (nms >= g_fac_ms)                    /* 延时时间大于OS的最少时间周期 */
        {
            delay_ostimedly(nms / g_fac_ms);    /* OS延时, 释放CPU */
        }
        nms %= g_fac_ms;                        /* OS已经无法提供这么小的延时了, 采用普通方式延时 */
    }
#endif
    delay_us((uint32_t)(nms * 1000));           /* 普通方式延时 */
}

/**
 * @brief     HAL库内部调用的延时函数 (重写 HAL_Delay)
 * @param     Delay: 要延时的毫秒数
 * @retval    None
 */
void HAL_Delay(uint32_t Delay)
{
     delay_ms(Delay);
}
