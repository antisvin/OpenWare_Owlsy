#ifndef __FLASH_STORAGE_BLOCK_HPP__
#define __FLASH_STORAGE_BLOCK_HPP__

#include "device.h"
#include <string.h>
#include "message.h"
#include "eepromcontrol.h"
#include "BaseStorageBlock.hpp"

extern char _FLASH_STORAGE_BEGIN, _FLASH_STORAGE_END;
#define EEPROM_PAGE_START   ((uint32_t)&_FLASH_STORAGE_BEGIN)
#define EEPROM_PAGE_END   ((uint32_t)&_FLASH_STORAGE_END)

#ifdef OWL_ARCH_L4
// Double word alignment is required for writing to L4 flash
using FlashStorageBlock = BaseStorageBlock<8>;
#else
using FlashStorageBlock = BaseStorageBlock<4>;
#endif

template<>
inline bool FlashStorageBlock::isValidSize() const {
    return header != nullptr &&
        (((uint32_t)header & alignment_mask) == 0) &&
        (uint32_t)header >= EEPROM_PAGE_START &&
        (uint32_t)header < EEPROM_PAGE_END &&
        (uint32_t)header + getBlockSize() < EEPROM_PAGE_END &&
        getDataSize() > 0;
}

template<>
inline bool FlashStorageBlock::write(void* data, uint32_t size) {
    if((uint32_t)header+4+size > EEPROM_PAGE_END)
        return false;
    eeprom_unlock();
    eeprom_wait(); // clear flash flags
#ifdef OWL_ARCH_L4
    uint32_t address = (uint32_t)getBlock();
    bool status = eeprom_write_dword(address, RESOURCE_NOT_DELETED); // write resource status
    if (!status) {        
        // We have to write resource header word and first word of resource data as a single operation
        // because L4 flash is PITA!
        uint64_t header_dword = 0xcf000000 | size;          // magick and size
        address += sizeof(uint64_t);
        header_dword |= (uint64_t)(*(uint32_t*)data) << 32; // plus first data word
        // Now we'll write header + first data word as double word
        status = eeprom_write_dword(address, header_dword);
    }
    if (!status){
        // Write remaining data. Magic and first word is already written. Note that size is aligned
        // and includes magic length - so we substract a double word from total size
        address += sizeof(uint64_t);
        uint32_t remaining_size = size - sizeof(uint32_t); // remaining chunk is unaligned due to 4 bytes written
        remaining_size += (block_offset - remaining_size % block_offset); // align up to fill storage block
        status = eeprom_write_block(address, (void*)((uint32_t*)data + 1), remaining_size);
    }
#else
    bool status = eeprom_write_word((uint32_t)getBlock(), 0xcf000000 | size); // set magick and size
    status = status || eeprom_write_block((uint32_t)getData(), data, size);
#endif
    eeprom_lock();
    if(status){
        error(FLASH_ERROR, "Flash write failed");
        return false;
    }
    if(size != getDataSize()){
        error(FLASH_ERROR, "Size verification failed");
        return false;
    }
    if(memcmp(data, getData(), size) != 0){
        error(FLASH_ERROR, "Data verification failed");
        return false;
    }
    return true;
}

template<>
inline bool FlashStorageBlock::setDeleted() {
    uint32_t size = getDataSize();
    if(size){
        eeprom_unlock();
#ifdef OWL_ARCH_L4
        // L4 flash allows overwriting only if new value contains nothing but zeros
        bool status = eeprom_write_dword((uint32_t)getBlock(), RESOURCE_DELETED);
#else
        bool status = eeprom_write_word((uint32_t)header, 0xc0000000 | size);
#endif
        eeprom_lock();
        if(status){
            error(FLASH_ERROR, "Flash delete failed");
            return false;
        }
        if(size != getDataSize() || !isDeleted()){
            error(FLASH_ERROR, "Flash delete error");
            return false;
        }
        return true;
    }
    else {
        return false;
    }
}

#endif