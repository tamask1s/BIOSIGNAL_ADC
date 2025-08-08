#if defined(ARDUINO)

#include <Arduino.h>
#include <SPI.h>
#include "ads1293.h"

#define CONFIG        0x00
#define FLEX_CH1_CN   0x01
#define FLEX_CH2_CN   0x02
#define FLEX_CH3_CN   0x03
#define FLEX_PACE_CN  0x04
#define FLEX_VBAT_CN  0x05

#define LOD_CN        0x06
#define LOD_EN        0x07
#define LOD_CURRENT   0x08
#define LOD_AC_CN     0x09

#define CMDET_EN      0x0a
#define CMDET_CN      0x0b
#define RLD_CN        0x0c

#define WILSON_EN1    0x0d
#define WILSON_EN2    0x0e
#define WILSON_EN3    0x0f
#define WILSON_CN     0x10

#define REF_CN        0x11
#define OSC_CN        0x12

#define AFE_RES       0x13
#define AFE_SHDN_CN   0x14
#define AFE_FAULT_CN  0x15
#define AFE_PACE_CN   0x17

#define ERROR_LOD     0x18
#define ERROR_STATUS  0x19
#define ERROR_RANGE1  0x1a
#define ERROR_RANGE2  0x1b
#define ERROR_RANGE3  0x1c
#define ERROR_SYNC    0x1d
#define ERROR_MISC    0x1e

#define DIGO_STRENGTH 0x1f
#define R2_RATE       0x21
#define R3_RATE_CH1   0x22
#define R3_RATE_CH2   0x23
#define R3_RATE_CH3   0x24
#define R1_RATE       0x25
#define DIS_EFILTER   0x26
#define DRDYB_SRC     0x27
#define SYNCB_CN      0x28
#define MASK_DRDYB    0x29
#define MASK_ERB      0x2a
#define ALARM_FILTER  0x2e
#define CH_CNFG       0x2f
#define DATA_STATUS   0x30
#define DATA_CH1_PACE 0x31
#define DATA_CH2_PACE 0x33
#define DATA_CH3_PACE 0x35
#define DATA_CH1_ECG  0x37
#define DATA_CH2_ECG  0x3a
#define DATA_CH3_ECG  0x3d
#define REVID         0x40
#define DATA_LOOP     0x50

static inline int32_t convert_channel_data(uint8_t* data)
{
    uint32_t tmp = ((uint32_t)data[0] << 16) | ((uint32_t)data[1] << 8) | data[2];
    tmp <<= 8;
    tmp = (int32_t)tmp;
    tmp >>= 8;
    return tmp;
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

#else /// ARDUINO

#include <inttypes.h>
#include <vector>
#include <cmath>
#include "ads1293.h"

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

#endif /// ARDUINO
