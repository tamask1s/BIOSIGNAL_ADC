#include <Arduino.h>
#include <SPI.h>
#include "ads1298.h"

///////// ADS1298 REGISTER MAP ///////////

#define ID_REG          0x00
#define CONFIG1_REG     0X01
#define CONFIG2_REG     0X02
#define CONFIG3_REG     0X03
#define LOFF            0X04
#define CH1SET_REG      0X05
#define CH2SET_REG      0X06
#define CH3SET_REG      0X07
#define CH4SET_REG      0X08
#define CH5SET_REG      0X09
#define CH6SET_REG      0X0A
#define CH7SET_REG      0X0B
#define CH8SET_REG      0X0C
#define RLD_SENSP_REG   0X0D
#define RLD_SENSN_REG   0X0E
#define LOFF_SENSP_REG  0X0F
#define LOFF_SENSN_REG  0X10
#define LOFF_FLIP_REG   0X11
#define LOFF_STATP_REG  0X12
#define LOFF_STATN_REG  0X13

#define ADS_GPIO_REG    0X14
#define PACE_REG        0X15
#define RESP_REG        0X16
#define CONFIG4_REG     0X17
#define WCT1_REG        0X18
#define WCT2_REG        0X19
// SPI beállítások (ADS129x: CPOL=0, CPHA=1 -> MODE1)
#define ADS_SPI_FREQ    1000000 // 1 MHz van konfigolva a BT832 firmware-ben
SPISettings adsSpi(ADS_SPI_FREQ, MSBFIRST, SPI_MODE1);

#define REMAINING_CHANNELS 5
#define BYTES_PER_CHANNEL 3

#define WAKEUP_CMD 0x02
#define STANDBY_CMD 0x04
#define RESET_CMD 0x06
#define START_CMD 0x08
#define STOP_CMD 0x0A
#define RDATAC_CMD 0x10
#define SDATAC_CMD 0x11
#define RDATA_CMD 0x12

ads1298::ads1298(uint8_t sck, uint8_t miso, uint8_t mosi, uint8_t ss, uint8_t drdy, uint8_t chip_select, uint8_t start, uint8_t pwdn,uint8_t adc_reset,uint8_t clk_sel)
    : pin_cs_(chip_select),
      pin_drdy_(drdy),
      pin_pwdn_(pwdn),
      pin_reset_(adc_reset),
      pin_start_(start)
{
    SPI.begin(sck, miso, mosi, ss);
    pinMode(pin_drdy_, INPUT_PULLUP);
    pinMode(pin_cs_, OUTPUT);
    pinMode(start,OUTPUT);
    pinMode(pwdn,OUTPUT);
    pinMode(adc_reset,OUTPUT);
    pinMode(clk_sel,OUTPUT);
    digitalWrite(clk_sel,HIGH); // internal clock
    digitalWrite(start,LOW);

}

void ads1298::init_adc()
{
    digitalWrite(pin_pwdn_,HIGH);
    digitalWrite(pin_reset_,HIGH);
    delayMicroseconds(200);
    digitalWrite(pin_reset_,LOW);
    delayMicroseconds(20);
    stop_data_stream(); // // Device Wakes Up in RDATAC Mode, so Send SDATAC Command so Registers can be Written
    uint8_t reg = read_reg(CONFIG1_REG);
    Serial.print("Config1 reg default:");
    Serial.println(reg);
    write_reg(CONFIG1_REG, 0b10000000 | reg);
    reg = read_reg(CONFIG1_REG);
    Serial.print("Config1 reg changed:");
    Serial.println(reg);
     write_reg(CONFIG1_REG, 0b11000110);
     write_reg(CONFIG2_REG, 0b00110011);
     write_reg(CONFIG3_REG, 0b11101101);
     write_reg(CH1SET_REG, 0b00010000);
     write_reg(CH2SET_REG, 0b00010000);
     write_reg(CH3SET_REG, 0b00010000);
     write_reg(CH4SET_REG, 0b00010000);
     write_reg(CH5SET_REG, 0b00010000);
     write_reg(CH6SET_REG, 0b00010000);
     write_reg(CH7SET_REG, 0b00010000);
     write_reg(CH8SET_REG, 0b00010000);
    start_data_stream();
     
}



void ads1298::write_reg(uint8_t addr, uint8_t data)
{
        SPI.beginTransaction(adsSpi);
    digitalWrite(pin_cs_, LOW);
    SPI.transfer((0b010 << 5) | addr ); // First opcode byte: 010r rrrr, where r rrrr is the starting register address.
    SPI.transfer(1); // Second opcode byte: 000n nnnn, where n nnnn is the number of registers to write – 1
    SPI.transfer(data);
    digitalWrite(pin_cs_, HIGH);
        SPI.endTransaction();
    delay(1);
}

uint8_t ads1298::read_reg(uint8_t addr)
{
    SPI.beginTransaction(adsSpi);
    uint8_t rdData;
    digitalWrite(pin_cs_, LOW);
    SPI.transfer((0b001 << 5) | addr); // First opcode byte: 001r rrrr, where r rrrr is the starting register address.
    SPI.transfer(1); //Second opcode byte: 000n nnnn, where n nnnn is the number of registers to read – 1.
    rdData = SPI.transfer(0);
    digitalWrite(pin_cs_, HIGH);
    SPI.endTransaction();
    return (rdData);
}

bool ads1298::is_data_ready()
{
    return !digitalRead(pin_drdy_);
}
void ads1298::read_data_stream(uint8_t* data, int length)
{
    SPI.beginTransaction(adsSpi);
    digitalWrite(pin_cs_, LOW);
   SPI.transfer(0xff);
    for (int i = 0; i < length; i++)
        data[i] = SPI.transfer(0xff);
    for (int i = 0; i < REMAINING_CHANNELS * BYTES_PER_CHANNEL; i++)
        SPI.transfer(0xff);
    digitalWrite(pin_cs_, HIGH);
    SPI.endTransaction();
}
void ads1298::stop_data_stream()
{
      SPI.beginTransaction(adsSpi);
    digitalWrite(pin_cs_, LOW);
    SPI.transfer(SDATAC_CMD);
    digitalWrite(pin_cs_, HIGH);
      SPI.endTransaction();
}
void ads1298::start_data_stream()
{
    
      SPI.beginTransaction(adsSpi);
    digitalWrite(pin_cs_, LOW);
    SPI.transfer(START_CMD);    
    SPI.transfer(RDATAC_CMD);
    digitalWrite(pin_cs_, HIGH);
      SPI.endTransaction();
      Serial.println("dataStream Started");
}
