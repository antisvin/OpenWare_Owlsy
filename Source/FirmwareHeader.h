#ifndef _FIRMWARE_HEADER_H_
#define _FIRMWARE_HEADER_H_

#include <stdint.h>

#ifdef __cplusplus
 extern "C" {
#endif

/*
 * Currently this is not used directly, so should be treated like a reference.
 * 
 * Actual header structure is defined in startup file, while processing logic is configured via hardware.h
 */

struct FirmwareHeader {
    uint32_t magic;
    /* TODO: common headers, i.e.:
    firmware_version
    hardware_id
    isr_vector_address
     */
#ifdef OWL_ARCH_H7
    uint32_t section_0_start; /* ITCM */
    uint32_t section_0_end;
    uint32_t section_0_address;
    uint32_t section_1_start; /* DTCM */
    uint32_t section_1_end;
    uint32_t section_1_address;
    uint32_t section_2_start; /* Code + static */
    uint32_t section_2_end;
    uint32_t section_2_address;
    uint32_t section_3_start; /* Reserved */
    uint32_t section_3_end;
    uint32_t section_3_address;
    uint32_t section_4_start; /* Reserved */
    uint32_t section_4_end;
    uint32_t section_4_address;
#endif
    /* Possibly end with
    uint32_t checksum;
    */
};

#ifdef __cplusplus
}
#endif

#endif
