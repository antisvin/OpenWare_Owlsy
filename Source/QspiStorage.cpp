#include "BaseStorage.hpp"
#include "BaseStorageBlock.hpp"
#include "FlashStorage.h"
#include "device.h"
#include "qspicontrol.h"
#include "message.h"
#include <string.h>

extern char _FLASH_STORAGE_BEGIN;
extern char _FLASH_STORAGE_END;
#define PATCH_EEPROM_PAGE_BEGIN ((uint32_t)&_FLASH_STORAGE_BEGIN)
#define PATCH_EEPROM_PAGE_END ((uint32_t)&_FLASH_STORAGE_END)

//#define QSPI_SECTOR_SIZE (128 * 1024)


#if 0
template<>
void Storage::recover(){
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


// erase entire allocated FLASH memory
template<>
void Storage::erase() {
  qspi_erase(start_page, end_page);
  init();
}

template<>
void Storage::defrag(void *buffer, uint32_t size) {
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
    qspi_write_block(start_page, (uint8_t*)buffer, offset);
    init();
  }
}

Storage storage(PATCH_EEPROM_PAGE_BEGIN, PATCH_EEPROM_PAGE_END);

