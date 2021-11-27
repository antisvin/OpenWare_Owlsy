#ifndef __SD_CARD_TASK_HPP__
#define __SD_CARD_TASK_HPP__

#include "ProgramManager.h"
#include "Storage.h"
#include "message.h"

/* FatFs includes component */
#include "fatfs.h"

//uint8_t rtext[64] CACHE_ALIGNED;
#define BUF_SIZE 512
uint8_t sd_buf[BUF_SIZE] CACHE_ALIGNED;

// 32812
// bank05.wav

class SdCardTask : public BackgroundTask {
public:
    enum SdStatus {
        PENDING,
        READING,
        SUCCESS,
        FAILURE,
    };
    void setDestination(const char* name, uint8_t* dst, size_t max_size) {
        strncpy(filename, name, 32);
        this->max_size = max_size;
        this->dst = dst;
    }
    void begin() override {
        status = PENDING;
        if (!is_initialized) {
            BSP_SD_Init();
            if(BSP_SD_IsDetected()) {
                is_initialized = true;
            }            
        }
        if (!is_initialized) {
            debugMessage("Card not initialized");
            status = FAILURE;
        }
    }
    void loop() override {
        if (status == PENDING) {
            if (f_mount(&SDFatFS, (TCHAR const*)SDPath, 0) == FR_OK) {
                DIR dir;
                FRESULT res;
                printf("Opening...\n");
                res = f_opendir(&dir, "/");
                if(res != FR_OK) {
                    printf("Open dir failed\n");
                    status = FAILURE;
                    end();                    
                }

                FILINFO fileInfo;
                uint32_t totalFiles = 0;
                uint32_t totalDirs = 0;
                printf("Root directory:\n");
                for(;;) {
                    res = f_readdir(&dir, &fileInfo);
                    if((res != FR_OK) || (fileInfo.fname[0] == '\0')) {
                        break;
                    }
       
                    if(fileInfo.fattrib & AM_DIR) {
                        printf("  DIR  %s\n", fileInfo.fname);
                        totalDirs++;
                    }
                    else {
                        printf("-> %s\n", fileInfo.fname);
                        totalFiles++;
                    }
                }

                printf("(total: %lu dirs, %lu files)\n",
                    totalDirs, totalFiles);

                res = f_closedir(&dir);
                if(res != FR_OK) {
                    printf("Close failed\n");
                    status = FAILURE;
                    end();
                }


                FILINFO fno;
                if (f_stat(filename, &fno) == FR_OK) {
                    if (fno.fsize > max_size) {
                        printf("File is too large\n");                        
                        status = FAILURE;
                        end();
                    }
                    else {
                        max_size = fno.fsize;
                        if(f_open(&SDFile, filename, FA_READ | FA_OPEN_EXISTING) == FR_OK) {
                            status = READING;
                        }
                    }
                }
                else {
                    printf("File not found\n");
                    status = FAILURE;
                    end();
                }
            }
            else {
                printf("Card not mounted\n");
                status = FAILURE;
                owl.setBackgroundTask(NULL);      
                end();
            }
        }
        if (status == READING) {
            if (f_eof(&SDFile)) {
                printf("Read done\n");
                status = SUCCESS;
                owl.setBackgroundTask(NULL);      
                end();
            }
            else {
                UINT num_read;
                if (f_read(&SDFile, sd_buf, BUF_SIZE, &num_read) != FR_OK) {
                    printf("Error reading file\n");
                    status = FAILURE;
                    end();                    
                }
                else {
                    printf("COPY\n");
                    memcpy(dst, sd_buf, num_read);
                    dst += num_read;
                }
            }
        }

    }
    void end() override {
    }
private:
    char filename[33] = {};
    size_t max_size;
    uint8_t* dst;
    SdStatus status;
    uint8_t workBuffer[2 * _MAX_SS];
    bool is_initialized = false;
};

extern SdCardTask sd_card_task;

#endif
