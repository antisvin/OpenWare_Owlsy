#ifndef __FlashStorage_h__
#define __FlashStorage_h__

#include "StorageBlock.h"
#include <device.h>
#include <inttypes.h>


class FlashStorage {
private:
  StorageBlock blocks[STORAGE_MAX_BLOCKS];
  uint8_t count;

public:
  FlashStorage(uint32_t start_page, uint32_t end_page, uint32_t max_blocks)
      : count(0), start_page(start_page), end_page(end_page),
        max_blocks(max_blocks) {}
  void init();
  uint8_t getBlocksTotal() { return count > 0 ? count - 1 : 0; }
  uint8_t getBlocksVerified();
  uint32_t getTotalUsedSize();
  uint32_t getDeletedSize();
  uint32_t getWrittenSize() { return getTotalUsedSize() - getDeletedSize(); }
  uint32_t getTotalAllocatedSize();
  uint32_t getFreeSize() {
    return getTotalAllocatedSize() - getTotalUsedSize();
  }
  void recover();
  void defrag(void *buffer, uint32_t size);
  StorageBlock append(void *data, uint32_t size);
  // erase entire allocated FLASH memory
  void erase();
  StorageBlock getLastBlock() { return blocks[count > 0 ? count - 1 : 0]; }
  StorageBlock getBlock(uint8_t index) {
    if (index < count)
      return blocks[index];
    else
      return getLastBlock();
  }

private:
  uint32_t start_page, end_page, max_blocks;
  StorageBlock createBlock(uint32_t page, uint32_t offset);
};

#ifndef DAISY
extern FlashStorage storage;
#define settings_storage storage
#define patch_storage storage
#else
extern FlashStorage settings_storage;
extern FlashStorage patch_storage;
#endif

#endif // __FlashStorage_h__
