#include "eepromcontrol.h"
#include <string.h> /* for memcpy */
#include "device.h"

extern void device_reset(void);

void eeprom_lock(){
  HAL_FLASH_Lock();
}

int eeprom_wait(){
#ifndef OWL_ARCH_H7
  return FLASH_WaitForLastOperation(5000);
#else
  return FLASH_WaitForLastOperation(5000, FLASH_BANK_1);
#endif
}

int eeprom_get_error() {
  return HAL_FLASH_GetError();
}

int eeprom_erase_sector(uint32_t sector) {
  FLASH_EraseInitTypeDef cfg;
  cfg.TypeErase = FLASH_TYPEERASE_SECTORS;
#ifndef OWL_ARCH_F7
  cfg.Banks = FLASH_BANK_1;
#endif
  cfg.Sector = sector;
  cfg.NbSectors = 1;
  cfg.VoltageRange = FLASH_VOLTAGE_RANGE_3;
  uint32_t error;
  HAL_StatusTypeDef status = HAL_FLASHEx_Erase(&cfg, &error);
  return status;
}

int eeprom_write_word(uint32_t address, uint32_t data){
#ifndef OWL_ARCH_H7
  HAL_StatusTypeDef status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, data);
#else
  HAL_StatusTypeDef status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, address, data);
#endif
  return status;
}

#ifndef OWL_ARCH_H7
int eeprom_write_byte(uint32_t address, uint8_t data){
  HAL_StatusTypeDef status =  HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, address, data);
  return status;
}
#endif

int eeprom_write_block(uint32_t address, void* data, uint32_t size){
  uint32_t* p32 = (uint32_t*)data;
  uint32_t i=0; 
  for(;i+4<=size; i+=4)
    eeprom_write_word(address+i, *p32++);
  uint8_t* p8 = (uint8_t*)p32;
#ifndef OWL_ARCH_H7
  for(;i<size; i++)
    eeprom_write_byte(address+i, *p8++);
#else
  // H7 requires flash alignment to words, so we align remaining bytes to the left
  if (size < i){
    eeprom_write_word(address + i, (*p32) << ((size - i) * 8));
  }
#endif
  return eeprom_wait() == HAL_FLASH_ERROR_NONE ? 0 : -1;
}

void eeprom_unlock(){
  HAL_FLASH_Unlock();
}

int eeprom_erase(uint32_t address){
  int ret = -1;
  if(address < ADDR_FLASH_SECTOR_1)
    ret = -1;  // protect boot sector
    /* eeprom_erase_sector(FLASH_SECTOR_0, VoltageRange_3); */
#ifdef DAISY
  // Naturally there are are H7 devices that have more than 1 sector. There also are
  // devices in other MCU families that have just 1 sector, but we don't support them. So
  // for now checking for Daisy should be good enough.
  else if(address == ADDR_FLASH_SECTOR_0)
    ret = eeprom_erase_sector(FLASH_SECTOR_0);
#else
  else if(address < ADDR_FLASH_SECTOR_2)
    ret = eeprom_erase_sector(FLASH_SECTOR_1);
  else if(address < ADDR_FLASH_SECTOR_3)
    ret = eeprom_erase_sector(FLASH_SECTOR_2);
  else if(address < ADDR_FLASH_SECTOR_4)
    ret = eeprom_erase_sector(FLASH_SECTOR_3);
  else if(address < ADDR_FLASH_SECTOR_5)
    ret = eeprom_erase_sector(FLASH_SECTOR_4);
  else if(address < ADDR_FLASH_SECTOR_6)
    ret = eeprom_erase_sector(FLASH_SECTOR_5);
  else if(address < ADDR_FLASH_SECTOR_7)
    ret = eeprom_erase_sector(FLASH_SECTOR_6);
  else if(address < ADDR_FLASH_SECTOR_8)
    ret = eeprom_erase_sector(FLASH_SECTOR_7);
#ifndef OWL_ARCH_F7
  else if(address < ADDR_FLASH_SECTOR_9)
    ret = eeprom_erase_sector(FLASH_SECTOR_8);
  else if(address < ADDR_FLASH_SECTOR_10)
    ret = eeprom_erase_sector(FLASH_SECTOR_9);
  else if(address < ADDR_FLASH_SECTOR_11)
    ret = eeprom_erase_sector(FLASH_SECTOR_10);
  else if(address < 0x08100000)
    ret = eeprom_erase_sector(FLASH_SECTOR_11);
#endif
#endif /* DAISY */
  else
    ret = -1;
  return ret;
}

int eeprom_write_unlock(uint32_t wrp_sectors){
  int ret = HAL_OK;
  FLASH_OBProgramInitTypeDef OptionBytes;
  /* Disable write protected pages */
  OptionBytes.OptionType = OPTIONBYTE_WRP;
  OptionBytes.WRPState = OB_WRPSTATE_DISABLE;
  OptionBytes.Banks = FLASH_BANK_1;
  OptionBytes.WRPSector = wrp_sectors;
  HAL_FLASH_OB_Unlock();
  ret = HAL_FLASHEx_OBProgram(&OptionBytes);
  /*
  * This would perform system reset here unless an error is returned
  */
  if (ret == HAL_OK) {
    ret |= HAL_FLASH_OB_Launch();
    device_reset();
  }
  HAL_FLASH_OB_Lock();
  return ret;
}

int eeprom_write_lock(uint32_t wrp_sectors){
  int ret = HAL_OK;
  FLASH_OBProgramInitTypeDef OptionBytes;
  /* Enable write protection */
  OptionBytes.OptionType = OPTIONBYTE_WRP;
  OptionBytes.WRPState = OB_WRPSTATE_ENABLE;
  OptionBytes.Banks = FLASH_BANK_1;
  OptionBytes.WRPSector = wrp_sectors;
  HAL_FLASH_OB_Unlock();
  ret = HAL_FLASHEx_OBProgram(&OptionBytes);
  /*
  * We perform system reset here unless an error is returned
  */
  if (ret == HAL_OK) {
    ret |= HAL_FLASH_OB_Launch();
    device_reset();
  }
  HAL_FLASH_OB_Lock();
    return ret;
}

/*
 * Checks which sectors have write protection option byte set
 */
uint32_t eeprom_write_protection(uint32_t wrp_sectors){
  FLASH_OBProgramInitTypeDef OptionBytes;
  HAL_FLASHEx_OBGetConfig(&OptionBytes);
  return ~OptionBytes.WRPSector & wrp_sectors;
}
