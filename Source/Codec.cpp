#include "device.h"
#include "Owl.h"
#include "Codec.h"
#include "errorhandlers.h"
#include "ApplicationSettings.h"
#include <cstring>
#include "ProgramManager.h"

#if defined USE_CS4271
#define HSAI_RX hsai_BlockB1
#define HSAI_TX hsai_BlockA1
#define HDMA_RX hdma_sai1_b
#define HDMA_TX hdma_sai1_a
#elif defined MULTI_CODEC
#define HDMA_TX HDMA_TX1
#define HDMA_RX HDMA_RX1
  #if NOF_INPUT_CODECS > 0
  extern DMA_HandleTypeDef HDMA_RX1;
  #if NOF_INPUT_CODECS > 1
  extern DMA_HandleTypeDef HDMA_RX2;
  #endif
  #if NOF_INPUT_CODECS > 2
  extern DMA_HandleTypeDef HDMA_RX3;
  #endif
  #if NOF_INPUT_CODECS > 3
  extern DMA_HandleTypeDef HDMA_RX4;
  #endif
  #endif
  #if NOF_OUTPUT_CODECS > 0
  extern DMA_HandleTypeDef HDMA_TX1;
  #endif
  #if NOF_OUTPUT_CODECS > 1
  extern DMA_HandleTypeDef HDMA_TX2;
  #endif
  #if NOF_OUTPUT_CODECS > 2
  extern DMA_HandleTypeDef HDMA_TX3;
  #endif
  #if NOF_OUTPUT_CODECS > 3
  extern DMA_HandleTypeDef HDMA_TX4;
  #endif
#elif defined USE_PCM3168A
#define HSAI_RX hsai_BlockA1
#define HSAI_TX hsai_BlockB1
#define HDMA_RX hdma_sai1_a
#define HDMA_TX hdma_sai1_b
#elif defined USE_WM8731
#define HDMA_RX hdma_i2s2_ext_rx
#define HDMA_TX hdma_i2s2_ext_rx // linked
#elif defined USE_ADS1294
#define HDMA_TX hdma_spi1_rx
#endif

extern "C" {
  uint16_t codec_blocksize = AUDIO_BLOCK_SIZE;
#ifdef MULTI_CODEC
  int32_t codec_rxbuf[CODEC_BUFFER_SIZE / 2];
  int32_t codec_txbuf[CODEC_BUFFER_SIZE / 2];
  #if NOF_INPUT_CODECS > 0
  int32_t codec_rx[NOF_INPUT_CODECS][CODEC_BUFFER_SIZE] DMA_RAM;
  #endif
  #if NOF_OUTPUT_CODECS > 0
  int32_t codec_tx[NOF_OUTPUT_CODECS][CODEC_BUFFER_SIZE / NOF_OUTPUT_CODECS] DMA_RAM;
  #endif
#else
  int32_t codec_rxbuf[CODEC_BUFFER_SIZE] DMA_RAM;
  int32_t codec_txbuf[CODEC_BUFFER_SIZE] DMA_RAM;
  extern DMA_HandleTypeDef HDMA_TX;
  extern DMA_HandleTypeDef HDMA_RX;
#endif
}

#ifdef USE_USBD_AUDIO
#include "usbd_audio.h"

void usbd_audio_mute_callback(int16_t gain){
  // todo!
#ifdef DEBUG
  printf("mute %d\n", gain);
#endif
}

void usbd_audio_gain_callback(int16_t gain){
  // codec_set_gain_in(gain); todo!
#ifdef DEBUG
  printf("gain %d\n", gain);
#endif
}
#endif // USE_USBD_AUDIO

uint16_t Codec::getBlockSize(){
  return codec_blocksize;
}

#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif

void Codec::init(){
  // todo: if(wet):
  // audio_tx_buffer = CircularBuffer<int32_t>(codec_txbuf, CODEC_BUFFER_SIZE);
  // audio_rx_buffer = CircularBuffer<int32_t>(codec_rxbuf, CODEC_BUFFER_SIZE);
  // todo: else
  // audio_tx_buffer = CircularBuffer<int32_t>(codec_rxbuf, CODEC_BUFFER_SIZE);
  // audio_rx_buffer = CircularBuffer<int32_t>(codec_txbuf, CODEC_BUFFER_SIZE);
  // audio_tx_buffer.reset();
  // audio_rx_buffer.reset();
  codec_init();
}

void Codec::reset(){
  // todo: this is called when blocksize is changed
  stop();
  // audio_tx_buffer.reset();
  // audio_rx_buffer.reset();
  start();
}

void Codec::ramp(uint32_t max){
#ifdef MULTI_CODEC
  uint32_t incr = max / CODEC_BUFFER_SIZE / 2;
  for(int i = 0; i < CODEC_BUFFER_SIZE / 2; ++i)
#else
  uint32_t incr = max/CODEC_BUFFER_SIZE;
  for(int i=0; i<CODEC_BUFFER_SIZE; ++i)
#endif
    codec_txbuf[i] = i*incr;
}

void Codec::clear(){
  set(0);
}

int32_t Codec::getMin(){
  int32_t min = codec_txbuf[0];
#ifdef MULTI_CODEC
  for(int i = 1; i < CODEC_BUFFER_SIZE / 2; ++i)
#else
  for(int i=1; i<CODEC_BUFFER_SIZE; ++i)
#endif
    if(codec_txbuf[i] < min)
      min = codec_txbuf[i];
  return min;
}

int32_t Codec::getMax(){
  int32_t max = codec_txbuf[0];
#ifdef MULTI_CODEC
  for(int i=1; i<CODEC_BUFFER_SIZE / 2; ++i)
#else
  for(int i=1; i<CODEC_BUFFER_SIZE; ++i)
#endif
    if(codec_txbuf[i] > max)
      max = codec_txbuf[i];
  return max;
}

float Codec::getAvg(){
  float avg = 0;
#ifdef MULTI_CODEC
  for(int i=0; i<CODEC_BUFFER_SIZE / 2; ++i)
    avg += codec_txbuf[i];
  return avg / CODEC_BUFFER_SIZE * 2;
#else
  for(int i=0; i<CODEC_BUFFER_SIZE; ++i)
    avg += codec_txbuf[i];
  return avg / CODEC_BUFFER_SIZE;
#endif
}

void Codec::set(int32_t value){
#ifdef MULTI_CODEC
  for(int i=0; i<CODEC_BUFFER_SIZE / 2; ++i){
#else
  for(int i=0; i<CODEC_BUFFER_SIZE; ++i){
#endif
    codec_txbuf[i] = value;
    codec_rxbuf[i] = value;
  }
}

void Codec::bypass(bool doBypass){
  codec_bypass(doBypass);
}

void Codec::mute(bool doMute){
  codec_set_gain_out(0); // todo: fixme!
}

void Codec::setInputGain(int8_t value){
  codec_set_gain_in(value);
}

void Codec::setOutputGain(int8_t value){
  codec_set_gain_out(value);
}

void Codec::setHighPass(bool hpf){
#if defined USE_CS4271
  if(hpf)
    codec_write(0x06, 0x10); // hp filters enabled, i2s data
  else
    codec_write(0x06, 0x10 | 0x03 ); // hp filters disabled
#endif
#ifdef USE_PCM3168A
  if(hpf)
    codec_write(82, 0b00000000); // enable HPF for all ADC channels
  else
    codec_write(82, 0b00000111); // disable HPF for all ADC channels
#endif
#ifdef USE_CS4271
  if(hpf)
    codec_write(0x06, 0x10); // hp filters enabled
  else
    codec_write(0x06, 0x10 | 0x03 ); // hp filters disabled
#endif
}

/** Get the number of individual samples (across channels) that have already been 
 * transferred to/from the codec in this block 
 */
size_t Codec::getSampleCounter(){
  // read NDTR: the number of remaining data units in the current DMA Stream transfer.
  return (CODEC_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(&HDMA_TX)) % (codec_blocksize*AUDIO_CHANNELS);
  // return (DWT->CYCCNT)/ARM_CYCLES_PER_SAMPLE;
}

#ifdef USE_IIS3DWB
extern "C" {
  void iis3dwb_read();
}

void Codec::start(){
  extern TIM_HandleTypeDef htim8;
  HAL_TIM_Base_Start_IT(&htim8);
  HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_1);
  iis3dwb_read();
}

void Codec::stop(){
  extern TIM_HandleTypeDef htim8;
  HAL_TIM_Base_Stop(&htim8);
}
#endif

#ifdef USE_ADS1294
#include "ads.h"

void Codec::start(){
  codec_blocksize = AUDIO_BLOCK_SIZE;
  ads_start_continuous();
  extern TIM_HandleTypeDef htim8;
  HAL_TIM_Base_Start_IT(&htim8);
  HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_4);
}

void Codec::stop(){
  ads_stop_continuous();
  extern TIM_HandleTypeDef htim8;
  HAL_TIM_Base_Stop(&htim8);
}

#endif // USE_ADS1294

#ifdef USE_WM8731

extern "C" {
  extern I2S_HandleTypeDef hi2s2;
}

void Codec::stop(){
  HAL_I2S_DMAStop(&hi2s2);
}

void Codec::start(){
  setInputGain(settings.audio_input_gain);
  setOutputGain(settings.audio_output_gain);
  // codec_blocksize = min(AUDIO_BLOCK_SIZE, settings.audio_blocksize);
  codec_blocksize = AUDIO_BLOCK_SIZE;
  HAL_StatusTypeDef ret;
  /* See STM32F405 Errata, I2S device limitations */
  /* The I2S peripheral must be enabled when the external master sets the WS line at: */
  while(HAL_GPIO_ReadPin(I2S_LRCK_GPIO_Port, I2S_LRCK_Pin)); // wait for low
  /* High level when the I2S protocol is selected. */
  while(!HAL_GPIO_ReadPin(I2S_LRCK_GPIO_Port, I2S_LRCK_Pin)); // wait for high

  // When a 16-bit data frame or a 16-bit data frame extended is selected during the I2S
  // configuration phase, the Size parameter means the number of 16-bit data length
  // in the transaction and when a 24-bit data frame or a 32-bit data frame is selected
  // the Size parameter means the number of 16-bit data length.
  ret = HAL_I2SEx_TransmitReceive_DMA(&hi2s2, (uint16_t*)codec_txbuf, (uint16_t*)codec_rxbuf, codec_blocksize*AUDIO_CHANNELS*2);
  ASSERT(ret == HAL_OK, "Failed to start I2S DMA");
}

void Codec::pause(){
  HAL_I2S_DMAPause(&hi2s2);
}

void Codec::resume(){
  HAL_I2S_DMAResume(&hi2s2);
}

extern "C"{
  
  void HAL_I2SEx_TxRxHalfCpltCallback(I2S_HandleTypeDef *hi2s){
    audioCallback(codec_rxbuf, codec_txbuf, codec_blocksize);
  }

  void HAL_I2SEx_TxRxCpltCallback(I2S_HandleTypeDef *hi2s){
    audioCallback(codec_rxbuf+codec_blocksize*2, codec_txbuf+codec_blocksize*2, codec_blocksize);
  }

  void HAL_I2S_ErrorCallback(I2S_HandleTypeDef *hi2s){
    error(CONFIG_ERROR, "I2S Error");
  }
  
}
#endif /* USE_WM8731 */

#if defined USE_CS4271 || defined USE_PCM3168A || defined USE_CS4344

extern "C" {
#if defined MULTI_CODEC && NOF_OUTPUT_CODECS > 0 && NOF_INPUT_CODECS == 0
  #if NOF_OUTPUT_CODECS > 1
  extern SAI_HandleTypeDef HSAI_TX1;
  #endif
  #if NOF_OUTPUT_CODECS > 1
  extern SAI_HandleTypeDef HSAI_TX2;
  #endif
  #if NOF_OUTPUT_CODECS > 2
  extern SAI_HandleTypeDef HSAI_TX3;
  #endif
  #if NOF_OUTPUT_CODECS > 3
  extern SAI_HandleTypeDef HSAI_TX4;
  #endif
  void HAL_SAI_TxHalfCpltCallback(SAI_HandleTypeDef *hsai){    
    if (hsai == &HSAI_TX1) {
      int32_t* srct = codec_txbuf;
      #if NOF_OUTPUT_CODECS > 0
      int32_t* dst1t = codec_tx[0];
      #endif
      #if NOF_OUTPUT_CODECS > 1
      int32_t* dst2t = codec_tx[1];
      #endif
      #if NOF_OUTPUT_CODECS > 2
      int32_t* dst3t = codec_tx[2];
      #endif
        #if NOF_OUTPUT_CODECS > 3
      int32_t* dst4t = codec_tx[3];
      #endif
      int32_t* last = codec_txbuf + sizeof(codec_txbuf) / sizeof(uint32_t);
      while (srct < last) {
        #if NOF_OUTPUT_CODECS > 0
        *dst1t++ = *srct++;
        *dst1t++ = *srct++;
        #endif
        #if NOF_OUTPUT_CODECS > 1
        *dst2t++ = *srct++;
        *dst2t++ = *srct++;
        #endif
        #if NOF_OUTPUT_CODECS > 2
        *dst3t++ = *srct++;
        *dst3t++ = *srct++;
        #endif
        #if NOF_OUTPUT_CODECS > 3
        *dst4t++ = *srct++;
        *dst4t++ = *srct++;
        #endif
      }
      audioCallback(codec_rxbuf, codec_txbuf, codec_blocksize);
    }
  }
  void HAL_SAI_TxCpltCallback(SAI_HandleTypeDef *hsai){
    if (hsai == &HSAI_TX1){
      int32_t* srct = codec_txbuf;
      #if NOF_OUTPUT_CODECS > 0
      int32_t* dst1t = codec_tx[0] + CODEC_BUFFER_SIZE / NOF_OUTPUT_CODECS / 2;
      #endif
      #if NOF_OUTPUT_CODECS > 1
      int32_t* dst2t = codec_tx[1] + CODEC_BUFFER_SIZE / NOF_OUTPUT_CODECS / 2;
      #endif
      #if NOF_OUTPUT_CODECS > 2
      int32_t* dst3t = codec_tx[2] + CODEC_BUFFER_SIZE / NOF_OUTPUT_CODECS / 2;
      #endif
      #if NOF_OUTPUT_CODECS > 3
      int32_t* dst4t = codec_tx[3] + CODEC_BUFFER_SIZE / NOF_OUTPUT_CODECS / 2;
      #endif
      int32_t* last = codec_txbuf + sizeof(codec_txbuf) / sizeof(uint32_t);
      while (srct < last) {
        #if NOF_OUTPUT_CODECS > 0
        *dst1t++ = *srct++;
        *dst1t++ = *srct++;
        #endif
        #if NOF_OUTPUT_CODECS > 1
        *dst2t++ = *srct++;
        *dst2t++ = *srct++;
        #endif
        #if NOF_OUTPUT_CODECS > 2
        *dst3t++ = *srct++;
        *dst3t++ = *srct++;
        #endif
        #if NOF_OUTPUT_CODECS > 3
        *dst4t++ = *srct++;
        *dst4t++ = *srct++;
        #endif
      }
      audioCallback(codec_rxbuf, codec_txbuf, codec_blocksize);
    }
  }
#else
  extern SAI_HandleTypeDef HSAI_RX;
  extern SAI_HandleTypeDef HSAI_TX;
#ifdef USE_CS4271
  void HAL_SAI_RxHalfCpltCallback(SAI_HandleTypeDef *hsai){
    audioCallback(codec_rxbuf, codec_txbuf, codec_blocksize);
  }
  void HAL_SAI_RxCpltCallback(SAI_HandleTypeDef *hsai){
    audioCallback(codec_rxbuf+codec_blocksize*AUDIO_CHANNELS, codec_txbuf+codec_blocksize*AUDIO_CHANNELS, codec_blocksize);
  }
#else // PCM3168A: TX is slave
  void HAL_SAI_TxHalfCpltCallback(SAI_HandleTypeDef *hsai){
    audioCallback(codec_rxbuf, codec_txbuf, codec_blocksize);
  }
  void HAL_SAI_TxCpltCallback(SAI_HandleTypeDef *hsai){
    audioCallback(codec_rxbuf+codec_blocksize*AUDIO_CHANNELS, codec_txbuf+codec_blocksize*AUDIO_CHANNELS, codec_blocksize);
  }
#endif
#endif
  void HAL_SAI_ErrorCallback(SAI_HandleTypeDef *hsai){
    error(CONFIG_ERROR, "SAI DMA Error");
  }
}

void Codec::txrx(){
#ifndef MULTI_CODEC
  HAL_SAI_DMAStop(&HSAI_RX);
  HAL_SAI_Transmit_DMA(&HSAI_RX, (uint8_t*)codec_rxbuf, codec_blocksize*AUDIO_CHANNELS*2);
#endif
}

void Codec::stop(){
#ifdef MULTI_CODEC
  #if NOF_INPUT_CODECS > 0
  HAL_SAI_DMAStop(&HSAI_RX1);
  #endif
  #if NOF_INPUT_CODECS > 1
  HAL_SAI_DMAStop(&HSAI_RX2);
  #endif
  #if NOF_INPUT_CODECS > 2
  HAL_SAI_DMAStop(&HSAI_RX3);
  #endif
  #if NOF_INPUT_CODECS > 3
  HAL_SAI_DMAStop(&HSAI_RX4);
  #endif
  #if NOF_OUTPUT_CODECS > 0
  HAL_SAI_DMAStop(&HSAI_TX1);
  #endif
  #if NOF_OUTPUT_CODECS > 1
  HAL_SAI_DMAStop(&HSAI_TX2);
  #endif
  #if NOF_OUTPUT_CODECS > 2
  HAL_SAI_DMAStop(&HSAI_TX3);
  #endif
  #if NOF_OUTPUT_CODECS > 3
  HAL_SAI_DMAStop(&HSAI_TX4);
  #endif
#else
  HAL_SAI_DMAStop(&HSAI_RX);
  HAL_SAI_DMAStop(&HSAI_TX);
#endif
}

void Codec::start(){
  setOutputGain(settings.audio_output_gain);
  // codec_blocksize = min(CODEC_BUFFER_SIZE/(AUDIO_CHANNELS*2), settings.audio_blocksize);
  codec_blocksize = CODEC_BUFFER_SIZE/(AUDIO_CHANNELS*2);
  HAL_StatusTypeDef ret = HAL_OK;
#ifdef MULTI_CODEC
  #if NOF_INPUT_CODECS > 0
  extern SAI_HandleTypeDef HSAI_RX1;
  #endif
  #if NOF_INPUT_CODECS > 1
  extern SAI_HandleTypeDef HSAI_RX2;
  #endif
  #if NOF_INPUT_CODECS > 2
  extern SAI_HandleTypeDef HSAI_RX3;
  #endif
  #if NOF_INPUT_CODECS > 3
  extern SAI_HandleTypeDef HSAI_RX4;
  #endif
  #if NOF_OUTPUT_CODECS > 0
  extern SAI_HandleTypeDef HSAI_TX1;
  #endif
  #if NOF_OUTPUT_CODECS > 1
  extern SAI_HandleTypeDef HSAI_TX2;
  #endif
  #if NOF_OUTPUT_CODECS > 2
  extern SAI_HandleTypeDef HSAI_TX3;
  #endif
  #if NOF_OUTPUT_CODECS > 3
  extern SAI_HandleTypeDef HSAI_TX4;
  #endif
  #if NOF_INPUT_CODECS > 3
  if (ret == HAL_OK)
    ret = HAL_SAI_Receive_DMA(&HSAI_RX4, (uint8_t*)codec_rx[3], codec_blocksize * 4);
  #endif
  #if NOF_INPUT_CODECS > 2
  if (ret == HAL_OK)
    ret = HAL_SAI_Receive_DMA(&HSAI_RX3, (uint8_t*)codec_rx[2], codec_blocksize * 4);
  #endif
  #if NOF_INPUT_CODECS > 1
  if (ret == HAL_OK)
    ret = HAL_SAI_Receive_DMA(&HSAI_RX2, (uint8_t*)codec_rx[1], codec_blocksize * 4);
  #endif
  #if NOF_INPUT_CODECS > 0
  if (ret == HAL_OK)
    ret = HAL_SAI_Receive_DMA(&HSAI_RX1, (uint8_t*)codec_rx[0], codec_blocksize * 4);
  #endif
  #if NOF_OUTPUT_CODECS > 3
  if (ret == HAL_OK)
    ret = HAL_SAI_Transmit_DMA(&HSAI_TX4, (uint8_t*)codec_tx[3], codec_blocksize * 4);
  #endif
  #if NOF_OUTPUT_CODECS > 2
  if (ret == HAL_OK)
    ret = HAL_SAI_Transmit_DMA(&HSAI_TX3, (uint8_t*)codec_tx[2], codec_blocksize * 4);
  #endif
  #if NOF_OUTPUT_CODECS > 1
  if (ret == HAL_OK)
    ret = HAL_SAI_Transmit_DMA(&HSAI_TX2, (uint8_t*)codec_tx[1], codec_blocksize * 4);
  #endif
  #if NOF_OUTPUT_CODECS > 0
  if (ret == HAL_OK)
    ret = HAL_SAI_Transmit_DMA(&HSAI_TX1, (uint8_t*)codec_tx[0], codec_blocksize * 4);
  #endif
#elif defined USE_CS4271
  ret = HAL_SAI_Receive_DMA(&HSAI_RX, (uint8_t*)codec_rxbuf, codec_blocksize*AUDIO_CHANNELS*2);
  if(ret == HAL_OK)
    ret = HAL_SAI_Transmit_DMA(&HSAI_TX, (uint8_t*)codec_txbuf, codec_blocksize*AUDIO_CHANNELS*2);
#else // PCM3168A
  // start slave first (Noctua)
  ret = HAL_SAI_Transmit_DMA(&HSAI_TX, (uint8_t*)codec_txbuf, codec_blocksize*AUDIO_CHANNELS*2);
  if(ret == HAL_OK)
    ret = HAL_SAI_Receive_DMA(&HSAI_RX, (uint8_t*)codec_rxbuf, codec_blocksize*AUDIO_CHANNELS*2);
#endif
  ASSERT(ret == HAL_OK, "Failed to start SAI DMA");
}

void Codec::pause(){
#ifdef MULTI_CODEC
  #if NOF_INPUT_CODECS > 0
  HAL_SAI_DMAPause(&HSAI_RX1);
  #endif
  #if NOF_INPUT_CODECS > 1
  HAL_SAI_DMAPause(&HSAI_RX2);
  #endif
  #if NOF_INPUT_CODECS > 2
  HAL_SAI_DMAPause(&HSAI_RX3);
  #endif
  #if NOF_INPUT_CODECS > 3
  HAL_SAI_DMAPause(&HSAI_RX4);
  #endif
  #if NOF_OUTPUT_CODECS > 0
  HAL_SAI_DMAPause(&HSAI_TX1);
  #endif
  #if NOF_OUTPUT_CODECS > 1
  HAL_SAI_DMAPause(&HSAI_TX2);
  #endif
  #if NOF_OUTPUT_CODECS > 2
  HAL_SAI_DMAPause(&HSAI_TX3);
  #endif
  #if NOF_OUTPUT_CODECS > 3
  HAL_SAI_DMAPause(&HSAI_TX4);
  #endif
#else
  HAL_SAI_DMAPause(&HSAI_RX);
  HAL_SAI_DMAPause(&HSAI_TX);
#endif
}

void Codec::resume(){
#ifdef MULTI_CODEC
  #if NOF_INPUT_CODECS > 0
  HAL_SAI_DMAResume(&HSAI_RX1);
  #endif
  #if NOF_INPUT_CODECS > 1
  HAL_SAI_DMAResume(&HSAI_RX2);
  #endif
  #if NOF_INPUT_CODECS > 2
  HAL_SAI_DMAResume(&HSAI_RX3);
  #endif
  #if NOF_INPUT_CODECS > 3
  HAL_SAI_DMAResume(&HSAI_RX4);
  #endif
  #if NOF_OUTPUT_CODECS > 0
  HAL_SAI_DMAResume(&HSAI_TX1);
  #endif
  #if NOF_OUTPUT_CODECS > 1
  HAL_SAI_DMAResume(&HSAI_TX2);
  #endif
  #if NOF_OUTPUT_CODECS > 2
  HAL_SAI_DMAResume(&HSAI_TX3);
  #endif
  #if NOF_OUTPUT_CODECS > 3
  HAL_SAI_DMAResume(&HSAI_TX4);
  #endif
#else
  HAL_SAI_DMAResume(&HSAI_RX);
  HAL_SAI_DMAResume(&HSAI_TX);
#endif
}

#endif /* USE_PCM3168A */

