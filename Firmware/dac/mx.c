/*
 * mx.c - all the basic system init code from CubeMX2
 * 04-09-26 E. Brombaugh
 */
#include "mx.h"

/*
 * init the MPU
 */
system_status_t mx_cortex_mpu_init(void)
{
  /* Disables the MPU */
  HAL_CORTEX_MPU_Disable();

  /*
     Initializes and configures the MPU attributes
  */
  HAL_CORTEX_MPU_SetCacheMemAttr(HAL_CORTEX_MPU_MEM_ATTR_0, HAL_CORTEX_MPU_NORMAL_MEM_NCACHEABLE);

  /*
     Initializes and configures the MPU Region
  */
  hal_cortex_mpu_region_config_t p_region_config = {0};

  p_region_config.base_addr = 0x8FFE000;
  p_region_config.limit_addr = 0x8FFFFFF;
  p_region_config.access_attr = HAL_CORTEX_MPU_REGION_ALL_RO;
  p_region_config.exec_attr = HAL_CORTEX_MPU_EXECUTION_ATTR_DISABLE;
  p_region_config.attr_idx = HAL_CORTEX_MPU_MEM_ATTR_0;
  HAL_CORTEX_MPU_SetConfigRegion(HAL_CORTEX_MPU_REGION_0, &p_region_config);

  HAL_CORTEX_MPU_EnableRegion(HAL_CORTEX_MPU_REGION_0);

  /* Enables the MPU */
  HAL_CORTEX_MPU_Enable(HAL_CORTEX_MPU_HARDFAULT_NMI_DISABLE, HAL_CORTEX_MPU_ACCESS_FAULT_ONLY_PRIV);

  return SYSTEM_OK;
}

/******************************************************************************/
/* Exported functions for NVIC in HAL layer (SW instance MyCORTEX_NVIC_1) */
/******************************************************************************/
system_status_t mx_cortex_nvic_init(void)
{
  /* Enable DebugMonitor exception */
  STM32_SET_BIT(DCB->DEMCR, DCB_DEMCR_MON_EN_Msk);

  /* Configure the Priority grouping */
  HAL_CORTEX_NVIC_SetPriorityGrouping(HAL_CORTEX_NVIC_PRIORITY_GROUP_4);

  /* Debug Monitor */
  HAL_CORTEX_NVIC_SetPriority(DebugMonitor_IRQn, HAL_CORTEX_NVIC_PREEMP_PRIORITY_0, HAL_CORTEX_NVIC_SUB_PRIORITY_0);

  /* Pendable request for system service */
  HAL_CORTEX_NVIC_SetPriority(PendSV_IRQn, HAL_CORTEX_NVIC_PREEMP_PRIORITY_0, HAL_CORTEX_NVIC_SUB_PRIORITY_0);

  return SYSTEM_OK;
}

/******************************************************************************/
/* Exported functions for ICACHE in HAL layer (SW instance MyICACHE_1) */
/******************************************************************************/
static hal_icache_handle_t hICACHE;

hal_icache_handle_t *mx_icache_init(void)
{
  if (HAL_ICACHE_Init(&hICACHE, HAL_ICACHE) != HAL_OK)
  {
    return NULL;
  }

  /* Associativity mode set to default value 2-ways */

  return &hICACHE;
}

hal_icache_handle_t *mx_icache_gethandle(void)
{
  return &hICACHE;
}

/******************************************************************************/
/* Exported functions for RCC in HAL layer */
/******************************************************************************/

#if 1
/**
  * Configure the system core clock only and activate it using the HAL RCC unitary APIs (footprint optimization)
  *         The system Clock is configured as follow :
  *            System Clock source            = PSIS
  *            SYSCLK(Hz)                     = 144000000
  *            HCLK(Hz)                       = 144000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 1
  *            APB2 Prescaler                 = 1
  *            APB3 Prescaler                 = 1
  *            Flash Latency(WS)              = 4
  */
system_status_t mx_rcc_init(void)
{
  if (HAL_RCC_HSE_Enable(HAL_RCC_HSE_ON) != HAL_OK)
  {
    return SYSTEM_CLOCK_ERROR;
  }

  hal_rcc_psi_config_t config_psi;
  config_psi.psi_source = HAL_RCC_PSI_SRC_HSE;
  config_psi.psi_ref = HAL_RCC_PSI_REF_24MHZ;
  config_psi.psi_out = HAL_RCC_PSI_OUT_144MHZ;
  if (HAL_RCC_PSI_SetConfig(&config_psi) != HAL_OK)
  {
    return SYSTEM_CLOCK_ERROR;
  }

  if (HAL_RCC_PSIS_Enable() != HAL_OK)
  {
    return SYSTEM_CLOCK_ERROR;
  }

  /** Initializes the CPU, AHB and APB busses clocks */
  hal_rcc_bus_clk_config_t config_bus;
  config_bus.hclk_prescaler  = HAL_RCC_HCLK_PRESCALER1;
  config_bus.pclk1_prescaler = HAL_RCC_PCLK_PRESCALER1;
  config_bus.pclk2_prescaler = HAL_RCC_PCLK_PRESCALER1;
  config_bus.pclk3_prescaler = HAL_RCC_PCLK_PRESCALER1;
  if (HAL_RCC_SetBusClockConfig(&config_bus) != HAL_OK)
  {
    return SYSTEM_CLOCK_ERROR;
  }

  /** Frequency will be increased */
  HAL_FLASH_ITF_SetLatency(HAL_FLASH, HAL_FLASH_ITF_LATENCY_4);

  if (HAL_RCC_SetSYSCLKSource(HAL_RCC_SYSCLK_SRC_PSIS) != HAL_OK)
  {
    return SYSTEM_CLOCK_ERROR;
  }

  HAL_FLASH_ITF_SetProgrammingDelay(HAL_FLASH, HAL_FLASH_ITF_PROGRAM_DELAY_2);

  if (HAL_UpdateCoreClock() != HAL_OK)
  {
    return SYSTEM_CLOCK_ERROR;
  }

  /* No GPIO configuration required for RCC */

  if (HAL_RCC_PSI_EnableInStopMode() != HAL_OK)
  {
    return SYSTEM_CLOCK_ERROR;
  }

  return SYSTEM_OK;
}
#else
/**
  * Configure the system core clock only and activate it using the HAL RCC unitary APIs (footprint optimization)
  *         The system Clock is configured as follow :
  *            System Clock source            = HSIS
  *            SYSCLK(Hz)                     = 144000000
  *            HCLK(Hz)                       = 144000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 1
  *            APB2 Prescaler                 = 1
  *            APB3 Prescaler                 = 1
  *            Flash Latency(WS)              = 4
  */
system_status_t mx_rcc_init(void)
{
  if (HAL_RCC_HSIS_Enable() != HAL_OK)
  {
    return SYSTEM_CLOCK_ERROR;
  }

  /** Initializes the CPU, AHB and APB busses clocks */
  hal_rcc_bus_clk_config_t config_bus;
  config_bus.hclk_prescaler  = HAL_RCC_HCLK_PRESCALER1;
  config_bus.pclk1_prescaler = HAL_RCC_PCLK_PRESCALER1;
  config_bus.pclk2_prescaler = HAL_RCC_PCLK_PRESCALER1;
  config_bus.pclk3_prescaler = HAL_RCC_PCLK_PRESCALER1;
  if (HAL_RCC_SetBusClockConfig(&config_bus) != HAL_OK)
  {
    return SYSTEM_CLOCK_ERROR;
  }

  /** Frequency will be increased */
  HAL_FLASH_ITF_SetLatency(HAL_FLASH, HAL_FLASH_ITF_LATENCY_4);

  if (HAL_RCC_SetSYSCLKSource(HAL_RCC_SYSCLK_SRC_HSIS) != HAL_OK)
  {
    return SYSTEM_CLOCK_ERROR;
  }

  HAL_FLASH_ITF_SetProgrammingDelay(HAL_FLASH, HAL_FLASH_ITF_PROGRAM_DELAY_2);

  if (HAL_UpdateCoreClock() != HAL_OK)
  {
    return SYSTEM_CLOCK_ERROR;
  }

  /* No GPIO configuration required for RCC */

  if (HAL_RCC_PSI_EnableInStopMode() != HAL_OK)
  {
    return SYSTEM_CLOCK_ERROR;
  }

  return SYSTEM_OK;
}
#endif

/**
  * configures and activate the clocks used by all the peripherals selected within the project
  */
#if 0
system_status_t mx_rcc_peripherals_clock_config(void)
{
  /* Peripherals using PCLK1 (144 MHz):
    I2C1
    SPI2
  */

  /* Peripherals using PCLK2 (144 MHz):
    USART1
    SPI1
  */

  /* Peripherals using HSIDIV3 (48 MHz):
    USB
  */
  if (HAL_RCC_HSIDIV3_Enable() != HAL_OK)
  {
    return SYSTEM_CLOCK_ERROR;
  }

  /* Peripherals using PSIS (144 MHz):
    ADC1
  */
  /* PSIS already enabled inside mx_rcc_init() */

  /* Peripherals using ADC_DAC_DIV (36 MHz):
    ADC1
  */
  if (HAL_RCC_ADCDAC_SetKernelClkPrescaler(HAL_RCC_ADCDAC_PRESCALER4) != HAL_OK)
  {
    return SYSTEM_CLOCK_ERROR;
  }

  return SYSTEM_OK;
}
#else
system_status_t mx_rcc_peripherals_clock_config(void)
{
  /* Peripherals using PCLK1 (144 MHz):
    I2C1
    SPI2
  */

  /* Peripherals using PCLK2 (144 MHz):
    USART1
    SPI1
  */

  /* Peripherals using HSIDIV3 (48 MHz):
    USB
  */
  if (HAL_RCC_HSIDIV3_Enable() != HAL_OK)
  {
    return SYSTEM_CLOCK_ERROR;
  }

  /* Peripherals using HSIK (144 MHz):
    ADC1
  */
  if (HAL_RCC_HSIK_Enable(HAL_RCC_HSIK_DIV1) != HAL_OK)
  {
    return SYSTEM_CLOCK_ERROR;
  }

  /* Peripherals using ADC_DAC_DIV (36 MHz):
    ADC1
  */
  if (HAL_RCC_ADCDAC_SetKernelClkPrescaler(HAL_RCC_ADCDAC_PRESCALER4) != HAL_OK)
  {
    return SYSTEM_CLOCK_ERROR;
  }

  return SYSTEM_OK;
}
#endif

/*
 * main entry point for this operation
 */
system_status_t mx_system_init(void)
{
  /*
    CORTEX MPU initialization in case of isolation is not activated
  */
  if (mx_cortex_mpu_init() != SYSTEM_OK)
  {
    return SYSTEM_RESOURCES_ISOLATION_ERROR;
  }

  /*
    startup system section
  */
  if (HAL_Init() != HAL_OK)
  {
    return SYSTEM_STARTUP_ERROR;
  }

  /*
    Interruptions section
  */
  if (mx_cortex_nvic_init() != SYSTEM_OK)
  {
    return SYSTEM_INTERRUPTS_ERROR;
  }

  /*
    ICACHE section
  */
  if (mx_icache_init() == NULL)
  {
    return SYSTEM_STARTUP_ERROR;
  }

  /* ICACHE automatically started at startup */
  if (HAL_ICACHE_Start(mx_icache_gethandle(), HAL_ICACHE_IT_NONE) != HAL_OK)
  {
    return SYSTEM_STARTUP_ERROR;
  }

  /*
    Clock system section
  */
  if (mx_rcc_init() != SYSTEM_OK)
  {
    return SYSTEM_CLOCK_ERROR;
  }

  /* peripherals clock configuration and activation
    is centralized: no clock activation/deactivation in pppi_init */

  if (mx_rcc_peripherals_clock_config() != SYSTEM_OK)
  {
    return SYSTEM_CLOCK_ERROR;
  }
  
  return SYSTEM_OK;
}


/******************************************************************************/
/*                            Systick Handler                                 */
/******************************************************************************/
/**
  * @brief   This function handles SysTick Handler
  * @warning Systick_Handler is generated as NVIC peripheral has not been activated
  *          By default it is generated in HAL.
  *          It must be located into mx_cortex_nvic.c file
  */
void SysTick_Handler(void)
{
  HAL_IncTick();
  HAL_CORTEX_SYSTICK_IRQHandler();
}

