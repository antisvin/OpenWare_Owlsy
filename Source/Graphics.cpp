#include "device.h"
#include "Graphics.h"
#include "errorhandlers.h"
#include "oled.h"

#ifdef OLED_I2C
void Graphics::begin(I2C_HandleTypeDef *i2c) {
  oled_init(i2c);
#else
void Graphics::begin(SPI_HandleTypeDef *spi) {
  oled_init(spi);
#endif
  screen.setBuffer(pixelbuffer);
  screen.clear();
  display();
}

void Graphics::display(){
  oled_write(pixelbuffer, OLED_BUFFER_SIZE);
}

void Graphics::draw(){
  // drawCallback(pixelbuffer, OLED_WIDTH, OLED_HEIGHT);
  // // params.draw(pixelbuffer, OLED_WIDTH, OLED_HEIGHT);
  params.draw(screen);
}

// static void defaultDrawCallback(uint8_t* pixels, uint16_t width, uint16_t height){
//   graphics.screen.setBuffer(pixels);
//   graphics.screen.clear();
//   graphics.params.draw(graphics.screen);
// }

void Graphics::setCallback(void *callback){
  params.setCallback(callback);
  // if(callback == NULL)
  //   drawCallback = defaultDrawCallback;
  // else
  //   drawCallback = (void (*)(uint8_t*, uint16_t, uint16_t))callback;
}

Graphics::Graphics() :
  screen(OLED_WIDTH, OLED_HEIGHT)
  // drawCallback(defaultDrawCallback)
{  
}

void defaultDrawCallback(uint8_t* pixels, uint16_t width, uint16_t height){
  graphics.params.drawTitle(graphics.screen);
  graphics.params.drawMessage(26, graphics.screen);
}
