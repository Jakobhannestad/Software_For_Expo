#ifndef PIN_H
#define PIN_H

namespace Pin 
{

    //**        ADC PINS            **//
    const uint8_t MC_Vref = 1;
    const uint8_t ENK_Vgmr = 2;
    const uint8_t ENK_SinN = 4;
    const uint8_t ENK_SinP = 5;
    const uint8_t ENK_CosP = 6;
    const uint8_t ENK_CosN = 7;
    const uint8_t MC_SOA = 8;
    const uint8_t MC_SOB = 9;
    const uint8_t MC_SOC = 10;


    //**        PWM PINS            **//    
    const uint8_t PWM_LC = 11;
    const uint8_t PWM_HC = 12;
    const uint8_t PWM_LB = 13;
    const uint8_t PWM_HB = 14;
    const uint8_t PWM_LA = 15;
    const uint8_t PWM_HA = 16;


    //**        I2C PINS            **//    
    const uint8_t I2C_SDA = 17;
    const uint8_t I2C_SCL = 18;


    //**        D PINS              **//    
    const uint8_t DPluss = 19;
    const uint8_t DMinus = 20;


    //**        SPI/FLAG PINS       **//
    const uint8_t PDA_RST = 21;
    const uint8_t SPI_nSCS = 33;
    const uint8_t SPI_SCLK = 34;
    const uint8_t SPI_SDI = 35;
    const uint8_t SPI_SDO = 36;
    const uint8_t MC_nSLEEP = 37;
    const uint8_t MC_nFAULT = 38;
    const uint8_t MC_DRVOFF = 39;
    
}

#endif 
