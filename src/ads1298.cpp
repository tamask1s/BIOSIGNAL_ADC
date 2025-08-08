#include <Arduino.h>
#include <SPI.h>
#include "ads1298.h"
#ifdef VALAMI

ads1298::ads1298(uint8_t sck, uint8_t miso, uint8_t mosi, uint8_t ss, uint8_t drdy, uint8_t chip_select, uint8_t start, uint8_t pwdn,uint8_t adc_reset,uint8_t clk_sel)
    : pin_cs_(chip_select),
      pin_drdy_(drdy)
{
    SPI.begin(sck, miso, mosi, ss);
    pinMode(pin_drdy_, INPUT_PULLUP);
    pinMode(pin_cs_, OUTPUT);
    pinMode(start,OUTPUT);
    pinMode(pwdn,OUTPUT);
    pinMode(adc_reset,OUTPUT);
    pinMode(clk_sel,OUTPUT);

}

void ads1298::init_adc()
{
    write_reg(FLEX_CH1_CN, 0b000010001); /// channel 1 INA analog inputs are connected to input pins 2 and 1
    write_reg(FLEX_CH2_CN, 0b000100011); /// channel 2 INA analog inputs are connected to input pins 4 and 3
    write_reg(FLEX_CH3_CN, 0b000110101); /// channel 3 INA analog inputs are connected to input pins 6 and 5
    write_reg(CMDET_EN, 0b00111111); /// all inputs are used for common point calculation
    ///write_reg(RLD_CN, 0b00000100); /// this would be the way how the RLD OUT would be connected to input 3. We can't afford it as we have already used all input pins to analog inputs for the INAs
    ///write_reg(WILSON_EN1, 0b00000001); write_reg(WILSON_EN2, 0b00000010); write_reg(WILSON_EN3, 0b00000011); write_reg(WILSON_CN, 0b00000001); /// some wilson things, I didnt get into it as we don't use it
    write_reg(OSC_CN, 0b00000100); /// use external clock driven by 409kHz xtal
    write_reg(AFE_SHDN_CN, 0); /// none of the inputs are connected to VDD
    write_reg(AFE_RES, 0b00111111); /// all channels are running on higher frequency (204kHz), and high resolution
    write_reg(R1_RATE, 0b00000010); /// decimation rates needs to be checked in the docs
    write_reg(R2_RATE, 0b00000010);
    write_reg(R3_RATE_CH1, 0b00000100);
    write_reg(R3_RATE_CH2, 0b00000100);
    write_reg(R3_RATE_CH3, 0b00000100);
    write_reg(DRDYB_SRC, 0b00001000); /// DRDY is set to ecg channel 1
    write_reg(CH_CNFG, 0b01110000); /// setup which channles shall be read on loop read mode. currently set to all 3 ecg channels, excluding pace channels
    write_reg(CONFIG, 0b00000001); /// start conversion, synchronize filters
}



void ads1298::write_reg(uint8_t addr, uint8_t data)
{
    digitalWrite(pin_cs_, LOW);
    SPI.transfer(addr & 0x7f);
    SPI.transfer(data);
    digitalWrite(pin_cs_, HIGH);
    delay(1);
}

uint8_t ads1298::read_reg(uint8_t addr)
{
    uint8_t rdData;
    digitalWrite(pin_cs_, LOW);
    SPI.transfer(addr  | 0x80);
    rdData = SPI.transfer(0);
    digitalWrite(pin_cs_, HIGH);
    return (rdData);
}

bool ads1298::is_data_ready()
{
    return !digitalRead(pin_drdy_);
}
void ads1298::read_data_stream(uint8_t* data, int length)
{
    digitalWrite(pin_cs_, LOW);
    SPI.transfer(DATA_LOOP | 0x80);
    for (int i = 0; i < length; i++)
        data[i] = SPI.transfer(0xff);
    digitalWrite(pin_cs_, HIGH);
}
#endif