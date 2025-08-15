#ifndef __ADS_1298_H__
#define __ADS_1298_H__

class ads1298
{
    uint8_t pin_cs_;
    uint8_t pin_drdy_;
    uint8_t pin_pwdn_;
    uint8_t pin_reset_;
    uint8_t pin_start_;

public:
    ads1298(uint8_t sck, uint8_t miso, uint8_t mosi, uint8_t ss, uint8_t drdy, uint8_t chip_select, uint8_t start, uint8_t pwdn,uint8_t adc_reset,uint8_t clk_sel);
    void init_adc();
    void read_data_stream(uint8_t* data, int length);
    void stop_data_stream();
    void start_data_stream();

private:
    bool is_data_ready();
    uint8_t read_reg(uint8_t addr);
    void write_reg(uint8_t addr, uint8_t data);
};

#endif /// __ADS_1298_H__