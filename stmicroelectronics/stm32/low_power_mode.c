/**
 * @file:   low_power_mode.c
 * @author: trieunvt
 * @brief:  Low Power Mode (LPM) module driver.
 *          USAGE
 *          1. Call LPM_InitIWDG() to config the Independent Watchdog Timer (IWDG).
 *          2. Call LPM_Enter() wherever to acess Low Power Mode.
 **/

#include "main.h"
#include "stdint.h"

#define LPM_IWDG_FREEZE 0x00
#define LPM_IWDG_RUN    0x01

void LPM_InitIWDG(uint16_t option) {
    /* Get the old OB configuration to reconfigure */
    FLASH_OBProgramInitTypeDef pOBConfig = {0};
    HAL_FLASHEx_OBGetConfig(&pOBConfig);

    switch (option) {
    case LPM_IWDG_FREEZE:
        if ((pOBConfig.USERConfig & OB_IWDG_STOP_RUN) != 0) {
            /* Reconfigure the OB */
            pOBConfig.OptionType = OPTIONBYTE_USER;
            pOBConfig.USERType = OB_USER_IWDG_STOP;
            pOBConfig.USERConfig = OB_IWDG_STOP_FREEZE;
        }
        break;

    case LPM_IWDG_RUN:
        if ((pOBConfig.USERConfig & OB_IWDG_STOP_RUN) == 0) {
            /* Reconfigure the OB */
            pOBConfig.OptionType = OPTIONBYTE_USER;
            pOBConfig.USERType = OB_USER_IWDG_STOP;
            pOBConfig.USERConfig = OB_IWDG_STOP_RUN;
        }
        break;

    default:
        return;
    }

    /* Unclock the flash and OB to reconfigure */
    HAL_FLASH_Unlock();
    // __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_OPTVERR);
    HAL_FLASH_OB_Unlock();

    /* Save the new OB configuration to the flash */
    HAL_StatusTypeDef status = HAL_OK;
    do {
        status = HAL_FLASHEx_OBProgram(&pOBConfig);
    } while (status != HAL_OK);

    /* Launch the new OB configuration loading */
    HAL_FLASH_OB_Launch();

    /* Lock the flash and OB after reconfiguration */
    HAL_FLASH_OB_Lock();
    HAL_FLASH_Lock();

    HAL_NVIC_SystemReset();
}

void LPM_ReconfigSystemClock(void) {
    /* Configure the LSE drive capability */
    HAL_PWR_EnableBkUpAccess();
    __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

    /* Initialize RCC oscillators according to
    specified parameters in the RCC_OscInitTypeDef structure */
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSE | RCC_OSCILLATORTYPE_MSI;
    RCC_OscInitStruct.LSEState = RCC_LSE_ON;
    RCC_OscInitStruct.MSIState = RCC_MSI_ON;
    RCC_OscInitStruct.MSICalibrationValue = 0;
    RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_0;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    /* Initialize CPU, AHB and APB buses clocks */
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) {
        Error_Handler();
    }

    /* Configure the main internal regulator output voltage */
    if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK) {
        Error_Handler();
    }

    /* Enable the MSI auto calibration */
    HAL_RCCEx_EnableMSIPLLMode();
}

void LPM_Enter(void) {
    osDelay(100);

    /* Disable the register clock of all GPIOs */
    __HAL_RCC_GPIOA_CLK_DISABLE();
    __HAL_RCC_GPIOB_CLK_DISABLE();
    __HAL_RCC_GPIOC_CLK_DISABLE();
    __HAL_RCC_GPIOH_CLK_DISABLE();

    /* Reconfigure the System Clock to 100KHz */
    LPM_ReconfigSystemClock();

    /* Enter Low Power Mode (LPM) */
    HAL_PWREx_EnableLowPowerRunMode();

    /* Wait until the system and the regulator are in Low Power Mode */
    while (__HAL_PWR_GET_FLAG(PWR_FLAG_REGLPF) == RESET) {
        osDelay(1);
    }

    /* Suspend tick increments of all timers to prevent wakeup by tick interrupts */
    /* Otherwise tick interrupts will wake up the device
    within HAL time base period or FreeRTOS time base period */
    /* Disable tick interrupts used by HAL */
    HAL_SuspendTick();
    /* Disable tick interrupts used by FreeRTOS */
    CLEAR_BIT(SysTick->CTRL, SysTick_CTRL_TICKINT_Msk);

    /* Enter STOP2 Mode */
    /* Wakeup is done
    once there is an interrupt (internal or external) in any EXTI line */
    HAL_PWREx_EnterSTOP2Mode(PWR_STOPENTRY_WFI);

    /* Resume tick interrupts of all timers if disabled prior to the sleep mode entry */
    /* Enable tick interrupt used by FreeRTOS */
    SET_BIT(SysTick->CTRL, SysTick_CTRL_TICKINT_Msk);
    /* Enable tick interrupt used by HAL */
    HAL_ResumeTick();

    /* Exit Low Power Mode (LPM) */
    HAL_PWREx_DisableLowPowerRunMode();

    /* Enable register clocks of all GPIOs */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();

    /* Give the System Clock back to its original configuration */
    SystemClock_Config();

    osDelay(100);
}
