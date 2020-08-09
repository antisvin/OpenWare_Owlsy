#ifndef __HAL_ENCODERS_H
#define __HAL_ENCODERS_H

#ifdef __cplusplus
extern "C" {
#endif

// Prototypes
void Encoders_readAll(void);
void Encoders_readSwitches(void);
#ifdef USE_SPI_ENCODERS
void Encoders_init(SPI_HandleTypeDef *spiconfig);
#else
// Right now we only need to init a single encoder, so there's no point to
// make a multi-encoder initializer
void Encoders_init(GPIO_TypeDef *port_a, uint32_t pin_a, GPIO_TypeDef *port_b,
                   uint32_t pin_b, GPIO_TypeDef *port_click, uint32_t pin_click);
#endif
int16_t *Encoders_get();

#ifdef __cplusplus
}
#endif
#endif
