#ifndef __ADS_1293_H__
#define __ADS_1293_H__



class ads1293
{
    uint8_t pin_cs_;
    uint8_t pin_drdy_;

public:
    ads1293(uint8_t sck, uint8_t miso, uint8_t mosi, uint8_t ss, uint8_t drdy, uint8_t chip_select);
    void init_adc();
    void read_data_stream(uint8_t* data, int length);

private:
    uint8_t read_reg(uint8_t addr);
    void write_reg(uint8_t addr, uint8_t data);
};

#endif /// __ADS_1293_H__
