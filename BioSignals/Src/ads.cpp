#include "ads.h"
// #include "spi.h"
#include "device.h"
// #include "gpio.h"
// #include "delay.h"
#include "ads1298.h"
#include "main.h"


#define ADS_CS_LO()	HAL_GPIO_WritePin(ADC_NCS_GPIO_Port, ADC_NCS_Pin, GPIO_PIN_RESET);
#define ADS_CS_HI()	HAL_GPIO_WritePin(ADC_NCS_GPIO_Port, ADC_NCS_Pin, GPIO_PIN_SET);
#define ADS_RESET_LO()	HAL_GPIO_WritePin(ADC_RESET_GPIO_Port, ADC_RESET_Pin, GPIO_PIN_RESET);
#define ADS_RESET_HI()	HAL_GPIO_WritePin(ADC_RESET_GPIO_Port, ADC_RESET_Pin, GPIO_PIN_SET);

#define ADS_HSPI hspi1
#define ADS_SPI_TIMEOUT 800
#define MAX_CHANNELS 4

uint32_t ads_status = 0;
int ads_timestamp;
int ads_maxChannels = 0; //maximum number of channels supported by ads129n = 4,6,8
int ads_gIDval = 0; //Device ID : lower 5 bits of  ID Control Register 
volatile bool ads_continuous = false;
// volatile bool doFilter = true;
// volatile bool pretty = false;

// #include "stm32f4xx_ll_iwdg.h"
// #define KICK_WDT() LL_IWDG_ReloadCounter()
// #define SYSTEM_US_TICKS		(SystemCoreClock / 1000000)//cycles per microsecon
#define SYSTEM_MS_TICKS		(SystemCoreClock / 1000) // cycles per millisecond

void delay_us(uint32_t uSec){
  uSec += 10;
  while(uSec--)
    asm("NOP");
  // volatile uint32_t DWT_START = DWT->CYCCNT;
  // // keep DWT_TOTAL from overflowing (max 59.652323s w/72MHz SystemCoreClock)
  // if(uSec > (UINT32_MAX / SYSTEM_US_TICKS))
  //   uSec = (UINT32_MAX / SYSTEM_US_TICKS);
  // volatile uint32_t DWT_TOTAL = (SYSTEM_US_TICKS * uSec);
  // while((DWT->CYCCNT - DWT_START) < DWT_TOTAL){
  //   KICK_WDT();
  // }
}

int32_t ads_samples[MAX_CHANNELS];

uint8_t spi_transfer(uint8_t tx){
  extern SPI_HandleTypeDef ADS_HSPI;
  uint8_t rx;
  HAL_SPI_TransmitReceive(&ADS_HSPI, &tx, &rx, 1, ADS_SPI_TIMEOUT);
  return rx;
}

void spi_cs(bool high){
  if(high){ // chip disable
    ADS_CS_HI();
  }else{
    ADS_CS_LO();
  }
}

void setSTART(bool high){
  HAL_GPIO_WritePin(ADC_START_GPIO_Port, ADC_START_Pin, high ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

bool isDRDY(){
  return HAL_GPIO_ReadPin(ADC_DRDY_GPIO_Port, ADC_DRDY_Pin);
}

void rldConfig(){
  // For Right Leg Drive: Power up the internal reference and wait for it to settle
  // ads_write_reg(ADS1298::CONFIG3, ADS1298::RLDREF_INT | ADS1298::PD_RLD | ADS1298::PD_REFBUF | ADS1298::VREF_4V | ADS1298::CONFIG3_const);
  ads_write_reg(ADS1298::CONFIG3, ADS1298::PD_REFBUF | ADS1298::CONFIG3_const | ADS1298::RLDREF_INT | ADS1298::PD_RLD); // use default 2.4v reference with buffer enabled
  HAL_Delay(150);
  ads_write_reg(ADS1298::RLD_SENSP, 0x01); // only use channel IN1P and IN1N
  ads_write_reg(ADS1298::RLD_SENSN, 0x01); // for the RLD Measurement
}

void defaultConfig(){
  // default settings for ADS1298 and compatible chips
  // delay_ms(1000); // pause to provide ads129n enough time to boot up...
  // Send SDATAC Command (Stop Read Data Continuously mode)
  ads_send_command(ADS1298::SDATAC);
  delay_us(2);

  // All GPIO set to output 0x0000: (floating CMOS inputs can flicker on and off, creating noise)
  ads_write_reg(ADS1298::GPIO, 0);
  // set voltage reference
  ads_write_reg(ADS1298::CONFIG3, ADS1298::PD_REFBUF | ADS1298::CONFIG3_const);
  // set sample rate
  ads_write_reg(ADS1298::CONFIG1, ADS1298::HIGH_RES_500_SPS | ADS1298::CONFIG1_const);

  for(int i = 0; i < MAX_CHANNELS; ++i) {
    ads_write_reg(ADS1298::CH1SET + i, ADS1298::ELECTRODE_INPUT | ADS1298::GAIN_12X); //report this channel with x12 gain
    //ads_write_reg(ADS1298::CH1SET + i, ADS1298::SHORTED); //disable this channel
  }
  // When using the START opcode to begin conversions, hold the START pin low.
  setSTART(false);
}

void setGain(int gain){
  int value = ADS1298::ELECTRODE_INPUT | gain;
  for(int i = 0; i < MAX_CHANNELS; ++i)
    ads_write_reg(ADS1298::CH1SET + i, value);
}

void startContinuous(){
  ads_send_command(ADS1298::RDATAC);
  ads_send_command(ADS1298::START);
  ads_continuous = true;
  // setLed(0, BLUE_COLOUR);
}
void stopContinuous(){
  ads_send_command(ADS1298::STOP);
  ads_send_command(ADS1298::SDATAC);
  ads_continuous = false;
  // setLed(0, NO_COLOUR);
}

void ads_drdy(){
    // toggleLed();
    if(ads_continuous){
      ads_sample(ads_samples, MAX_CHANNELS);
      // if(doFilter)
      // 	filter();
      // if(pretty)
      // 	sendSerial();
      // else
      // 	sendSarcduino();
    }    
}

void ads_setup(){
  ADS_RESET_LO();
  HAL_Delay(10);
  ADS_RESET_HI();
  spi_cs(true);

  HAL_Delay(500); //wait for the ads129n to be ready - it can take a while to charge caps
  ads_send_command(ADS1298::SDATAC); // Send SDATAC Command (Stop Read Data Continuously mode)
  HAL_Delay(10);
  // Determine model number and number of channels available
  ads_gIDval = ads_read_reg(ADS1298::ID); //lower 5 bits of register 0 reveal chip type
  switch(ads_gIDval & 0x3f  ) { //least significant bits reports channels
  case 16:
    ads_maxChannels = 4; //ads1294
    break;
  case 17:
    ads_maxChannels = 6; //ads1296
    break; 
  case 18:
    ads_maxChannels = 8; //ads1298
    break;
  case 30:
    ads_maxChannels = 8; //ads1299
    break;
  default: 
    ads_maxChannels = 0;
  }

  // default config:
  // Continuous sampling: Off
  // Sampling rate: 500sps
  // Drift filter: On
  // Right leg drive: On
  // Gain: 12x
  defaultConfig();
  rldConfig();
  // 500sps
  ads_write_reg(ADS1298::CONFIG1, ADS1298::DAISY_EN | ADS1298::CLK_EN | ADS1298::HIGH_RES_500_SPS | ADS1298::CONFIG1_const);
  stopContinuous();
  ads_status = isDRDY();
  startContinuous();
}

int ads_read_single_sample(){
  // the ADS129x outputs 24 bits of data per channel in binary twos complement format, MSB first.
  // convert 24bit to 32bit signed
  return (spi_transfer(0) << 24) | (spi_transfer(0) << 16) | (spi_transfer(0) << 8);
}

void ads_sample(int32_t* samples, size_t len){
  spi_cs(false); // chip enable
  // 24-bits status header plus 24 bits per channel
  ads_status = (uint32_t)ads_read_single_sample()>>8;
  ads_timestamp = (int)(DWT->CYCCNT / SYSTEM_MS_TICKS);
  for(size_t i=0; i<len; ++i)
    samples[i] = ads_read_single_sample();
  spi_cs(true); // chip disable
}

void ads_send_command(int cmd){
  spi_cs(false); // chip enable
  spi_transfer(cmd);
  delay_us(1);
  spi_cs(true); // chip disable
}

void ads_write_reg(int reg, int val){
  //see pages 40,43 of datasheet - 
  spi_cs(false); // chip enable
  spi_transfer(ADS1298::WREG | reg);
  spi_transfer(0);	// number of registers to be read/written – 1
  spi_transfer(val);
  delay_us(1);
  spi_cs(true); // chip disable
}

int ads_read_reg(int reg){
  int out = 0;
  spi_cs(false); // chip enable
  spi_transfer(ADS1298::RREG | reg);
  delay_us(5);
  spi_transfer(0);	// number of registers to be read/written – 1
  delay_us(5);
  out = spi_transfer(0);
  delay_us(1);
  spi_cs(true); // chip disable
  return(out);
}
