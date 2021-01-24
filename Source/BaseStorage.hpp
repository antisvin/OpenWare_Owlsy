#ifndef __BASE_STORAGE_H__
#define __BASE_STORAGE_H__

#include "device.h"
#include "message.h"
#include "BaseStorageBlock.hpp"

template<class StorageBlock, size_t max_blocks = STORAGE_MAX_BLOCKS>
class BaseStorage {
public:
  BaseStorage(uint32_t start_page, uint32_t end_page)
    : count(0), start_page(start_page), end_page(end_page) {}

  void init() {
    uint32_t offset = 0;
    StorageBlock block;
    count = 0;
    do {
      block = createBlock(start_page, offset);
      blocks[count++] = block;
      offset += block.getBlockSize();
    } while (!block.isFree() && count + 1U < max_blocks &&
             start_page + offset < end_page);
    // fills at least one (possibly empty) block into list
  }

  uint8_t getBlocksTotal() const { return count > 0 ? count - 1 : 0; }

  uint8_t getBlocksVerified() const {
    uint8_t nof = 0;
    for (uint8_t i = 0; i < count; ++i)
      if (blocks[i].verify())
        nof++;
    return nof;
  }

  uint32_t getTotalUsedSize() const {
    // returns bytes used by written and deleted blocks
    uint32_t size = 0;
    for (uint8_t i = 0; i < count; ++i)
      if (blocks[i].isValidSize())
        size += blocks[i].getBlockSize();
    return size;
  }

  uint32_t getDeletedSize() const {
    uint32_t size = 0;
    for (uint8_t i = 0; i < count; ++i)
      if (blocks[i].isDeleted())
        size += blocks[i].getBlockSize();
    return size;
  }

  uint32_t getWrittenSize() const { return getTotalUsedSize() - getDeletedSize(); }

  uint32_t getTotalAllocatedSize() const { return end_page - start_page; }

  uint32_t getFreeSize() const {
    return getTotalAllocatedSize() - getTotalUsedSize();
  }

  StorageBlock getLastBlock() const { return blocks[count > 0 ? count - 1 : 0]; }

  StorageBlock getBlock(uint8_t index) const {
    if (index < count)
      return blocks[index];
    else
      return getLastBlock();
  }

  /*
  * Write operations below
  */

 #if 0
  void recover(){
    StorageBlock block = getLastBlock();
    if(!block.isFree() && !block.isValidSize()){
      // count backwards to see how much free space (0xff words) there is
      uint32_t* top = (uint32_t*)block.getData();
      uint32_t* ptr = (uint32_t*)(page_end);
      uint32_t free = 0;
      while(--ptr > top && *ptr == 0xffffffff)
        free += 4;
      uint32_t size = (ptr-top)*4;
      // write size to last block, and update magick to deleted
      block.setSize(size);
      setDeleted(block);
      // add empty block to end
      if(count+1 < STORAGE_MAX_BLOCKS)
        blocks[count++] = createBlock(ptr, 4); // createBlock(ptr+4);
    }
  }
#endif

  StorageBlock append(void *data, uint32_t size) {
    StorageBlock last = getLastBlock();
    if (last.isFree()) {
      if (last.write(data, size)) {
        if (count < max_blocks)
          blocks[count++] = createBlock((uint32_t)last.getBlock(), last.getBlockSize());
        return last;
      }
    }
    else if (last.isValidSize()) {
      if (count < max_blocks) {
        // create a new block
        blocks[count++] = createBlock((uint32_t)last.getBlock(), last.getBlockSize());
        return append(data, size);
      }
      else {
        error(FLASH_ERROR, "No more blocks available");
      }
    }
    else {
      error(FLASH_ERROR, "Invalid non-empty block");
      // invalid non-empty block
      // probably have to rewrite everything
      // todo!
      /* erase(); */
      /* recover(); */
    }
    // return getLastBlock();
    return StorageBlock();
  }

  void erase(uint8_t sector = 0xFF);
  bool setDeleted(StorageBlock &block);
  void defrag(void *buffer, uint32_t size);
  void write(uint32_t offset, void* buffer, uint32_t size);

protected:
  StorageBlock blocks[max_blocks];
  uint8_t count;
  uint32_t start_page, end_page;

  StorageBlock createBlock(uint32_t page, uint32_t offset) {
    // offset = (offset + (4 - 1)) & -4;  // Round up to 4-byte boundary
    if (page + offset + 4 >= end_page || page < start_page) {
      error(FLASH_ERROR, "Block out of bounds");
      return StorageBlock();
    }
    StorageBlock block((uint32_t *)(page + offset));
    return block;
  }
};

#endif
