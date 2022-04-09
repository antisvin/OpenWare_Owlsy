#include "ili9341.h"
#include "device.h"
#include "errorhandlers.h"

static SPI_HandleTypeDef* OLED_SPIInst;

uint16_t windowLastX = 9999;

uint8_t dummyDataOut[] = { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255 };
uint8_t dummyDataIn[16];

static void ILI9341_Reset() {
    HAL_GPIO_WritePin(OLED_RST_GPIO_Port, OLED_RST_Pin, GPIO_PIN_RESET);
    ILI9341_Unselect();
    HAL_Delay(40);
    ILI9341_Select();
    HAL_GPIO_WritePin(OLED_RST_GPIO_Port, OLED_RST_Pin, GPIO_PIN_SET);
    windowLastX = 9999;
}

static HAL_StatusTypeDef ILI9341_WriteCommand(uint8_t cmd) {
    clearPin(OLED_DC_GPIO_Port, OLED_DC_Pin);
    return HAL_SPI_TransmitReceive(OLED_SPIInst, &cmd, dummyDataIn, 1, HAL_MAX_DELAY);
}

static HAL_StatusTypeDef ILI9341_WriteData(uint8_t* buff, size_t buff_size) {
    setPin(OLED_DC_GPIO_Port, OLED_DC_Pin);
    return HAL_SPI_TransmitReceive(
        OLED_SPIInst, buff, dummyDataIn, buff_size, HAL_MAX_DELAY);
}

HAL_StatusTypeDef ILI9341_ReadPowerMode(uint8_t buff[2]) {
    static uint8_t readPowerModeCommand[2] = { 0x0A, 0xff };

    ILI9341_Select();
    // We send a command
    clearPin(OLED_DC_GPIO_Port, OLED_DC_Pin);
    HAL_StatusTypeDef ret = HAL_SPI_TransmitReceive(
        OLED_SPIInst, readPowerModeCommand, buff, 2, HAL_MAX_DELAY);
    ILI9341_Unselect();
    return ret;
}

HAL_StatusTypeDef ILI9341_GetScanline(uint8_t buff[2]) {
    static uint8_t getScanlineCommand[3] = { 0x45, 0x00, 0x00};
    uint8_t getScanlineArgs[] = { 0x00, 0x00};

    ILI9341_Select();
    // We send a command
    clearPin(OLED_DC_GPIO_Port, OLED_DC_Pin);
    uint8_t dummy;
    HAL_SPI_Transmit(OLED_SPIInst, getScanlineCommand, 1, HAL_MAX_DELAY);
    for(int i = 0; i < 5000; i++)  __asm("NOP");
    //HAL_SPI_Receive(OLED_SPIInst, &dummy, 1, HAL_MAX_DELAY);
    HAL_StatusTypeDef ret = HAL_SPI_Receive(
        OLED_SPIInst, buff, 2, HAL_MAX_DELAY);
    ILI9341_Unselect();
    return ret;
}

HAL_StatusTypeDef ILI9341_ReadDisplayStatus(uint8_t buff[2]) {
    static uint8_t readPowerModeCommand[5] = { 0x09, 0xFF, 0xFF, 0xFF, 0xFF };

    ILI9341_Select();
    // We send a command
    clearPin(OLED_DC_GPIO_Port, OLED_DC_Pin);
    HAL_StatusTypeDef ret = HAL_SPI_TransmitReceive(
        OLED_SPIInst, readPowerModeCommand, buff, 5, HAL_MAX_DELAY);
    ILI9341_Unselect();
    return ret;
}

HAL_StatusTypeDef ILI9341_ReadPixelFormat(uint8_t buff[3]) {
    static uint8_t readPixelFormatCommand[3] = { 0x0C, 0xFF, 0xFF };

    ILI9341_Select();
    // We send a command
    clearPin(OLED_DC_GPIO_Port, OLED_DC_Pin);
    HAL_StatusTypeDef ret = HAL_SPI_TransmitReceive(
        OLED_SPIInst, readPixelFormatCommand, buff, 3, HAL_MAX_DELAY);
    ILI9341_Unselect();
    return ret;
}

HAL_StatusTypeDef ILI9341_SetAddressWindow(uint16_t y0, uint16_t y1) {
    static uint8_t dataX[] = { 0, 0, 0, 239 };
    // column address set
    if (true) {
        //  if (windowLastX != 0) {
        windowLastX = 0;
        if (ILI9341_WriteCommand(0x2A) != HAL_OK) {
            return HAL_ERROR;
        }
        if (ILI9341_WriteData(dataX, 4) != HAL_OK) {
            return HAL_ERROR;
        }
    }
    // row address set
    // Optimization removed to fix a button refresh problem
    if (ILI9341_WriteCommand(0x2B) != HAL_OK) {
        return HAL_ERROR;
    }
    uint8_t dataY[] = { (y0 >> 8) & 0xFF, y0 & 0xFF, (y1 >> 8) & 0xFF, y1 & 0xFF };
    if (ILI9341_WriteData(dataY, 4) != HAL_OK) {
        return HAL_ERROR;
    }
    // write to RAM
    return ILI9341_WriteCommand(0x2C); // RAMWR
}

void ILI9341_Init() {
    // INIT ILI9341
    ILI9341_Select();
    ILI9341_Reset();

    // command list is based on https://github.com/martnak/STM32-ILI9341

    // SOFTWARE RESET
    ILI9341_WriteCommand(0x01);
    ILI9341_Unselect();
    HAL_Delay(50);
    ILI9341_Select();

    // POWER CONTROL A
    ILI9341_WriteCommand(0xCB);
    {
        uint8_t data[] = { 0x39, 0x2C, 0x00, 0x34, 0x02 };
        ILI9341_WriteData(data, sizeof(data));
    }

    // POWER CONTROL B
    ILI9341_WriteCommand(0xCF);
    {
        uint8_t data[] = { 0x00, 0xC1, 0x30 };
        ILI9341_WriteData(data, sizeof(data));
    }

    // DRIVER TIMING CONTROL A
    ILI9341_WriteCommand(0xE8);
    {
        uint8_t data[] = { 0x85, 0x00, 0x78 };
        ILI9341_WriteData(data, sizeof(data));
    }

    // DRIVER TIMING CONTROL B
    ILI9341_WriteCommand(0xEA);
    {
        uint8_t data[] = { 0x00, 0x00 };
        ILI9341_WriteData(data, sizeof(data));
    }

    // POWER ON SEQUENCE CONTROL
    ILI9341_WriteCommand(0xED);
    {
        uint8_t data[] = { 0x64, 0x03, 0x12, 0x81 };
        ILI9341_WriteData(data, sizeof(data));
    }

    // PUMP RATIO CONTROL
    ILI9341_WriteCommand(0xF7);
    {
        uint8_t data[] = { 0x20 };
        ILI9341_WriteData(data, sizeof(data));
    }

    // POWER CONTROL,VRH[5:0]
    ILI9341_WriteCommand(0xC0);
    {
        uint8_t data[] = { 0x23 };
        ILI9341_WriteData(data, sizeof(data));
    }

    // POWER CONTROL,SAP[2:0];BT[3:0]
    ILI9341_WriteCommand(0xC1);
    {
        uint8_t data[] = { 0x10 };
        ILI9341_WriteData(data, sizeof(data));
    }

    // VCM CONTROL
    ILI9341_WriteCommand(0xC5);
    {
        uint8_t data[] = { 0x3E, 0x28 };
        ILI9341_WriteData(data, sizeof(data));
    }
    HAL_Delay(50);

    // VCM CONTROL 2
    ILI9341_WriteCommand(0xC7);
    {
        uint8_t data[] = { 0x86 };
        ILI9341_WriteData(data, sizeof(data));
    }
    HAL_Delay(50);

    // MADCTL
    ILI9341_WriteCommand(0x36);
    {
        uint8_t data[] = { ILI9341_MADCTL_MX | ILI9341_MADCTL_RGB };
        ILI9341_WriteData(data, sizeof(data));
    }

    // FRAME RATE CONTROL
    ILI9341_WriteCommand(0xB3);
    {
        uint8_t data[] = { 0x00, 0x13}; // 100 Hz refresh rate
        ILI9341_WriteData(data, sizeof(data));
    }


    /*
    // INTERFACE CONTROL
    ILI9341_WriteCommand(0xF6);
    {
      uint8_t data[] = {0x00, 0x00, 0x20};
      ILI9341_WriteData(data, sizeof(data));
    }
    */

    // PIXEL FORMAT
    ILI9341_WriteCommand(0x3A);
    {
        uint8_t data[] = { 0x55 };
        ILI9341_WriteData(data, sizeof(data));
    }
    HAL_Delay(50);

    // FRAME RATIO CONTROL, STANDARD RGB COLOR
    ILI9341_WriteCommand(0xB1);
    {
        uint8_t data[] = { 0x00, 0x13 };
        //0x18 };
        ILI9341_WriteData(data, sizeof(data));
    }

    // DISPLAY FUNCTION CONTROL
    ILI9341_WriteCommand(0xB6);
    {
        uint8_t data[] = { 0x08, 0x82, 0x27 };
        ILI9341_WriteData(data, sizeof(data));
    }

    // 3GAMMA FUNCTION DISABLE
    ILI9341_WriteCommand(0xF2);
    {
        uint8_t data[] = { 0x00 };
        ILI9341_WriteData(data, sizeof(data));
    }

    // GAMMA CURVE SELECTED
    ILI9341_WriteCommand(0x26);
    {
        uint8_t data[] = { 0x01 };
        ILI9341_WriteData(data, sizeof(data));
    }

    // POSITIVE GAMMA CORRECTION
    ILI9341_WriteCommand(0xE0);
    {
        uint8_t data[] = { 0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37,
            0x07, 0x10, 0x03, 0x0E, 0x09, 0x00 };
        ILI9341_WriteData(data, sizeof(data));
    }

    // NEGATIVE GAMMA CORRECTION
    ILI9341_WriteCommand(0xE1);
    {
        uint8_t data[] = { 0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48,
            0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F };
        ILI9341_WriteData(data, sizeof(data));
    }

    // EXIT SLEEP
    ILI9341_WriteCommand(0x11);
    // ILI9341_Unselect();
    HAL_Delay(120);
    // ILI9341_Select();

    // TURN ON DISPLAY
    ILI9341_WriteCommand(0x29);
    // ILI9341_Unselect();
    HAL_Delay(50);
}

void oled_init(SPI_HandleTypeDef* spi) {
    OLED_SPIInst = spi;
    ILI9341_Init();
}

#define TFT_NUMBER_OF_PARTS 6
uint32_t tftDirtyBits = 0;
uint8_t tftPart = 0;
bool pushToTftInProgress = false;
static uint16_t areaHeight[TFT_NUMBER_OF_PARTS] = { 16, 10, 10, 214, 30, 40 };
static uint16_t screenY[TFT_NUMBER_OF_PARTS] = { 0, 16, 26, 36, 250, 280 };
//static uint16_t areaY[TFT_NUMBER_OF_PARTS] = { 0, 0, 0, 0, 0, 0, 0 };
static const uint8_t* last_data;

void oled_write(const uint8_t* data, uint32_t length) {
    last_data = data;
#if 0
    // Not sure if this works as intended, copied from PFM as is
    uint8_t status_[4];
    status_[1] = 1;
    if (ILI9341_ReadPowerMode(status_) != HAL_OK) {
        status_[1] = 1;
    }
    // Try to detect when TFT is BLANK
    if (status_[1] != 222 && status_[1] != 156) {
        if (status_[3] == status_[1]) {
            ILI9341_Init();
            // tftDirtyBits = (1 << TFT_NUMBER_OF_PARTS) - 1;
            // force redraw?
        }
    }
    // status_2 allows to make sure we got 2 wrong number in a row
    status_[3] = status_[1];
#endif

    int p;
    for (p = 0; p < TFT_NUMBER_OF_PARTS; p++) {
        if ((tftDirtyBits & (1UL << tftPart)) > 0) {
            uint16_t height = areaHeight[tftPart];
            uint16_t screen_y = screenY[tftPart];
            //uint16_t y = areaY[tftPart];

            //static uint8_t scanline_buf[2] = {0, 0};
            //ILI9341_GetScanline(scanline_buf);

            // Update TFT part
            ILI9341_Select();

            if (ILI9341_SetAddressWindow(screen_y, screen_y + height) == HAL_OK) {
                pushToTftInProgress = true;

#ifdef USE_DMA2D
                #include "stm32h7xx_hal.h"
                extern DMA2D_HandleTypeDef hdma2d;
                while( (hdma2d.Instance->CR & DMA2D_CR_START) != 0) {}
#endif

                setPin(OLED_DC_GPIO_Port, OLED_DC_Pin);
                SPI1->CFG2 |= SPI_CFG2_LSBFRST;
#ifdef OLED_DMA
                OLED_SPIInst->Instance->CFG1 |= SPI_CFG1_DSIZE_3;
                OLED_SPIInst->Init.DataSize = SPI_DATASIZE_16BIT;
                if (HAL_OK ==
                    HAL_SPI_Transmit_DMA(OLED_SPIInst,
                        (uint8_t*)data, height * 240)) {
                    tftDirtyBits &= ~(1UL << tftPart);
                }
                else {
                    SPI1->CFG2 &= ~SPI_CFG2_LSBFRST;
                    pushToTftInProgress = false;
                }
#else
                if (HAL_OK ==
                    HAL_SPI_Transmit(OLED_SPIInst,
                        (uint8_t*)data, height * 240 * 2, 1000)) {
                    tftDirtyBits &= ~(1UL << tftPart);
                }
                SPI1->CFG2 &= ~SPI_CFG2_LSBFRST;
                pushToTftInProgress = false;
#endif
            }
            tftPart = (tftPart + 1) % TFT_NUMBER_OF_PARTS;
            return;
        }
        tftPart = (tftPart + 1) % TFT_NUMBER_OF_PARTS;
    }
}

// SPI callback
#ifdef OLED_DMA
void HAL_SPI_ErrorCallback(SPI_HandleTypeDef* hspi) {
    if (hspi->ErrorCode == HAL_SPI_ERROR_OVR) {
        __HAL_SPI_CLEAR_OVRFLAG(hspi);
        error(RUNTIME_ERROR, "SPI OVR");
    }
    else {
        assert_param(0);
    }
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef* hspi) {
    if (hspi == OLED_SPIInst) {
        // MSB order
        SPI1->CFG2 &= ~SPI_CFG2_LSBFRST;
        // 8 bit transfer
        OLED_SPIInst->Instance->CFG1 &= ~SPI_CFG1_DSIZE_3;
        OLED_SPIInst->Init.DataSize = SPI_DATASIZE_8BIT;
        ILI9341_Unselect();
        pushToTftInProgress = false;
    }
}

#endif
