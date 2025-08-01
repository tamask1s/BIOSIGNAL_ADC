#include <functional>
#include <vector>
#include <string.h>
#include "FifoBuffer.h"
#include "http_server.h"
#include "print.h"

#if defined(ARDUINO)

#include <Arduino.h>
#include "BluetoothSerial.h"

#if defined(CONFIG_BT_SPP_ENABLED)

const char *pin = "1234"; // Change this to more secure PIN.
String device_name = "ESP32-BT-Slave";
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif
#if !defined(CONFIG_BT_SPP_ENABLED)
#error Serial Bluetooth not available or not enabled. It is only available for the ESP32 chip.
#endif

static bool bluetooth_initialized = false;

BluetoothSerial* SerialBT()
{
    static BluetoothSerial serial_bt;

    if (!bluetooth_initialized)
    {
        bluetooth_initialized = true;
        serial_bt.begin(device_name);
        Serial.printf("The device with name \"%s\" is started.\nNow you can pair it with Bluetooth!\n", device_name.c_str());
#ifdef USE_PIN
        serial_bt.setPin(pin);
        Serial.println("Using PIN");
#endif
    }

    return &serial_bt;
}

void bluetooth_disconnect()
{
    if (bluetooth_initialized)
    {
        BluetoothSerial* bt = SerialBT();
        bt->disconnect();
        delay(300);
        bt->end();
        bluetooth_initialized = false;
    }
}

#endif /// CONFIG_BT_SPP_ENABLED

class http_server_serial: public i_http_server
{
    i_http_msg_handler* handler_ = 0;
    fifo_buffer<char> serial_rcv_buf_;

public:
    http_server_serial()
        : serial_rcv_buf_(64)
    {}

    virtual void start_accept() {}

    void add_handler(i_http_msg_handler* http_msg_handler)
    {
        handler_ = http_msg_handler;
    }

    void poll()
    {
        while (Serial.available() > 0)
        {
            char rc = Serial.read();

            if (rc != '\n')
                serial_rcv_buf_.push_back(rc);
            else
            {
                serial_rcv_buf_.push_back('\0');
                size_t response_len = 0;
                if (handler_)
                    if (char* response = (char*)handler_->handle_http_message("", "", "api", (unsigned char*)serial_rcv_buf_.front_elements(), serial_rcv_buf_.size(), response_len))
                        println(response);
                serial_rcv_buf_.clear();
            }
        }
    }

    virtual ~http_server_serial() = default;
};

#if defined(CONFIG_BT_SPP_ENABLED)

class http_server_bluetooth: public i_http_server
{
    i_http_msg_handler* handler_ = 0;
    fifo_buffer<char> serial_rcv_buf_;
    class BluetoothSerial* SerialBT_;

public:
    http_server_bluetooth()
        : serial_rcv_buf_(64),
          SerialBT_(SerialBT())
    {}

    virtual void start_accept() {}

    void add_handler(i_http_msg_handler* http_msg_handler)
    {
        handler_ = http_msg_handler;
    }

    void poll()
    {
        while (SerialBT_ && (SerialBT_->available() > 0))
        {
            char rc = SerialBT_->read();

            if (rc != '\n')
                serial_rcv_buf_.push_back(rc);
            else
            {
                serial_rcv_buf_.push_back('\0');
                size_t response_len = 0;
                println("serial incoming BT: ");
                println(serial_rcv_buf_.front_elements());
                if (handler_)
                    if (char* response = (char*)handler_->handle_http_message("", "", "api", (unsigned char*)serial_rcv_buf_.front_elements(), serial_rcv_buf_.size(), response_len))
                    {
                        SerialBT_->write((uint8_t*)response, response_len);
                        SerialBT_->write((uint8_t*)"\n", 1);
                    }
                serial_rcv_buf_.clear();
            }
        }
    }

    virtual ~http_server_bluetooth()
    {
        bluetooth_disconnect();
    }
};

#endif /// CONFIG_BT_SPP_ENABLED

#else /// ARDUINO

class http_server_serial: public i_http_server
{
public:
    http_server_serial()
    {}

    virtual void start_accept() {}

    void add_handler(i_http_msg_handler* http_msg_handler)
    {}

    void poll()
    {}

    virtual ~http_server_serial() = default;
};

#define http_server_bluetooth http_server_serial

#endif /// ARDUINO

i_http_server* i_http_server::new_instance_serial()
{
    return new http_server_serial();
}

void i_http_server::delete_instance_serial(i_http_server* instance)
{
    delete((http_server_serial*)instance);
}

i_http_server* i_http_server::new_instance_bluetooth()
{
#if defined(CONFIG_BT_SPP_ENABLED)
    return new http_server_bluetooth();
#else
    return 0;
#endif
}

void i_http_server::delete_instance_bluetooth(i_http_server* instance)
{
#if defined(CONFIG_BT_SPP_ENABLED)
    delete((http_server_bluetooth*)instance);
#endif
}
