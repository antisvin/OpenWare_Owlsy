#include <stdint.h>
#include "device.h"

#ifdef __cplusplus
 extern "C" {
#endif

#ifdef OLED_I2C
  void oled_init(I2C_HandleTypeDef *i2c);
#else
  void oled_init(SPI_HandleTypeDef *spi);
#endif
  void oled_write(const uint8_t* pixels, uint16_t size);

#ifdef __cplusplus
}
#endif
