#ifndef __QSPI_STORAGE_BLOCK_HPP__
#define __QSPI_STORAGE_BLOCK_HPP__

#include "device.h"
#include <string.h>
#include "message.h"
#include "qspicontrol.h"
#include "BaseStorageBlock.hpp"

extern char _FLASH_STORAGE_END;
#define PATCH_PAGE_END   ((uint32_t)&_FLASH_STORAGE_END)

/*
 * This is only intended to be used for storing patches on Daisy QSPI flash.
 */

#define QSPI_PAGE_SIZE 256
#define QSPI_SECTOR_SIZE 4096
#define QSPI_ALIGNMENT QSPI_SECTOR_SIZE
// Writes are in 256 byte pages
// Erasure is in 4k subsectors

class QspiStorageBlock : public BaseStorageBlock<QSPI_ALIGNMENT> {
public:
    QspiStorageBlock() : BaseStorageBlock<QSPI_ALIGNMENT>() {};
    QspiStorageBlock(uint32_t* header) : BaseStorageBlock<QSPI_ALIGNMENT>(header) {};

    bool isValidSize() const {
        return header != nullptr && getDataSize() > 0 && (uint32_t)header + getBlockSize() < PATCH_PAGE_END;
    }

    bool write(void* data, uint32_t size) {
        if((uint32_t)header+4+size > PATCH_PAGE_END)
            return false;

        // On first page we write header + beginning of data
        uint32_t header_page[QSPI_ALIGNMENT / 4];
        header_page[0] = 0xcf000000 | size;
        size_t header_size = (size < QSPI_ALIGNMENT - 4) ? size + 4 : QSPI_ALIGNMENT;
        memcpy((void*)(header_page + 1), data, header_size);
        int status = qspi_write_block((uint32_t)getBlock(), (uint8_t*)header_page, header_size);

        if (size > QSPI_ALIGNMENT - 4 && status == MEMORY_OK) {
            // Remaining data write.
            // This will be called only when header_size is 256
            status = qspi_write_block(
                (uint32_t)getBlock() + header_size / 4,
                (uint8_t*)data + QSPI_ALIGNMENT - 4,
                size - QSPI_ALIGNMENT + 4);
        }
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

    bool setDeleted() {
        uint32_t size = getDataSize();
        if(size){
            // We can't write a single word on QSPI, so we'll read 4k page, set necessary bits
            // and write it back
            uint32_t old_page[1024];
            // Copy sector to memory
            memcpy((void*)old_page, getData(), 4096);
            old_page[0] =  0xc0000000 | size;

            // Erase sector
            int status = qspi_erase_sector((uint32_t)getBlock());

            if (status == MEMORY_OK) {
                // Write updated sector
                status = qspi_write_block((uint32_t)getBlock(), (uint8_t*)old_page, 4096);
                if (status != MEMORY_OK) {
                    error(FLASH_ERROR, "Flash write failed");
                    return false;
                }
            }
            else {
                error(FLASH_ERROR, "Flash erasure failed");
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
};

#endif