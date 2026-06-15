/**
 ****************************************************************************************************
 * @file        sys.h
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-08-01
 * @brief       系统初始化头文件(含时钟配置/中断管理/GPIO设置等)
 *              [FreeRTOS 适配版]
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 */

#ifndef __SYS_H
#define __SYS_H

#include "stm32f1xx.h"

/**
 * SYS_SUPPORT_OS: 定义系统文件是否支持OS
 * 0: 不支持OS (裸机)
 * 1: 支持 FreeRTOS
 */
#define SYS_SUPPORT_OS          1

#if SYS_SUPPORT_OS
#include "FreeRTOS.h"
#include "task.h"
#endif

/* 函数声明 ***************************************************************************************/

void sys_nvic_set_vector_table(uint32_t baseaddr, uint32_t offset);             /* 设置中断偏移量 */
void sys_standby(void);                                                         /* 进入待机模式 */
void sys_soft_reset(void);                                                      /* 系统软复位 */
uint8_t sys_clock_set(uint32_t plln);                                           /* 时钟设置函数 */
void sys_stm32_clock_init(uint32_t plln);                                       /* 系统时钟初始化函数 */

/* 以下为汇编函数 */
void sys_wfi_set(void);                                                         /* 执行WFI指令 */
void sys_intx_disable(void);                                                    /* 关闭所有中断 */
void sys_intx_enable(void);                                                     /* 开启所有中断 */
void sys_msr_msp(uint32_t addr);                                                /* 设置栈顶地址 */

#endif
