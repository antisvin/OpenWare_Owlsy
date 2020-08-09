#include "stm32_arch_hal.h"
#include "sdram.h"

// TODO:
// - Consider alternative to libdaisy.h inclusion for board specific details.
// - Optimize Timing Variables, etc. for Maximum Speed.

// For now all configuration is done specifically for
//    the AS4C16M32MSA-6BIN 64MB SDRAM from Alliance Memory.

// Notes from the Datasheet
// tCK(3) - Clock Cycle Time (min.) 6ns
// tAC(3) - Access time from Clk (max.) 5.5ns
// tRAS   - Row Active time (min.) 48ns
// tRC    - Row Cycle time (min.) 60ns

// 166MHz = 6.024ns
// RAS = 8 ticks at 166
// RC = 10 ticks at 166

//#include "fmc.h"
#define SDRAM_MODEREG_BURST_LENGTH_2 ((1 << 0))
#define SDRAM_MODEREG_BURST_LENGTH_4 ((1 << 1))

#define SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL ((0 << 3))

#define SDRAM_MODEREG_CAS_LATENCY_3 ((1 << 4) | (1 << 5))

#define SDRAM_MODEREG_OPERATING_MODE_STANDARD ()

#define SDRAM_MODEREG_WRITEBURST_MODE_SINGLE ((1 << 9))
#define SDRAM_MODEREG_WRITEBURST_MODE_PROG_BURST ((0 << 9))


extern char _EXTRAM;

void MPU_Config(void){
  MPU_Region_InitTypeDef MPU_InitStruct;
  
  /* Disable the MPU */
  HAL_MPU_Disable();

  /* Configure the MPU attributes as WB for SDRAM */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.BaseAddress = (uint32_t)&_EXTRAM;
  MPU_InitStruct.Size = MPU_REGION_SIZE_64MB;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.SubRegionDisable = 0x00;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /* Enable the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

void SDRAM_Initialization_Sequence(SDRAM_HandleTypeDef *hsdram){
    __enable_irq(); // must enable SysTick IRQ for call to HAL_Delay()

    FMC_SDRAM_TimingTypeDef SdramTiming = {0};
    hsdram->Instance           = FMC_SDRAM_DEVICE;
    // Init
    hsdram->Init.SDBank             = FMC_SDRAM_BANK1;
    hsdram->Init.ColumnBitsNumber   = FMC_SDRAM_COLUMN_BITS_NUM_9;
    hsdram->Init.RowBitsNumber      = FMC_SDRAM_ROW_BITS_NUM_13;
    hsdram->Init.MemoryDataWidth    = FMC_SDRAM_MEM_BUS_WIDTH_32;
    hsdram->Init.InternalBankNumber = FMC_SDRAM_INTERN_BANKS_NUM_4;
    hsdram->Init.CASLatency         = FMC_SDRAM_CAS_LATENCY_3;
    hsdram->Init.WriteProtection = FMC_SDRAM_WRITE_PROTECTION_DISABLE;
    hsdram->Init.SDClockPeriod   = FMC_SDRAM_CLOCK_PERIOD_2;
    hsdram->Init.ReadBurst       = FMC_SDRAM_RBURST_ENABLE;
    hsdram->Init.ReadPipeDelay   = FMC_SDRAM_RPIPE_DELAY_0;
    /* SdramTiming */
    SdramTiming.LoadToActiveDelay    = 2;
    SdramTiming.ExitSelfRefreshDelay = 7;
    SdramTiming.SelfRefreshTime      = 4;
    SdramTiming.RowCycleDelay        = 8; // started at 7
    SdramTiming.WriteRecoveryTime    = 3;
    SdramTiming.RPDelay              = 0;
    SdramTiming.RCDDelay             = 10; // started at 2
    //  SdramTiming.LoadToActiveDelay = 16;
    //  SdramTiming.ExitSelfRefreshDelay = 16;
    //  SdramTiming.SelfRefreshTime = 16;
    //  SdramTiming.RowCycleDelay = 16;
    //  SdramTiming.WriteRecoveryTime = 16;
    //  SdramTiming.RPDelay = 16;
    //  SdramTiming.RCDDelay = 16;

    if(HAL_SDRAM_Init(hsdram, &SdramTiming) != HAL_OK)
    {
        //Error_Handler();
        error(MEM_ERROR, "SDRAM periph init error");
    }

    FMC_SDRAM_CommandTypeDef Command;

    __IO uint32_t tmpmrd = 0;
    /* Step 3:  Configure a clock configuration enable command */
    Command.CommandMode            = FMC_SDRAM_CMD_CLK_ENABLE;
    Command.CommandTarget          = FMC_SDRAM_CMD_TARGET_BANK1;
    Command.AutoRefreshNumber      = 1;
    Command.ModeRegisterDefinition = 0;

    /* Send the command */
    if (HAL_SDRAM_SendCommand(hsdram, &Command, 0x1000) != HAL_OK)
        error(MEM_ERROR, "SDRAM cmd1 error");

    /* Step 4: Insert 100 ms delay */
    HAL_Delay(100);


    /* Step 5: Configure a PALL (precharge all) command */
    Command.CommandMode            = FMC_SDRAM_CMD_PALL;
    Command.CommandTarget          = FMC_SDRAM_CMD_TARGET_BANK1;
    Command.AutoRefreshNumber      = 1;
    Command.ModeRegisterDefinition = 0;

    /* Send the command */
    if (HAL_SDRAM_SendCommand(hsdram, &Command, 0x1000) != HAL_OK)
        error(MEM_ERROR, "SDRAM cmd2 error");

    /* Step 6 : Configure a Auto-Refresh command */
    Command.CommandMode            = FMC_SDRAM_CMD_AUTOREFRESH_MODE;
    Command.CommandTarget          = FMC_SDRAM_CMD_TARGET_BANK1;
    Command.AutoRefreshNumber      = 4;
    Command.ModeRegisterDefinition = 0;

    /* Send the command */
    if (HAL_SDRAM_SendCommand(hsdram, &Command, 0x1000) != HAL_OK)
        error(MEM_ERROR, "SDRAM cmd3 error");

    /* Step 7: Program the external memory mode register */
    tmpmrd = (uint32_t)SDRAM_MODEREG_BURST_LENGTH_4
             | SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL | SDRAM_MODEREG_CAS_LATENCY_3
             | SDRAM_MODEREG_WRITEBURST_MODE_SINGLE;
    //SDRAM_MODEREG_OPERATING_MODE_STANDARD | // Used in example, but can't find reference except for "Test Mode"

    Command.CommandMode            = FMC_SDRAM_CMD_LOAD_MODE;
    Command.CommandTarget          = FMC_SDRAM_CMD_TARGET_BANK1;
    Command.AutoRefreshNumber      = 1;
    Command.ModeRegisterDefinition = tmpmrd;

    /* Send the command */
    if (HAL_SDRAM_SendCommand(hsdram, &Command, 0x1000) != HAL_OK)
        error(MEM_ERROR, "SDRAM cmd4 error");

    //HAL_SDRAM_ProgramRefreshRate(hsdram, 0x56A - 20);
    if (HAL_SDRAM_ProgramRefreshRate(hsdram, 0x81A - 20) != HAL_OK)
        error(MEM_ERROR, "SDRAM refresh rate error");
}

