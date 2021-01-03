#include "eepromcontrol.h"
#include <string.h> /* for memcpy */
#include "device.h"

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

int eeprom_erase_sector(uint32_t sector) {
  FLASH_EraseInitTypeDef cfg;
#ifndef OWL_ARCH_L4
  cfg.TypeErase = FLASH_TYPEERASE_SECTORS;
  cfg.Sector = sector;
  cfg.NbSectors = 1;
  cfg.VoltageRange = FLASH_VOLTAGE_RANGE_3;
#else
  cfg.TypeErase = FLASH_TYPEERASE_PAGES;
  cfg.Page = sector;
  cfg.NbPages = 1;
#endif  
#ifndef OWL_ARCH_F7
  cfg.Banks = FLASH_BANK_1;
#endif
  uint32_t error;
  HAL_StatusTypeDef status = HAL_FLASHEx_Erase(&cfg, &error);
  return status;
}

int eeprom_write_word(uint32_t address, uint32_t data){
#ifdef OWL_ARCH_H7
  HAL_StatusTypeDef status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, address, data);
#elif defined(OWL_ARCH_L4)
  HAL_StatusTypeDef status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, address, data);
#else
  HAL_StatusTypeDef status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, data);
#endif
  return status;
}

#if !defined(OWL_ARCH_H7) && !defined(OWL_ARCH_L4)
int eeprom_write_byte(uint32_t address, uint8_t data){
  HAL_StatusTypeDef status =  HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, address, data);
  return status;
}
#endif

int eeprom_write_block(uint32_t address, void* data, uint32_t size){
  uint32_t* p32 = (uint32_t*)data;
  uint32_t i=0; 
#if defined(OWL_ARCH_H7)
  for(;i+16<=size; i+=16){
    eeprom_write_word(address+i, *p32);
    *p32 += 4;
  }
#elif defined(OWL_ARCH_L4)
  for(;i+8<=size; i+=8) {
    eeprom_write_word(address+i, *p32);
    *p32+= 2;
  }
#else
  for(;i+4<=size; i+=4)
    eeprom_write_word(address+i, *p32++);
#endif
  uint8_t* p8 = (uint8_t*)p32;
#if !defined(OWL_ARCH_H7) && !defined(OWL_ARCH_L4)
  for(;i<size; i++)
    eeprom_write_byte(address+i, *p8++);
#else
  // TODO: do something with unaligned remainder?
#endif
  return eeprom_wait() == HAL_FLASH_ERROR_NONE ? 0 : -1;
}

void eeprom_unlock(){
  HAL_FLASH_Unlock();
}

#ifndef OWL_ARCH_L4
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
#else
int eeprom_erase(uint32_t address){
  int ret;
  extern char _FLASH_STORAGE_BEGIN, _FLASH_STORAGE_END;
  if(address < (uint32_t)&_FLASH_STORAGE_BEGIN || address >= (uint32_t)&_FLASH_STORAGE_END)
    ret = -1;  // protect boot sector
    /* eeprom_erase_sector(FLASH_SECTOR_0, VoltageRange_3); */
  else
    /* This assumes 8kb pages, note that dual flash bank would use 4kb */
    ret = eeprom_erase_sector((address >> 13) & 0xFF);
  return ret;
}
#endif
