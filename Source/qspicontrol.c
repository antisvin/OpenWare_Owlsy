#include "stm32_arch_hal.h"
#include "device.h"
#include "qspicontrol.h"


// TODO: Add handling for alternate device types,
//		This will be a thing much sooner than anticipated
//		due to upgrading the RAM size for the new 4MB chip.
// TODO: autopolling_mem_ready only works for 1-Line, not 4-Line

extern QSPI_HandleTypeDef QSPI_HANDLE;

static uint32_t reset_memory(QSPI_HandleTypeDef *hqspi);
static uint32_t dummy_cycles_cfg(QSPI_HandleTypeDef *hqspi);
static uint32_t write_enable(QSPI_HandleTypeDef *hqspi);
static uint32_t quad_enable(QSPI_HandleTypeDef *hqspi);
static uint32_t enable_memory_mapped_mode(QSPI_HandleTypeDef *hqspi);
static uint32_t autopolling_mem_ready(QSPI_HandleTypeDef *hqspi,
                                      uint32_t timeout);

// These functions are defined, but we haven't added the ability to switch to
// quad mode. So they're currently unused.
static uint32_t enter_quad_mode(QSPI_HandleTypeDef *hqspi)
    __attribute__((unused));
static uint32_t exit_quad_mode(QSPI_HandleTypeDef *hqspi)
    __attribute__((unused));
static uint8_t get_status_register(QSPI_HandleTypeDef *hqspi)
    __attribute__((unused));

int qspi_init(QspiMode mode) {
  // Set Handle Settings
  if (HAL_QSPI_DeInit(&QSPI_HANDLE) != HAL_OK) {
    return MEMORY_ERROR;
  }
  // HAL_QSPI_MspInit(&qspi_handle);	 // I think this gets called a
  // in HAL_QSPI_Init(); Set Initialization values for the QSPI Peripheral
  uint32_t flash_size;
  flash_size = QSPI_FLASH_SIZE;
  QSPI_HANDLE.Instance = QUADSPI;
  // qspi_handle.Init.ClockPrescaler = 7;
  // qspi_handle.Init.ClockPrescaler = 7;
  // qspi_handle.Init.ClockPrescaler = 2; // Conservative setting for now.
  // Signal gets very weak faster than this.
  QSPI_HANDLE.Init.ClockPrescaler = 1;
  QSPI_HANDLE.Init.FifoThreshold = 1;
  QSPI_HANDLE.Init.SampleShifting = QSPI_SAMPLE_SHIFTING_NONE;
  QSPI_HANDLE.Init.FlashSize = POSITION_VAL(flash_size) - 1;
  QSPI_HANDLE.Init.ChipSelectHighTime = QSPI_CS_HIGH_TIME_2_CYCLE;
  QSPI_HANDLE.Init.FlashID = QSPI_FLASH_ID_1;
  QSPI_HANDLE.Init.DualFlash = QSPI_DUALFLASH_DISABLE;

  if (HAL_QSPI_Init(&QSPI_HANDLE) != HAL_OK) {
    return MEMORY_ERROR;
  }
  if (reset_memory(&QSPI_HANDLE) != MEMORY_OK) {
    return MEMORY_ERROR;
  }
  if (dummy_cycles_cfg(&QSPI_HANDLE) != MEMORY_OK) {
    return MEMORY_ERROR;
  }
  // Once writing test with 1 Line is confirmed lets move this out, and update
  // writing to use 4-line.
  if (quad_enable(&QSPI_HANDLE) != MEMORY_OK) {
    return MEMORY_ERROR;
  }
  if (mode == QSPI_MODE_MEMORY_MAPPED) {
    if (enable_memory_mapped_mode(&QSPI_HANDLE) != MEMORY_OK) {
      return MEMORY_ERROR;
    }
  }
  return MEMORY_OK;
}

int qspi_deinit() {
  QSPI_HANDLE.Instance = QUADSPI;
  if (HAL_QSPI_DeInit(&QSPI_HANDLE) != HAL_OK) {
    return MEMORY_ERROR;
  }
  HAL_QSPI_MspDeInit(&QSPI_HANDLE);
  return MEMORY_OK;
}

int qspi_write_page(uint32_t address, uint8_t *data, uint32_t size) {
  QSPI_CommandTypeDef s_command;
  s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
  s_command.Instruction = PAGE_PROG_CMD;
  s_command.AddressMode = QSPI_ADDRESS_1_LINE;
  s_command.AddressSize = QSPI_ADDRESS_24_BITS;
  s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  s_command.DataMode = QSPI_DATA_1_LINE;
  s_command.DummyCycles = 0;
  s_command.NbData = size <= 256 ? size : 256;
  s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
  s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
  s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;
  s_command.Address = address;
  if (write_enable(&QSPI_HANDLE) != MEMORY_OK) {
    return MEMORY_ERROR;
  }
  if (HAL_QSPI_Command(&QSPI_HANDLE, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) !=
      MEMORY_OK) {
    return MEMORY_ERROR;
  }
  if (HAL_QSPI_Transmit(&QSPI_HANDLE, (uint8_t *)data,
                        HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != MEMORY_OK) {
    return MEMORY_ERROR;
  }
  if (autopolling_mem_ready(&QSPI_HANDLE, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) !=
      MEMORY_OK) {
    return MEMORY_ERROR;
  }
  return MEMORY_OK;
}

int qspi_write_block(uint32_t address, uint8_t *data, uint32_t size) {
  uint32_t NumOfPage = 0, NumOfSingle = 0, Addr = 0, count = 0, temp = 0;
  uint32_t QSPI_DataNum = 0;
  uint32_t flash_page_size = QSPI_PAGE_SIZE;
  address = address & 0x0FFFFFFF;
  Addr = address % flash_page_size;
  count = flash_page_size - Addr;
  NumOfPage = size / flash_page_size;
  NumOfSingle = size % flash_page_size;

  if (Addr == 0) /*!< Address is QSPI_PAGESIZE aligned  */
  {
    if (NumOfPage == 0) /*!< NumByteToWrite < QSPI_PAGESIZE */
    {
      QSPI_DataNum = size;
      qspi_write_page(address, data, QSPI_DataNum);
    } else /*!< Size > QSPI_PAGESIZE */
    {
      while (NumOfPage--) {
        QSPI_DataNum = flash_page_size;
        qspi_write_page(address, data, QSPI_DataNum);
        address += flash_page_size;
        data += flash_page_size;
      }

      QSPI_DataNum = NumOfSingle;
      if (QSPI_DataNum > 0)
        qspi_write_page(address, data, QSPI_DataNum);
    }
  } else /*!< Address is not QSPI_PAGESIZE aligned  */
  {
    if (NumOfPage == 0) /*!< Size < QSPI_PAGESIZE */
    {
      if (NumOfSingle > count) /*!< (Size + Address) > QSPI_PAGESIZE */
      {
        temp = NumOfSingle - count;
        QSPI_DataNum = count;
        qspi_write_page(address, data, QSPI_DataNum);
        address += count;
        data += count;
        QSPI_DataNum = temp;
        qspi_write_page(address, data, QSPI_DataNum);
      } else {
        QSPI_DataNum = size;
        qspi_write_page(address, data, QSPI_DataNum);
      }
    } else /*!< Size > QSPI_PAGESIZE */
    {
      size -= count;
      NumOfPage = size / flash_page_size;
      NumOfSingle = size % flash_page_size;
      QSPI_DataNum = count;
      qspi_write_page(address, data, QSPI_DataNum);
      address += count;
      data += count;

      while (NumOfPage--) {
        QSPI_DataNum = flash_page_size;
        qspi_write_page(address, data, QSPI_DataNum);
        address += flash_page_size;
        data += flash_page_size;
      }

      if (NumOfSingle != 0) {
        QSPI_DataNum = NumOfSingle;
        qspi_write_page(address, data, QSPI_DataNum);
      }
    }
  }
  return MEMORY_OK;
}

int qspi_erase(uint32_t start_adr, uint32_t end_adr) {
  uint32_t erase_addr;
  uint32_t remaining_size;

  // Align start/end addresses to 4k page
  start_adr &= 0xFFFFF000;
  if (end_adr & 0xFFF) {
    end_adr &= 0xFFFFF000;
    end_adr += 0x1000;
  }

  erase_addr = start_adr & 0x0FFFFFFF;
  remaining_size = end_adr - start_adr;
  // Start with 64k blocks, then use 4k blocks if necessary
  while (remaining_size) {
    // Block erase
    if (remaining_size >= QSPI_BLOCK_SIZE) {
      if (qspi_erase_block(erase_addr) != MEMORY_OK) {
        return MEMORY_ERROR;
      }
      else {
        remaining_size -= QSPI_BLOCK_SIZE;
        erase_addr += QSPI_BLOCK_SIZE;
      }
    }
    // Sector erase
    else if (remaining_size) {
      if (qspi_erase_sector(erase_addr) != MEMORY_OK) {
        return MEMORY_ERROR;
      }
      else {
        remaining_size -= QSPI_SECTOR_SIZE;
        erase_addr += QSPI_SECTOR_SIZE;
      }
    }
  }
  return MEMORY_OK;
}

int qspi_erase_sector(uint32_t addr) {
  uint8_t use_qpi = 0;
  QSPI_CommandTypeDef s_command;
  if (use_qpi) {
    s_command.InstructionMode = QSPI_INSTRUCTION_4_LINES;
    s_command.Instruction = SECTOR_ERASE_QPI_CMD;
    s_command.AddressMode = QSPI_ADDRESS_4_LINES;
  } else {
    s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction = SECTOR_ERASE_CMD;
    s_command.AddressMode = QSPI_ADDRESS_1_LINE;
  }
  s_command.AddressSize = QSPI_ADDRESS_24_BITS;
  s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  s_command.DataMode = QSPI_DATA_NONE;
  s_command.DummyCycles = 0;
  s_command.NbData = 0;
  s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
  s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
  s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;
  s_command.Address = addr;
  if (write_enable(&QSPI_HANDLE) != MEMORY_OK) {
    return MEMORY_ERROR;
  }
  if (HAL_QSPI_Command(&QSPI_HANDLE, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) !=
      MEMORY_OK) {
    return MEMORY_ERROR;
  }
  if (autopolling_mem_ready(&QSPI_HANDLE, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) !=
      MEMORY_OK) {
    return MEMORY_ERROR;
  }
  return MEMORY_OK;
}

int qspi_erase_block(uint32_t addr) {
  QSPI_CommandTypeDef s_command;
  s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
  s_command.Instruction = BLOCK_ERASE_CMD;
  s_command.AddressMode = QSPI_ADDRESS_1_LINE;
  s_command.AddressSize = QSPI_ADDRESS_24_BITS;
  s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  s_command.DataMode = QSPI_DATA_NONE;
  s_command.DummyCycles = 0;
  s_command.NbData = 0;
  s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
  s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
  s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;
  s_command.Address = addr;
  if (write_enable(&QSPI_HANDLE) != MEMORY_OK) {
    return MEMORY_ERROR;
  }
  if (HAL_QSPI_Command(&QSPI_HANDLE, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) !=
      MEMORY_OK) {
    return MEMORY_ERROR;
  }
  if (autopolling_mem_ready(&QSPI_HANDLE, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) !=
      MEMORY_OK) {
    return MEMORY_ERROR;
  }
#ifdef USE_IWDG
  IWDG1->KR = 0xaaaa; // reset the watchdog timer
#endif
  return MEMORY_OK;
}

/* Static Function Implementation */
static uint32_t reset_memory(QSPI_HandleTypeDef *hqspi) {
  QSPI_CommandTypeDef s_command;

  /* Initialize the reset enable command */
  s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
  s_command.Instruction = RESET_ENABLE_CMD;
  s_command.AddressMode = QSPI_ADDRESS_NONE;
  s_command.DataMode = QSPI_DATA_NONE;
  s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  s_command.DummyCycles = 0;
  s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
  s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
  s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

  /* Send the command */
  if (HAL_QSPI_Command(hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) !=
      HAL_OK) {
    return MEMORY_ERROR;
  }

  /* Send the reset memory command */
  s_command.Instruction = RESET_MEMORY_CMD;
  if (HAL_QSPI_Command(hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) !=
      HAL_OK) {
    return MEMORY_ERROR;
  }

  /* Configure automatic polling mode to wait the memory is ready */
  if (autopolling_mem_ready(hqspi, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) !=
      MEMORY_OK) {
    return MEMORY_ERROR;
  }
  return MEMORY_OK;
}

static uint32_t dummy_cycles_cfg(QSPI_HandleTypeDef *hqspi) {
  QSPI_CommandTypeDef s_command;
  uint16_t reg = 0;
  s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
  s_command.AddressMode = QSPI_ADDRESS_NONE;
  s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  s_command.DataMode = QSPI_DATA_1_LINE;
  s_command.DummyCycles = 0;
  s_command.NbData = 1;
  s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
  s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
  s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;
#ifdef QSPI_DEVICE_IS25LP080D
  /* Initialize the read volatile configuration register command */
  s_command.Instruction = READ_READ_PARAM_REG_CMD;

  /* Configure the command */
  if (HAL_QSPI_Command(hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) !=
      HAL_OK) {
    return MEMORY_ERROR;
  }

  /* Reception of the data */
  if (HAL_QSPI_Receive(hqspi, (uint8_t *)(&reg),
                       HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return MEMORY_ERROR;
  }
  MODIFY_REG(reg, 0x78, (IS25LP080D_DUMMY_CYCLES_READ_QUAD << 3));
  /* Enable write operations */
  if (write_enable(hqspi) != MEMORY_OK) {
    return MEMORY_ERROR;
  }
#else
  // Only volatile Read Params on 16MB chip.
  // Explicitly set:
  // Burst Length: 8 bytes (0, 0)
  // Wrap Enable: 0
  // Dummy Cycles: (Config 3, bits 1 0)
  // Drive Strength (50%, bits 1 1 1)
  // Byte to write: 0b11110000 (0xF0)
  // TODO: Probably expand Burst to maximum if that works out.

  reg = 0xF0;
#endif

  /* Update volatile configuration register (with new dummy cycles) */
  s_command.Instruction = WRITE_READ_PARAM_REG_CMD;

  /* Configure the write volatile configuration register command */
  if (HAL_QSPI_Command(hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) !=
      HAL_OK) {
    return MEMORY_ERROR;
  }

  /* Transmission of the data */
  if (HAL_QSPI_Transmit(hqspi, (uint8_t *)(&reg),
                        HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return MEMORY_ERROR;
  }

  /* Configure automatic polling mode to wait the memory is ready */
  if (autopolling_mem_ready(hqspi, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) !=
      MEMORY_OK) {
    return MEMORY_ERROR;
  }
  return MEMORY_OK;
}
static uint32_t write_enable(QSPI_HandleTypeDef *hqspi) {
  QSPI_CommandTypeDef s_command;
  QSPI_AutoPollingTypeDef s_config;

  /* Enable write operations */
  s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
  s_command.Instruction = WRITE_ENABLE_CMD;
  s_command.AddressMode = QSPI_ADDRESS_NONE;
  s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  s_command.DataMode = QSPI_DATA_NONE;
  s_command.DummyCycles = 0;
  s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
  s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
  s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

  if (HAL_QSPI_Command(hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) !=
      HAL_OK) {
    return MEMORY_ERROR;
  }

  /* Configure automatic polling mode to wait for write enabling */
  //		s_config.Match           = IS25LP080D_SR_WREN |
  //(IS25LP080D_SR_WREN
  //<< 8); 		s_config.Mask            = IS25LP080D_SR_WREN |
  //(IS25LP080D_SR_WREN
  //<< 8);
  s_config.MatchMode = QSPI_MATCH_MODE_AND;
  s_config.Match = QSPI_SR_WREN;
  s_config.Mask = QSPI_SR_WREN;
  s_config.Interval = 0x10;
  s_config.StatusBytesSize = 1;
  s_config.AutomaticStop = QSPI_AUTOMATIC_STOP_ENABLE;

  s_command.Instruction = READ_STATUS_REG_CMD;
  s_command.DataMode = QSPI_DATA_1_LINE;

  if (HAL_QSPI_AutoPolling(hqspi, &s_command, &s_config,
                           HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return MEMORY_ERROR;
  }
  return MEMORY_OK;
}

static uint32_t quad_enable(QSPI_HandleTypeDef *hqspi) {
  QSPI_CommandTypeDef s_command;
  QSPI_AutoPollingTypeDef s_config;
  uint8_t reg = 0;

  /* Enable write operations */
  s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
  s_command.Instruction = WRITE_STATUS_REG_CMD;
  s_command.AddressMode = QSPI_ADDRESS_NONE;
  s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  s_command.DataMode = QSPI_DATA_1_LINE;
  s_command.DummyCycles = 0;
  s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
  s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
  s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

  /* Enable write operations */
  if (write_enable(hqspi) != MEMORY_OK) {
    return MEMORY_ERROR;
  }

  if (HAL_QSPI_Command(hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) !=
      HAL_OK) {
    return MEMORY_ERROR;
  }

  //	reg = 0;
  //	MODIFY_REG(reg,
  //		0xF0,
  //		(IS25LP08D_SR_QE));
  reg = QSPI_SR_QE; // Set QE bit  to 1
  /* Transmission of the data */
  if (HAL_QSPI_Transmit(hqspi, (uint8_t *)(&reg),
                        HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return MEMORY_ERROR;
  }

  /* Configure automatic polling mode to wait for write enabling */
  //	s_config.Match           = IS25LP08D_SR_WREN | (IS25LP08D_SR_WREN << 8);
  //	s_config.Mask            = IS25LP08D_SR_WREN | (IS25LP08D_SR_WREN << 8);
  //	s_config.MatchMode       = QSPI_MATCH_MODE_AND;
  //	s_config.StatusBytesSize = 2;
  s_config.Match = QSPI_SR_QE;
  s_config.Mask = QSPI_SR_QE;
  s_config.MatchMode = QSPI_MATCH_MODE_AND;
  s_config.StatusBytesSize = 1;

  s_config.Interval = 0x10;
  s_config.AutomaticStop = QSPI_AUTOMATIC_STOP_ENABLE;

  s_command.Instruction = READ_STATUS_REG_CMD;
  s_command.DataMode = QSPI_DATA_1_LINE;

  if (HAL_QSPI_AutoPolling(hqspi, &s_command, &s_config,
                           HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return MEMORY_ERROR;
  }

  /* Configure automatic polling mode to wait the memory is ready */
  if (autopolling_mem_ready(hqspi, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) !=
      MEMORY_OK) {
    return MEMORY_ERROR;
  }
  return MEMORY_OK;
}

static uint32_t enable_memory_mapped_mode(QSPI_HandleTypeDef *hqspi) {
  QSPI_CommandTypeDef s_command;
  QSPI_MemoryMappedTypeDef s_mem_mapped_cfg;

  /* Configure the command for the read instruction */
  s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
  s_command.Instruction = QUAD_INOUT_FAST_READ_CMD;
  s_command.AddressMode = QSPI_ADDRESS_4_LINES;
  s_command.AddressSize = QSPI_ADDRESS_24_BITS;
  //	s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  // s_command.DummyCycles       = IS25LP080D_DUMMY_CYCLES_READ_QUAD;
  s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_4_LINES;
  s_command.AlternateBytesSize = QSPI_ALTERNATE_BYTES_8_BITS;
  s_command.AlternateBytes = 0x000000A0;
  s_command.DummyCycles = 6;
  s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
  s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
  // s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;
  s_command.SIOOMode = QSPI_SIOO_INST_ONLY_FIRST_CMD;
  s_command.DataMode = QSPI_DATA_4_LINES;

  /* Configure the memory mapped mode */
  s_mem_mapped_cfg.TimeOutActivation = QSPI_TIMEOUT_COUNTER_DISABLE;
  s_mem_mapped_cfg.TimeOutPeriod = 0;

  if (HAL_QSPI_MemoryMapped(hqspi, &s_command, &s_mem_mapped_cfg) != HAL_OK) {
    return MEMORY_ERROR;
  }
  return MEMORY_OK;
}
static uint32_t autopolling_mem_ready(QSPI_HandleTypeDef *hqspi,
                                      uint32_t timeout) {
  QSPI_CommandTypeDef s_command;
  QSPI_AutoPollingTypeDef s_config;

  /* Configure automatic polling mode to wait for memory ready */
  s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
  s_command.Instruction = READ_STATUS_REG_CMD;
  s_command.AddressMode = QSPI_ADDRESS_NONE;
  s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  s_command.DataMode = QSPI_DATA_1_LINE;
  s_command.DummyCycles = 0;
  s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
  s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
  s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

  s_config.Match = 0;
  s_config.MatchMode = QSPI_MATCH_MODE_AND;
  s_config.Interval = 0x10;
  s_config.AutomaticStop = QSPI_AUTOMATIC_STOP_ENABLE;
  s_config.Mask = QSPI_SR_WIP;
  // s_config.Mask            = 0;
  s_config.StatusBytesSize = 1;

  if (HAL_QSPI_AutoPolling(hqspi, &s_command, &s_config, timeout) != HAL_OK) {
    return MEMORY_ERROR;
  }
  return MEMORY_OK;
}

static uint32_t enter_quad_mode(QSPI_HandleTypeDef *hqspi) {
  QSPI_CommandTypeDef s_command;

  /* Initialize the read volatile configuration register command */
  s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
  s_command.Instruction = ENTER_QUAD_CMD;
  s_command.AddressMode = QSPI_ADDRESS_NONE;
  s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  s_command.DataMode = QSPI_DATA_NONE;
  s_command.DummyCycles = 0;
  s_command.NbData = 0;
  s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
  s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
  s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

  /* Configure the command */
  if (HAL_QSPI_Command(hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) !=
      MEMORY_OK) {
    return MEMORY_ERROR;
  }
  //	/* Wait for WIP bit in SR */
  //	if (QSPI_AutoPollingMemReady(hqspi, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) !=
  // MEMORY_OK)
  //	{
  //		return MEMORY_ERROR;
  //	}
  return MEMORY_OK;
}
static uint32_t exit_quad_mode(QSPI_HandleTypeDef *hqspi) {
  QSPI_CommandTypeDef s_command;

  /* Initialize the read volatile configuration register command */
  s_command.InstructionMode = QSPI_INSTRUCTION_4_LINES;
  s_command.Instruction = EXIT_QUAD_CMD;
  s_command.AddressMode = QSPI_ADDRESS_NONE;
  s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  s_command.DataMode = QSPI_DATA_NONE;
  s_command.DummyCycles = 0;
  s_command.NbData = 0;
  s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
  s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
  s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

  /* Configure the command */
  if (HAL_QSPI_Command(hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) !=
      MEMORY_OK) {
    return MEMORY_ERROR;
  }
  /* Wait for WIP bit in SR */
  if (autopolling_mem_ready(hqspi, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) !=
      MEMORY_OK) {
    return MEMORY_ERROR;
  }
  return MEMORY_OK;
}

static uint8_t get_status_register(QSPI_HandleTypeDef *hqspi) {
  QSPI_CommandTypeDef s_command;
  uint8_t reg;
  reg = 0x00;
  s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
  s_command.Instruction = READ_STATUS_REG_CMD;
  s_command.AddressMode = QSPI_ADDRESS_NONE;
  s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  s_command.DataMode = QSPI_DATA_1_LINE;
  s_command.DummyCycles = 0;
  s_command.NbData = 1;
  s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
  s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
  s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

  if (HAL_QSPI_Command(hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) !=
      HAL_OK) {
    return 0x00;
  }
  if (HAL_QSPI_Receive(hqspi, (uint8_t *)(&reg),
                       HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return 0x00;
  }
  return reg;
}
