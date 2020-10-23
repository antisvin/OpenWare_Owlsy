#include "device.h"
#include "FirmwareHeader.h"
#include "ProgramVector.h"

extern char _ISR_VECTOR;
#ifdef OWL_ARCH_H7
extern char _sitcm, _eitcm, _siitcm;
extern char _sdata, _edata, _sidata;
extern char _scode, _ecode, _sicode;
#endif  

#ifdef USE_IWDG
static const uint32_t iwdg_enable     = 1U;
#else
static const uint32_t iwdg_enable     = 0U;
#endif

#ifdef OWL_ARCH_H7
static const uint32_t num_relocations = 3U;
#else
static const uint32_t num_relocations = 0U;
#endif

__attribute__ ((section (".firmware_header")))  
const struct FirmwareHeader firmware_header {
    HEADER_MAGIC,
    sizeof(FirmwareHeader),
    (FIRMWARE_VERSION_MAJOR << 24) | (FIRMWARE_VERSION_MINOR << 16),
    HARDWARE_ID,
    (uint32_t)&_ISR_VECTOR,
    ((num_relocations & OPT_NUM_RELOCATIONS_MASK) << OPT_NUM_RELOCATIONS_OFFSET) |
    ((iwdg_enable & OPT_IWDG_MASK) << OPT_IWDG_OFFSET),
#ifdef OWL_ARCH_H7
    (uint32_t)&_sitcm,
    (uint32_t)&_eitcm,
    (uint32_t)&_siitcm,
    (uint32_t)&_sdata,
    (uint32_t)&_edata,
    (uint32_t)&_sidata,
    (uint32_t)&_scode,
    (uint32_t)&_ecode,
    (uint32_t)&_sicode,
#endif
    HEADER_CHECKSUM_PLACEHOLDER
};