#ifndef __QSPI_CONTROL_H__
#define __QSPI_CONTROL_H__


#ifdef __cplusplus
extern "C" {
#endif

#define MEMORY_OK ((uint32_t)0x00)    /**< & */
#define MEMORY_ERROR ((uint32_t)0x01) /**< & */

#define QSPI_TEXT                                                              \
  __attribute__((section(".qspiflash_text"))) /**< used for reading memory in  \
                                                 memory_mapped mode. */
#define QSPI_DATA                                                              \
  __attribute__((section(".qspiflash_data"))) /**< used for reading memory in  \
                                                 memory_mapped mode. */
#define QSPI_BSS                                                               \
  __attribute__((section(".qspiflash_bss"))) /**< used for reading memory in   \
                                                memory_mapped mode. */

/**
  Driver for QSPI peripheral to interface with external flash memory.
  Currently supported QSPI Devices:
  * IS25LP080D
*/

/**
  Modes of operation.
  Memory Mapped mode: QSPI configured so that the QSPI can be
  read from starting address 0x90000000. Writing is not
  possible in this mode.
  Indirect Polling mode: Device driver enabled.
  Read/Write possible via qspi_* functions
*/
typedef enum {
  QSPI_MODE_MEMORY_MAPPED,    /**< & */
  QSPI_MODE_INDIRECT_POLLING, /**< & */
  QSPI_MODE_LAST,             /**< & */
} QspiMode;

/**
  Initializes QSPI peripheral, and Resets, and prepares memory for access.
  @param mode which QSPI mode to use.
  @retval MEMORY_OK or MEMORY_ERROR
*/
int qspi_init(QspiMode mode);

/**
  Deinitializes the peripheral
  This should be called before reinitializing QSPI in a different mode.
  @retval MEMORY_OK or MEMORY_ERROR
*/
int qspi_deinit();

/**
  Writes a single page to to the specified address on the QSPI chip.
  For IS25LP* page size is 256 bytes.
  @param adr Address to write to
  @param data Data to write
  @param size Data size
  @retval MEMORY_OK or MEMORY_ERROR
*/
int qspi_write_page(uint32_t address, uint8_t *data, uint32_t size);

/**
  Writes data in buffer to to the QSPI. Starting at address to address+size
  @param address Address to write to
  @param data Data to write
  @param size Data size
  @retval MEMORY_OK or MEMORY_ERROR
 */
int qspi_write_block(uint32_t address, uint8_t *data, uint32_t size);

/**
  Erases the area specified on the chip.
  Erasures will happen by 4K, 32K or 64K increments.
  Smallest erase possible is 4kB at a time. (on IS25LP*)
  @param start_adr Address to begin erasing from
  @param end_adr  Address to stop erasing at
  @retval MEMORY_OK or MEMORY_ERROR
 */
int qspi_erase(uint32_t start_adr, uint32_t end_adr);

/**
  Erases a single sector of the chip.
  TODO: Document the size of this function.
  @param addr Address of sector to erase
  @retval MEMORY_OK or MEMORY_ERROR
 */
int qspi_erase_sector(uint32_t addr);

#ifdef __cplusplus
}
#endif

#endif