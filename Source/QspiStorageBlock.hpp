#ifndef __QSPI_STORAGE_BLOCK_HPP__
#define __QSPI_STORAGE_BLOCK_HPP__

#include "device.h"
#include <string.h>
#include "message.h"
#include "qspicontrol.h"
#include "BaseStorageBlock.hpp"

extern char _FLASH_STORAGE_BEGIN, _FLASH_STORAGE_END;
#define PATCH_PAGE_BEGIN ((uint32_t)&_FLASH_STORAGE_BEGIN)
#define PATCH_PAGE_END   ((uint32_t)&_FLASH_STORAGE_END)

/*
 * This is only intended to be used for storing patches on Daisy QSPI flash.
 */

#define QSPI_ALIGNMENT 4
/*
 * Writes will be performed in 256 byte pages. Whole storage could be aligned to 4k sector
 * to enable erasure for individual patches. But using flash modification would better distrubet
 * writes over the whole chip.
 */

using QspiStorageBlock = BaseStorageBlock<QSPI_ALIGNMENT>;

template<>
inline bool QspiStorageBlock::isValidSize() const {
    return header != nullptr &&
        (((uint32_t)header & alignment_mask) == 0) &&
        (uint32_t)header >= PATCH_PAGE_BEGIN &&
        (uint32_t)header < PATCH_PAGE_END &&
        (uint32_t)header + getBlockSize() < PATCH_PAGE_END &&
        getDataSize() > 0;
}

template<>
inline bool QspiStorageBlock::write(void* data, uint32_t size) {
    if((uint32_t)header+4+size >= PATCH_PAGE_END)
        return false;

    // On first page we write header + beginning of data
    //uint32_t header_page[QSPI_PAGE_SIZE / 4];
    //header_page[0] = 0xcf000000 | size;
    //size_t header_size = (size < QSPI_PAGE_SIZE - 4) ? size + 4 : QSPI_PAGE_SIZE;
    // Header size is counted in bytes
    //memcpy((void*)(header_page + 1), data, header_size - 4);

    int status = qspi_init(QSPI_MODE_INDIRECT_POLLING);
    if (status != MEMORY_OK) {
        error(FLASH_ERROR, "Flash init failed");
        qspi_init(QSPI_MODE_MEMORY_MAPPED);
        return false;
    }

    uint32_t new_header = 0xcf000000 | size;
    status = qspi_write_block((uint32_t)getBlock(), (uint8_t*)&new_header, 4);
    if (status == MEMORY_OK) {
        status = qspi_write_block((uint32_t)getBlock() + 4, (uint8_t*)data, size);
    }
    
    //status = qspi_write_block((uint32_t)getBlock(), (uint8_t*)header_page, header_size);

    //if (status == MEMORY_OK && size > QSPI_PAGE_SIZE - 4) {
        // Remaining data write.
        // This will be called only when header_size is 256
        //status = qspi_write_block(
        //    (uint32_t)getBlock() + QSPI_PAGE_SIZE,
        //    (uint8_t*)data + (QSPI_PAGE_SIZE - 4),
        //    size - (QSPI_PAGE_SIZE - 4));
    //}
    qspi_init(QSPI_MODE_MEMORY_MAPPED);
    if (status != MEMORY_OK) {
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
inline bool QspiStorageBlock::setDeleted() {
    uint32_t size = getDataSize();
    if(size){
        // We preallocate full page in order to prevent accidental reading outside of valid
        // memory region in certain cases
        uint32_t header_page[QSPI_PAGE_SIZE / 4];
        header_page[0] = 0xc0000000 | size;        
        int status = qspi_init(QSPI_MODE_INDIRECT_POLLING);
        //if (status == MEMORY_OK) {
        //    //status = 
        //    status = qspi_erase((uint32_t)header, (uint32_t)header + QSPI_SECTOR_SIZE);
        //}
        if (status == MEMORY_OK) {
            status = qspi_write_block((uint32_t)header, (uint8_t*)header_page, 4);
        }

        qspi_init(QSPI_MODE_MEMORY_MAPPED);
        
        if (status != MEMORY_OK) {
            // We want to return to memory mapped mode even on error
            error(FLASH_ERROR, "Flash write failed");
            return false;
        }     
        else {
            if(size != getDataSize() || !isDeleted()){
                error(FLASH_ERROR, "Flash delete error");
                return false;
            }
        }
        return true;
    }
    else {
        return false;
    }
}

#endif
