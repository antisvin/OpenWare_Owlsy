#ifndef _SD_TEST_H
#define _SD_TEST_H

#include "fatfs.h"
#include "message.h"

void test_sd() {
  // retSD = FATFS_LinkDriver(&SD_Driver, SDPath);
  // printf("LINK %i\n", retSD);

  if (f_mount(&SDFatFS, (TCHAR const *)SDPath, 1) == FR_OK) {
    DIR dir;
    FRESULT res;
//    printf("Opening dir...\n");
    res = f_opendir(&dir, "/");
    if (res != FR_OK) {
//      printf("Open dir failed\n");
      return;
    }

    FILINFO fileInfo;
    uint32_t totalFiles = 0;
    uint32_t totalDirs = 0;
//    printf("Root directory:\n");
    for (;;) {
      res = f_readdir(&dir, &fileInfo);
      if ((res != FR_OK) || (fileInfo.fname[0] == '\0')) {
        break;
      }

      if (fileInfo.fattrib & AM_DIR) {
//        printf("  DIR  %s\n", fileInfo.fname);
        totalDirs++;
      } else {
//        printf("-> %s\n", fileInfo.fname);
        totalFiles++;
      }
    }

//    printf("(total: %lu dirs, %lu files)\n", totalDirs, totalFiles);

    res = f_closedir(&dir);
    if (res != FR_OK) {
//      printf("Close failed\n");
      return;
    }
  } else {
//    printf("Mount failed\n");
    extern uint32_t SystemCoreClock;
    debugMessage("Mount failed");
  }
}
#endif
