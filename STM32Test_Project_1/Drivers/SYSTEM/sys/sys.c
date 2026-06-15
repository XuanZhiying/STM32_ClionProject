/**
 ****************************************************************************************************
 * @file        sys.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.1
 * @date        2023-08-01
 * @brief       系统初始化代码(含时钟配置/中断管理/GPIO设置等)
 *              [已添加 HSI 降级保护 — 无外部晶振时自动切内部时钟]
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 */

#include "./SYSTEM/sys/sys.h"


/**
 * @brief       设置中断向量表偏移地址
 * @param       baseaddr: 基地址
 * @param       offset: 偏移量(一般为0, 或 0X100的倍数)
 * @retval      无
 */
void sys_nvic_set_vector_table(uint32_t baseaddr, uint32_t offset)
{
    SCB->VTOR = baseaddr | (offset & (uint32_t)0xFFFFFE00);
}

/**
 * @brief       执行 WFI 指令
 */
void sys_wfi_set(void)
{
    __ASM volatile("wfi");
}

/**
 * @brief       关闭所有中断(但不包括fault和NMI中断)
 */
void sys_intx_disable(void)
{
    __ASM volatile("cpsid i");
}

/**
 * @brief       开启所有中断
 */
void sys_intx_enable(void)
{
    __ASM volatile("cpsie i");
}

/**
 * @brief       设置栈顶地址
 */
void sys_msr_msp(uint32_t addr)
{
    __set_MSP(addr);
}

/**
 * @brief       进入待机模式
 */
void sys_standby(void)
{
    __HAL_RCC_PWR_CLK_ENABLE();
    SET_BIT(PWR->CR, PWR_CR_PDDS);
}

/**
 * @brief       系统软复位
 */
void sys_soft_reset(void)
{
    NVIC_SystemReset();
}

/**
 * @brief       系统时钟初始化函数
 * @param       plln: PLL倍频系数, 取值范围: 2~16
 * @note        优先使用 HSE (外部8MHz晶振), 失败自动降级 HSI (内部8MHz)
 *              HSE: 8MHz × plln → 默认72MHz (plln=9)
 *              HSI: 4MHz × 9    → 36MHz (降级)
 */
void sys_stm32_clock_init(uint32_t plln)
{
    HAL_StatusTypeDef ret = HAL_ERROR;
    RCC_OscInitTypeDef rcc_osc_init = {0};
    RCC_ClkInitTypeDef rcc_clk_init = {0};

    rcc_osc_init.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    rcc_osc_init.HSEState = RCC_HSE_ON;
    rcc_osc_init.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    rcc_osc_init.PLL.PLLState = RCC_PLL_ON;
    rcc_osc_init.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    rcc_osc_init.PLL.PLLMUL = plln;
    ret = HAL_RCC_OscConfig(&rcc_osc_init);

    if (ret != HAL_OK)
    {
        /* HSE 启动失败(无外部晶振或晶振损坏), 降级为 HSI */
        rcc_osc_init.OscillatorType = RCC_OSCILLATORTYPE_HSI;
        rcc_osc_init.HSIState = RCC_HSI_ON;
        rcc_osc_init.HSEState = RCC_HSE_OFF;
        rcc_osc_init.PLL.PLLState = RCC_PLL_ON;
        rcc_osc_init.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;   /* HSI/2 = 4MHz */
        rcc_osc_init.PLL.PLLMUL = RCC_PLL_MUL9;                /* 4MHz × 9 = 36MHz */
        ret = HAL_RCC_OscConfig(&rcc_osc_init);
        if (ret != HAL_OK) while (1);                           /* HSI 也失败则死循环 */
    }

    rcc_clk_init.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
    rcc_clk_init.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    rcc_clk_init.AHBCLKDivider = RCC_SYSCLK_DIV1;
    rcc_clk_init.APB1CLKDivider = RCC_HCLK_DIV2;
    rcc_clk_init.APB2CLKDivider = RCC_HCLK_DIV1;
    ret = HAL_RCC_ClockConfig(&rcc_clk_init, FLASH_LATENCY_2);

    if (ret != HAL_OK)
    {
        while (1);
    }
}
