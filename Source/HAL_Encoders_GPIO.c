#include "stm32_arch_hal.h"
#include "HAL_Encoders.h"
#include "main.h"

#include <string.h>

static int16_t rgENC_Values[1] = {0};

//static uint16_t NOP_CNT = 250; // doesn't work in Release build

//_____ Functions _____________________________________________________________________________________________________
// Port and Chip Setup
void Encoders_readAll(void)
{ 
    /*
	volatile uint16_t x  = NOP_CNT;
	
	pbarCS(0);
	while(--x){__NOP();} // TODO: microsecond delay using CYCCNT
	// *** The minimum NOP delay for proper operation seems to be 150 ***
	HAL_SPI_Receive(Encoders_SPIConfig, (uint8_t*)rgENC_Values, 14, 100);
	pbarCS(1);
    */
}

void Encoders_readSwitches(void)
{ 
    /*
	volatile uint16_t x  = NOP_CNT;
	
	pbarCS(0);
	while(--x){__NOP();}
	// *** The minimum NOP delay for proper operation seems to be 150 ***
	HAL_SPI_Receive(Encoders_SPIConfig, (uint8_t*)rgENC_Values, 2, 100);
	pbarCS(1);
    */
}

int16_t* Encoders_get()
{
  return rgENC_Values;
}

//_____ Initialisaion _________________________________________________________________________________________________
void Encoders_init (GPIO_TypeDef *port_a, uint32_t pin_a, GPIO_TypeDef *port_b,
                    uint32_t pin_b, GPIO_TypeDef *port_click, uint32_t pin_click)
{

}
