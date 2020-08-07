#include "FlashStorage.h"
#include "device.h"
#include "eepromcontrol.h"
#include "message.h"
#include <string.h>

#ifndef DAISY
// Most devices need a single flash storage
extern char _FLASH_STORAGE_BEGIN;
extern char _FLASH_STORAGE_END;
#define EEPROM_PAGE_BEGIN ((uint32_t)&_FLASH_STORAGE_BEGIN)
#define EEPROM_PAGE_END ((uint32_t)&_FLASH_STORAGE_END)
#else
// Daisy uses separate storage - R/W settings storage and readonly patch storage
extern char _SETTINGS_STORAGE_BEGIN;
extern char _SETTINGS_STORAGE_END;
#define SETTINGS_EEPROM_PAGE_BEGIN ((uint32_t)&_SETTINGS_STORAGE_BEGIN)
#define SETTINGS_EEPROM_PAGE_END ((uint32_t)&_SETTINGS_STORAGE_END)
extern char _PATCH_STORAGE_BEGIN;
extern char _PATCH_STORAGE_END;
#define PATCH_EEPROM_PAGE_BEGIN ((uint32_t)&_PATCH_STORAGE_BEGIN)
#define PATCH_EEPROM_PAGE_END ((uint32_t)&_PATCH_STORAGE_END)
#endif

#define EEPROM_PAGE_SIZE (128 * 1024)


uint32_t FlashStorage::getTotalAllocatedSize() {
  return end_page - start_page;
}


void FlashStorage::init() {
  uint32_t offset = 0;
  StorageBlock block;
  count = 0;
  do {
    block = createBlock(start_page, offset);
    blocks[count++] = block;
    offset += block.getBlockSize();
  } while (!block.isFree() && count + 1 < max_blocks &&
           start_page + offset < end_page);
  // fills at least one (possibly empty) block into list
}

#if 0
template<uint32_t start_page, uint32_t end_page, uint32_t max_blocks>
void FlashStorage<start_page, end_page, max_blocks>::recover(){
  StorageBlock block = getLastBlock();
  if(!block.isFree() && !block.isValidSize()){
    // count backwards to see how much free space (0xff words) there is
    uint32_t* top = (uint32_t*)block.getData();
    uint32_t* ptr = (uint32_t*)(end_page);
    uint32_t free = 0;
    while(--ptr > top && *ptr == 0xffffffff)
      free += 4;
    uint32_t size = (ptr-top)*4;
    // write size to last block, and update magick to deleted
    block.setSize(size);
    block.setDeleted();
    // add empty block to end
    if(count+1 < max_blocks)
      blocks[count++] = createBlock(ptr, 4); // createBlock(ptr+4);
  }
}
#endif

StorageBlock FlashStorage::append(void *data, uint32_t size) {
  StorageBlock last = getLastBlock();
  if (last.isFree()) {
    last.write(data, size);
    if (count < max_blocks)
      blocks[count++] =
          createBlock((uint32_t)last.getBlock(), last.getBlockSize());
    return last;
  }else{
    if(last.isValidSize()){
      if(count < max_blocks){
        // create a new block
        blocks[count++] = createBlock((uint32_t)last.getBlock(), last.getBlockSize());
        return append(data, size);
      }else{
        error(FLASH_ERROR, "No more blocks available");
      }
    } else {
      error(FLASH_ERROR, "Invalid non-empty block");
      // invalid non-empty block
      // probably have to rewrite everything
      // todo!
      /* erase(); */
      /* recover(); */
    }
  }
  // return getLastBlock();
  return StorageBlock();
}

uint8_t FlashStorage::getBlocksVerified() {
  uint8_t nof = 0;
  for (uint8_t i = 0; i < count; ++i)
    if (blocks[i].verify())
      nof++;
  return nof;
}

uint32_t FlashStorage::getTotalUsedSize() {
  // returns bytes used by written and deleted blocks
  uint32_t size = 0;
  for (uint8_t i = 0; i < count; ++i)
    if (blocks[i].isValidSize())
      size += blocks[i].getBlockSize();
  return size;
}

uint32_t FlashStorage::getDeletedSize() {
  uint32_t size = 0;
  for (uint8_t i = 0; i < count; ++i)
    if (blocks[i].isDeleted())
      size += blocks[i].getBlockSize();
  return size;
}

// erase entire allocated FLASH memory
void FlashStorage::erase() {
  uint32_t page = start_page;
  eeprom_unlock();
  while (page < end_page) {
    eeprom_erase(page);
    page += EEPROM_PAGE_SIZE;
  }
  eeprom_lock();
  init();
}

void FlashStorage::defrag(void *buffer, uint32_t size) {
  // ASSERT(size >= getWrittenSize(), "Insufficient space for full defrag");
  uint8_t *ptr = (uint8_t *)buffer;
  if (getDeletedSize() > 0 && getWrittenSize() > 0) {
    uint32_t offset = 0;
    for (uint8_t i = 0; i < count && offset < size; ++i) {
      if (blocks[i].verify()) {
        memcpy(ptr + offset, blocks[i].getBlock(), blocks[i].getBlockSize());
        offset += blocks[i].getBlockSize();
      }
    }
    erase();
    eeprom_unlock();
    eeprom_write_block(start_page, buffer, offset);
    eeprom_lock();
    init();
  }
}

StorageBlock FlashStorage::createBlock(uint32_t page, uint32_t offset) {
  // offset = (offset + (4 - 1)) & -4;  // Round up to 4-byte boundary
  if (page + offset + 4 >= end_page || page < start_page) {
    error(FLASH_ERROR, "Block out of bounds");
    return StorageBlock();
  }
  StorageBlock block((uint32_t *)(page + offset));
  return block;
}

#ifndef DAISY
FlashStorage storage(EEPROM_PAGE_BEGIN, EEPROM_PAGE_END, STORAGE_MAX_BLOCKS);
#else
FlashStorage settings_storage(SETTINGS_EEPROM_PAGE_BEGIN, SETTINGS_EEPROM_PAGE_END);
FlashStorage patch_storage(PATCH_EEPROM_PAGE_BEGIN, PATCH_EEPROM_PAGE_END);
#endif

