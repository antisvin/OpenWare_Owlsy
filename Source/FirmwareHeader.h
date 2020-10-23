#ifndef _FIRMWARE_HEADER_H_
#define _FIRMWARE_HEADER_H_

#include <stdint.h>

#define HEADER_MAGIC                (0xBABECAFE)
#define HEADER_CHECKSUM_PLACEHOLDER (0xDEADC0DE)

#ifdef __cplusplus
 extern "C" {
#endif

#define OPT_IWDG_MASK               0b1
#define OPT_IWDG_OFFSET             0U
#define OPT_NUM_RELOCATIONS_MASK    0b111
#define OPT_NUM_RELOCATIONS_OFFSET  1U

struct FirmwareHeader {
    uint32_t magic;
    uint32_t header_size; // Header size will be determined automatically when building FW
    uint32_t firmware_version;
    uint32_t hardware_id;
    uint32_t isr_vector_address;
    uint32_t options;
#ifdef OWL_ARCH_H7
    uint32_t section_0_start; /* H7 - ITCM */
    uint32_t section_0_end;
    uint32_t section_0_address;
    uint32_t section_1_start; /* H7 - DTCM */
    uint32_t section_1_end;
    uint32_t section_1_address;
    uint32_t section_2_start; /* H7 - Code + static */
    uint32_t section_2_end;
    uint32_t section_2_address;
#endif
    /*
     * Any amount of data can be added here. The limitations are:
     * 1. object should be aligned to 4 bytes
     * 2. checksum must stay last field
     */
    uint32_t checksum; // This field will be set by bootloader after storing FW ROM
};

#ifdef __cplusplus
}
#endif

#endif
