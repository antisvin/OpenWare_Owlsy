#ifndef __BSP_SD_DISKIO_H__
#define __BSP_SD_DISKIO_H__ /**< & */

#include <stdint.h>
#include "stm32h7xx_hal.h"

#ifdef __cplusplus
 extern "C" {
#endif

/* Exported types --------------------------------------------------------*/
/**
  * @brief SD Card information structure
  */
#define BSP_SD_CardInfo HAL_SD_CardInfoTypeDef

/** @addtogroup utility
    @{
    */


/** Functions for handling DiskIO via SDMMC
    These are usually configured through the FatFS driver/interface,
    and won't need to be accessed directly often.
*/

#define BSP_SD_CardInfo HAL_SD_CardInfoTypeDef   /**< & */
#define MSD_OK ((uint8_t)0x00)                   /**< & */
#define MSD_ERROR ((uint8_t)0x01)                /**< & */
#define MSD_ERROR_SD_NOT_PRESENT ((uint8_t)0x02) /**< & */
#define SD_TRANSFER_OK ((uint8_t)0x00)           /**< & */
#define SD_TRANSFER_BUSY ((uint8_t)0x01)         /**< & */
#define SD_PRESENT ((uint8_t)0x01)               /**< & */
#define SD_NOT_PRESENT ((uint8_t)0x00)           /**< & */
#define SD_DATATIMEOUT ((uint32_t)100000000)     /**< & */

// Functions internal for diskIO
uint8_t BSP_SD_Init(void);     /**< \return card state, ERROR, etc.*/
uint8_t BSP_SD_ITConfig(void); /**< \return card state, ERROR, etc. */

/** \param  *pData &
    \param  ReadAddr Address to read from
    \param  NumOfBlocks Number of blocks to be read
    \param  Timeout Timeout len in ms
    \return OK ERROR, etc.
*/
uint8_t BSP_SD_ReadBlocks(uint32_t *pData,
                          uint32_t  ReadAddr,
                          uint32_t  NumOfBlocks,
                          uint32_t  Timeout);
/** \param  *pData &
    \param  WriteAddr Address to write to
    \param  NumOfBlocks Number of blocks to be written
    \param  Timeout Timeout len in ms
    \return card state, ERROR, etc.
*/
uint8_t BSP_SD_WriteBlocks(uint32_t *pData,
                           uint32_t  WriteAddr,
                           uint32_t  NumOfBlocks,
                           uint32_t  Timeout);

/** No timeout
    \param  *pData &
    \param  ReadAddr Address to read from
    \param  NumOfBlocks Number of blocks to be read
    \return card state, ERROR, etc.
*/
uint8_t
BSP_SD_ReadBlocks_DMA(uint32_t *pData, uint32_t ReadAddr, uint32_t NumOfBlocks);

/** No timeout
    \param  *pData &
    \param  WriteAddr Address to write to
    \param  NumOfBlocks Number of blocks to be read
    \return card state, ERROR, etc.
*/
uint8_t BSP_SD_WriteBlocks_DMA(uint32_t *pData,
                               uint32_t  WriteAddr,
                               uint32_t  NumOfBlocks);

/** Erase a section of memory
    \param StartAddr Address to start erasing at
    \param EndAddr Address to stop erasing at
    \return card state, ERROR, etc.
*/
uint8_t BSP_SD_Erase(uint32_t StartAddr, uint32_t EndAddr);

/** \return card state, ERROR, etc. */
uint8_t BSP_SD_GetCardState(void);

/** \param *CardInfo Pointer to write card info to */
void BSP_SD_GetCardInfo(BSP_SD_CardInfo *CardInfo);

/** \return Is card detected */
uint8_t BSP_SD_IsDetected(void);

/** These functions can be modified in case the current settings (e.g. DMA stream)
  need to be changed for specific application needs /n 

  Abort the callback */
void BSP_SD_AbortCallback(void);
/** Read complete callback */
void BSP_SD_WriteCpltCallback(void);
/** Write complete callback */
void BSP_SD_ReadCpltCallback(void);

#ifdef __cplusplus
}
#endif

#endif
/** @} */
