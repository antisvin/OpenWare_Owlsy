#ifdef OWL_ARCH_F4
#include "stm32f4xx_hal.h"
#elif defined(OWL_ARCH_F7)
#include "stm32f7xx_hal.h"
#elif defined(OWL_ARCH_H7)
#include "stm32h7xx_hal.h"
#else
#error "Unknown OWL architecture"
#endif
