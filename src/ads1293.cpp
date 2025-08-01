#if defined(ARDUINO)

#include <Arduino.h>
#include <SPI.h>
#include "ads1293.h"

static inline int32_t convert_channel_data(uint8_t* data)
{
    uint32_t tmp = ((uint32_t)data[0] << 16) | ((uint32_t)data[1] << 8) | data[2];
    tmp <<= 8;
    tmp = (int32_t)tmp;
    tmp >>= 8;
    return tmp;
}

int32_t ads1293::get_channel_data(uint8_t channel_indx)
{
    uint8_t tmp[3];
    channel_indx *= 3;
    tmp[0] = read_reg(DATA_CH1_ECG + channel_indx);
    tmp[1] = read_reg(DATA_CH1_ECG + channel_indx + 1);
    tmp[2] = read_reg(DATA_CH1_ECG + channel_indx + 2);
    return convert_channel_data(tmp);
}

void ads1293::get_channel_data_3chan(int32_t& ch_1, int32_t& ch_2, int32_t& ch_3)
{
    uint8_t tmp[9];
    read_data_stream(tmp, 9);
    ch_1 = convert_channel_data(tmp);
    ch_2 = convert_channel_data(tmp + 3);
    ch_3 = convert_channel_data(tmp + 6);
}

void ads1293::read_data_stream(uint8_t* data, int length)
{
    digitalWrite(pin_cs_, LOW);
    SPI.transfer(DATA_LOOP | 0x80);
    for (int i = 0; i < length; i++)
        data[i] = SPI.transfer(0xff);
    digitalWrite(pin_cs_, HIGH);
}

ads1293::ads1293(uint8_t sck, uint8_t miso, uint8_t mosi, uint8_t ss, uint8_t drdy, uint8_t chip_select)
    : pin_cs_(chip_select),
      pin_drdy_(drdy)
{
    SPI.begin(sck, miso, mosi, ss);
    pinMode(pin_drdy_, INPUT_PULLUP);
    pinMode(pin_cs_, OUTPUT);
}

void ads1293::init_adc()
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

void ads1293::write_reg(uint8_t addr, uint8_t data)
{
    digitalWrite(pin_cs_, LOW);
    SPI.transfer(addr & 0x7f);
    SPI.transfer(data);
    digitalWrite(pin_cs_, HIGH);
    delay(1);
}

uint8_t ads1293::read_reg(uint8_t addr)
{
    uint8_t rdData;
    digitalWrite(pin_cs_, LOW);
    SPI.transfer(addr  | 0x80);
    rdData = SPI.transfer(0);
    digitalWrite(pin_cs_, HIGH);
    return (rdData);
}

bool ads1293::is_data_ready()
{
    return !digitalRead(pin_drdy_);
}

#else /// ARDUINO

#include <inttypes.h>
#include <vector>
#include <cmath>
#include "ads1293.h"

int32_t ads1293::get_channel_data(uint8_t channel_indx)
{
    return 0;
}

void ads1293::get_channel_data_3chan(int32_t& ch_1, int32_t& ch_2, int32_t& ch_3)
{}

float const ch_coef_ = 0.05;
float const digital_range_ = 8388608.0; /// 23 bits (2^23, half of the 24 bit range)
std::vector<float> sineStates_ = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

void ads1293::read_data_stream(uint8_t* data, int length)
{
    int buffit = 0;
    for (uint16_t i = 0; i < 3/**NRCHANNELS*/; ++i)
    {
        int lMapping = i; /// = physicalMapping[i];
        float chCoef = (lMapping + 1.0) * ch_coef_;
        sineStates_[i] = sineStates_[i] + 0.77;
        if (sineStates_[i] > 1000000)
            sineStates_[i] = 0;
        uint32_t num = sin((sineStates_[i]) * chCoef) * digital_range_ + digital_range_;
        data[buffit * 3] = (num & 0x00ff0000) >> 16;
        data[buffit * 3 + 1] = (num & 0x0000ff00) >> 8;
        data[buffit * 3 + 2] = num & 0x000000ff;
        buffit++;
    }
}

ads1293::ads1293(uint8_t sck, uint8_t miso, uint8_t mosi, uint8_t ss, uint8_t drdy, uint8_t chip_select)
    : pin_cs_(chip_select),
      pin_drdy_(drdy)
{}

void ads1293::init_adc()
{}

void ads1293::write_reg(uint8_t addr, uint8_t data)
{}

uint8_t ads1293::read_reg(uint8_t addr)
{
    uint8_t rdData = 0;
    return (rdData);
}

bool ads1293::is_data_ready()
{
    return true;
}

#endif /// ARDUINO
