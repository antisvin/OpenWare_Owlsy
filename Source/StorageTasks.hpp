#ifndef __STORAGE_TASKS_HPP__
#define __STORAGE_TASKS_HPP__

#include "ProgramManager.h"
#include "Storage.h"
#include "message.h"

#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif

extern char _FLASH_STORAGE_SIZE;
extern char _EXTRAM_END, _FLASH_STORAGE_SIZE;

class EraseStorageTask : public BackgroundTask {
public:
  void begin() override {
    program.exitProgram(false);
    state = 0;
    owl.setOperationMode(ERASE_MODE);
    total_sectors = (uint32_t)&_FLASH_STORAGE_SIZE / FLASH_SECTOR_SIZE;
    debugMessage("Erasing storage");
  }

  void loop() override {
    if (state < total_sectors){
      storage.erase(state);
      setParameterValue(LOAD_INDICATOR_PARAMETER, uint32_t(state++) * 4095 / total_sectors);
#ifdef USE_SCREEN
      char buf[40];
      (void)buf;
      char* p;
      (void)p;
      p = &buf[0];
      int size = state * FLASH_SECTOR_SIZE / 1024;
      if (size > 999) {
        p = stpcpy(p, msg_itoa(size / 1024, 10));
        p = stpcpy(p, (const char*)"M");
      }
      else {
        p = stpcpy(p, msg_itoa(size, 10));
        p = stpcpy(p, (const char*)"k");
      }
      p = stpcpy(p, " / ");      
      size = total_sectors * FLASH_SECTOR_SIZE / 1048576;
      p = stpcpy(p, msg_itoa(size, 10));
      p = stpcpy(p, (const char*)"M");
      debugMessage(buf);
#endif
    }
    else {
      end();
      owl.setBackgroundTask(NULL);      
    }
  }

  void end() override {
    debugMessage("Storage erased");
    owl.setOperationMode(RUN_MODE);
    program.startProgram(false);
  }
private:
  uint8_t state;
  uint8_t total_sectors;
};

extern EraseStorageTask erase_task;

#ifdef DEFRAG_TAST
// Can't use blocks anymore ;-(
class DefragStorageTask : public BackgroundTask {
public:
  void begin() override {
    program.exitProgram(false);
    owl.setOperationMode(DEFRAG_MODE);
    state = DEFRAG_COPY;
    sector = 0;
    storage_sectors = (uint32_t)&_FLASH_STORAGE_SIZE / FLASH_SECTOR_SIZE;
    if ((uint32_t)&_FLASH_STORAGE_SIZE % FLASH_SECTOR_SIZE){
      storage_sectors++;
    }
    resource = 0;
    written_sector = 0;
    total_resources = storage.getNumberOfResources();
    buffer_start = (uint32_t)&_EXTRAM_END - (uint32_t)&_FLASH_STORAGE_SIZE;
    buffer_end = buffer_start;
    buffer_sectors = 0;
    debugMessage("Defrag started");
  }

  void loop() override {
#ifdef USE_SCREEN
      char buf[40];
      (void)buf;
      char* p;
      (void)p;
      p = &buf[0];
#endif
    switch (state){
      case DEFRAG_COPY:
        if (resource < total_resources){
          Resource* resource_ptr = storage.getResourceBySlot(resource);
          size_t read_bytes = storage.readResource()
          if (storage_block.verify()){
#ifdef USE_SCREEN
            p = stpcpy(p, "Copying block\n");
            p = stpcpy(p, (const char*)msg_itoa(block, 10));
            p = stpcpy(p, " / ");
            p = stpcpy(p, (const char*)msg_itoa(total_blocks, 10));
            debugMessage(buf);
#endif  
            // Copy storage block
            memcpy((void*)buffer_end, storage_block.getBlock(), storage_block.getBlockSize());
            buffer_end += storage_block.getBlockSize();
            // Update sectors count
            buffer_sectors = (buffer_end - buffer_start) / FLASH_SECTOR_SIZE;
            if ((buffer_end - buffer_start) % FLASH_SECTOR_SIZE)
              buffer_sectors++;
          }
          block++;
        }
        else {
          state = DEFRAG_ERASE;
          debugMessage("Erasing storage");
        }
        break;
      case DEFRAG_ERASE:
        if (sector < storage_sectors){
#ifdef USE_SCREEN
          p = stpcpy(p, "Erasing sector\n");
          p = stpcpy(p, (const char*)msg_itoa(sector, 10));
          p = stpcpy(p, " / ");
          p = stpcpy(p, (const char*)msg_itoa(storage_sectors, 10));
          debugMessage(buf);
#endif          
          storage.erase(sector);
          sector++;
        }
        else {
          state = DEFRAG_RESTORE;
          debugMessage("Restoring");
        }
        break;
      case DEFRAG_RESTORE:
        if (written_sector < buffer_sectors){
#ifdef USE_SCREEN
          p = stpcpy(p, "Restoring sector\n");
          p = stpcpy(p, (const char*)msg_itoa(written_sector, 10));
          p = stpcpy(p, " / ");
          p = stpcpy(p, (const char*)msg_itoa(buffer_sectors, 10));
          debugMessage(buf);
#endif
          storage.write(
            written_sector * FLASH_SECTOR_SIZE, (void*)buffer_start,
            min(FLASH_SECTOR_SIZE, buffer_end - buffer_start));
          buffer_start += FLASH_SECTOR_SIZE;
          written_sector++;
        }
        else {
          state = DEFRAG_DONE;
        }
        break;
      case DEFRAG_DONE:
        end();
        owl.setBackgroundTask(NULL);          
        break;
    }
    setParameterValue(
      LOAD_INDICATOR_PARAMETER,
      (uint32_t(block) + uint32_t(sector) * 4 + uint32_t(written_sector)) * 4095 /
        (uint32_t(total_blocks) + uint32_t(storage_sectors) * 4 + uint32_t(buffer_sectors)));
  }

  void end() override {
    debugMessage("Defrag finished");
    storage.init();        
    owl.setOperationMode(RUN_MODE);
    program.startProgram(false);
  }
private:
  enum DefragState {
    DEFRAG_COPY,
    DEFRAG_ERASE,
    DEFRAG_RESTORE,
    DEFRAG_DONE
  } state;
  uint8_t sector, resource, written_sector;
  uint8_t storage_sectors, total_resources, buffer_sectors;
  uint32_t buffer_start, buffer_end;
};
DefragStorageTask defrag_task;
#endif

#endif
