#ifndef __FLASH_STORAGE_BLOCK_HPP__
#define __FLASH_STORAGE_BLOCK_HPP__

#include "device.h"
#include <string.h>
#include "message.h"
#include "eepromcontrol.h"
#include "BaseStorageBlock.hpp"

#ifndef DAISY
extern char _FLASH_STORAGE_END;
#define EEPROM_PAGE_END   ((uint32_t)&_FLASH_STORAGE_END)
#else
extern char _SETTINGS_STORAGE_END;
extern char _PATCH_STORAGE_END;
#define EEPROM_PAGE_END   ((uint32_t)&_SETTINGS_STORAGE_END)
#define PATCH_PAGE_END   ((uint32_t)&_PATCH_STORAGE_END)
#endif



class FlashStorageBlock : public BaseStorageBlock<4> {
public:
    FlashStorageBlock() : BaseStorageBlock<4>() {};
    FlashStorageBlock(uint32_t* header) : BaseStorageBlock<4>(header) {};

    bool isValidSize() const {
        return header != nullptr && getDataSize() > 0 && 
        (uint32_t)header + getBlockSize() < EEPROM_PAGE_END;
    }

    bool write(void* data, uint32_t size) {
        if((uint32_t)header+4+size > EEPROM_PAGE_END)
            return false;
        eeprom_unlock();
        eeprom_wait(); // clear flash flags
        bool status = eeprom_write_word((uint32_t)getBlock(), 0xcf000000 | size); // set magick and size
        status = status || eeprom_write_block((uint32_t)getData(), data, size);
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

    bool setDeleted() {
        uint32_t size = getDataSize();
        if(size){
            eeprom_unlock();
            bool status = eeprom_write_word((uint32_t)header, 0xc0000000 | size);
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
};

#ifdef DAISY
class QspiStorageBlock : public BaseStorageBlock<4096> {
public:
    QspiStorageBlock() : BaseStorageBlock<4096>() {};
    QspiStorageBlock(uint32_t* header) : BaseStorageBlock<4096>(header) {};

    bool isValidSize() const {
        return header != nullptr && getDataSize() > 0 && 
        (uint32_t)header + getBlockSize() < PATCH_PAGE_END;
    }

    bool write(void* data, uint32_t size) {
        error(FLASH_ERROR, "QSPI in readonly mode");
        return false;
    }

    bool setDeleted() {
        error(FLASH_ERROR, "QSPI in readonly mode");
        return false;
    }
};
#endif

#endif