#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <vector>
#include <functional>

#include "FifoBuffer.h"
#include "ZaxJsonParser.h"
#include "http_server.h"
#include "rest_api.h"
#include "http_rest_api.h"
#include "wifi_connection.h"
#include "print.h"
#include "http_file_handler.h"
#include "compatibility.h"
#include "scheduler.h"
#include "file_io.h"
#include "data_server.h"
#include "ads1298.h"

using namespace std;
#if defined(ARDUINO)
constexpr char ip_addr[] = "192.168.1.184";
#else
constexpr char ip_addr[] = "127.0.0.1";
#endif

class buffer_writer
{
    static const int NR_MAX_PACKETS_CACHED = 20;
    int packet_sample_iterator_ = 0;
    uint8_t* current_address_to_fill = 0;
    io_buffer<NR_MAX_PACKETS_CACHED> io_buffer_;

public:
    static const int NRCHANNELS = 3;
    static const int BYTESPERCHANNEL = 3;
    static const int NR_MILLISECS_IN_ONE_PACKET = 100;
    static const int SAMPLING_RATE = 2000;
    static const int NR_SAMPLES_PER_PACKET_PER_CHANNEL = (SAMPLING_RATE * NR_MILLISECS_IN_ONE_PACKET) / 1000; /// 200
    static const int NR_BYTES_PER_PACKET = NR_SAMPLES_PER_PACKET_PER_CHANNEL * NRCHANNELS * BYTESPERCHANNEL; /// 1800

    buffer_writer()
        : io_buffer_(NR_BYTES_PER_PACKET)
    {
        current_address_to_fill = io_buffer_.get_next_address_to_fill();
    }

    inline bool has_room()
    {
        bool res = current_address_to_fill ? true : false;
        if (!res)
        {
            current_address_to_fill = io_buffer_.get_next_address_to_fill();
            res = current_address_to_fill ? true : false;
        }
        return res;
    }

    void write_one_sample(uint8_t* sample_bytes) /// size: NRCHANNELS * BYTESPERCHANNEL
    {
        if (current_address_to_fill)
        {
           for (uint16_t i = 0; i < NRCHANNELS; ++i)
               for (int k = 0; k < BYTESPERCHANNEL; ++k)
                   current_address_to_fill[i * NR_SAMPLES_PER_PACKET_PER_CHANNEL * BYTESPERCHANNEL + packet_sample_iterator_ * BYTESPERCHANNEL + k] = sample_bytes[i * BYTESPERCHANNEL + k];
//            for (int k = 0; k < BYTESPERCHANNEL; ++k)
//                for (uint16_t i = 0; i < NRCHANNELS; ++i)
//                    current_address_to_fill[k * NR_SAMPLES_PER_PACKET_PER_CHANNEL * NRCHANNELS + packet_sample_iterator_ * NRCHANNELS + i] = sample_bytes[i * BYTESPERCHANNEL + k];
            // for (uint16_t i = 0; i < NRCHANNELS; ++i)
            //     for (int k = 0; k < BYTESPERCHANNEL; ++k)
            //         current_address_to_fill[(i * BYTESPERCHANNEL + k) * NR_SAMPLES_PER_PACKET_PER_CHANNEL + packet_sample_iterator_] = sample_bytes[i * BYTESPERCHANNEL + k];
            ++packet_sample_iterator_;
            if (packet_sample_iterator_ == NR_SAMPLES_PER_PACKET_PER_CHANNEL)
            {
                current_address_to_fill = io_buffer_.get_next_address_to_fill();
                packet_sample_iterator_ = 0;
            }
        }
        else
            current_address_to_fill = io_buffer_.get_next_address_to_fill();
    }

    uint8_t* get_next_filled_address()
    {
        return io_buffer_.get_next_filled_address();
    }
};

class analog_data_aq
{
    buffer_writer& buffer_writer_;
    ads1298 adc;

public:
    static int const PIN_CSB = 15;
    static int const PIN_SCLK = 14;
    static int const PIN_MOSI = 13;
    static int const PIN_MISO = 12;
    static int const PIN_ALAB = 2;
    static int const PIN_DRDY = 35;
    static int const PIN_START = 23;
    static int const PIN_PWDN = 22;
    static int const PIN_RESET = 19;
    static int const PIN_CLKSEL = 18;

    analog_data_aq(buffer_writer& buffer_writer)
        : buffer_writer_(buffer_writer),
          adc(PIN_SCLK, PIN_MISO, PIN_MOSI, PIN_CSB, PIN_DRDY, PIN_CSB, PIN_START,PIN_PWDN,PIN_RESET,PIN_CLKSEL)
    {
        adc.init_adc();
    }

    void aquire_data()
    {
        uint8_t buf[buffer_writer_.BYTESPERCHANNEL * buffer_writer_.NRCHANNELS];
        if (buffer_writer_.has_room())
        {
            adc.read_data_stream(buf, buffer_writer_.BYTESPERCHANNEL * buffer_writer_.NRCHANNELS);
            buffer_writer_.write_one_sample(buf);
        }
    }
};

class application: public http_rest_api_handler
{
    struct is_sampling
    {
        bool serial = false;
        bool bluetooth = false;
        bool tcp = false;
    };

    struct read_setup_params
    {
        std::vector<unsigned int> sampleRates;
        std::vector<unsigned int> gain;
        std::vector<unsigned int> physicalMapping;
        unsigned int millisecondsToReadInOnePacket;
        ZAX_JSON_SERIALIZABLE(read_setup_params, JSON_PROPERTY(sampleRates), JSON_PROPERTY(gain), JSON_PROPERTY(physicalMapping), JSON_PROPERTY(millisecondsToReadInOnePacket))
    };

    struct persistent_settings
    {
        std::string wireless_connection_type = "wifi"; /// "bluetooth"
        wifi_connection::wifi_settings wifi_settings;
        ZAX_JSON_SERIALIZABLE(persistent_settings, JSON_PROPERTY(wireless_connection_type), JSON_PROPERTY(wifi_settings))
    };

    persistent_settings persistent_settings_;
    buffer_writer io_buffer_;
    wifi_connection wifi_connection_;
    i_http_server* http_server_serial_ = 0;
    i_http_server* http_server_bluetooth_ = 0;
    i_http_server* http_server_tcp_ = 0;
    i_data_server* data_server_serial_ = 0;
    i_data_server* data_server_bluetooth_ = 0;
    i_data_server* data_server_tcp_ = 0;

    http_file_handler http_file_handler_;
    i_scheduler* scheduler_;
    std::string http_response_;
    std::map<std::string, std::string> cmd_desc_ =
    {
        {"setupReading", "sets up the recording."},
        {"startReading", "starts the recording."},
        {"stopReading", "stops the recording."},
        {"set_wifi_settings", "Sets wifi settings. Parameters must contain an object with 2 key-value pairs, the keys being 'ssid' and 'password' while the values must be strings."},
        {"connect_wifi", "Connects WiFi."},
        {"disconnect_wifi", "Connects WiFi."}
    };

    void add_handler(const char* command, std::function<const char* (const char*, size_t&)> handler)
    {
        http_rest_api_handler::restapi->add_handler(command, cmd_desc_[command].c_str(), handler);
    }

    int update_wireless_services()
    {
        int res = 0;
        if (persistent_settings_.wireless_connection_type == "bluetooth")
        {
            if (wifi_connection_.is_connected())
            {
                wifi_connection_.disconnect();
                i_http_server::delete_instance_tcp(http_server_tcp_);
                i_data_server::delete_tcp(data_server_tcp_);
                http_server_tcp_ = 0;
                data_server_tcp_ = 0;
            }
            if (!data_server_bluetooth_)
            {
                data_server_bluetooth_ = i_data_server::new_bluetooth(0);
                http_server_bluetooth_ = i_http_server::new_instance_bluetooth();
                http_server_bluetooth_->add_handler(this);
            }
        }
        else
        {
            if (data_server_bluetooth_)
            {
                i_data_server::delete_bluetooth(data_server_bluetooth_);
                i_http_server::delete_instance_bluetooth(http_server_bluetooth_);
                data_server_bluetooth_ = 0;
                http_server_bluetooth_ = 0;
            }
            if (wifi_connection_.is_connected())
                wifi_connection_.disconnect();
            res = wifi_connection_.connect(ip_addr);
            if (http_server_tcp_)
                i_http_server::delete_instance_tcp(http_server_tcp_);
            http_server_tcp_ = i_http_server::new_instance_tcp(ip_addr, 80);
            http_server_tcp_->add_handler(this);
            http_server_tcp_->add_handler(&http_file_handler_);
            http_server_tcp_->start_accept();
            if (!data_server_tcp_)
                data_server_tcp_ = i_data_server::new_tcp(ip_addr, 81);
            data_server_tcp_->start_accept();
        }
        return res;
    }

    void save_persistent_settings()
    {
        std::string settingsstr = persistent_settings_;
        write_buffer("/persistent_settings_.txt", std::vector<char>(settingsstr.begin(), settingsstr.end()));
        print("persistent settings written: ");
        println(settingsstr.c_str());
    }

    bool load_persistent_settings()
    {
        bool result = false;
        std::vector<char> conn_sets;
        if (read_buffer("/persistent_settings_.txt", conn_sets))
        {
            conn_sets.push_back(0);
            print("persistent settings found: ");
            println(conn_sets.data());
            if (conn_sets.size() > 10)
            {
                persistent_settings_ = conn_sets.data();
                result = true;
            }
            else
                print("persistent settings not valid.");
        }
        else
            println("persistent settings not found.");
        return result;
    }

    const char* return_http_response(size_t& response_len)
    {
        response_len = http_response_.length();
        return http_response_.c_str();
    }

public:
    analog_data_aq analog_data_aq_;
    is_sampling is_sampling_;

public:
    application()
        : analog_data_aq_(io_buffer_)
    {
        if (!load_persistent_settings())
            save_persistent_settings();

        wifi_connection_.set_ap_settings(persistent_settings_.wifi_settings);

        http_server_serial_ = i_http_server::new_instance_serial();
        http_server_serial_->add_handler(this);
        data_server_serial_ = i_data_server::new_serial(0);
        update_wireless_services();

        scheduler_ = chrono_scheduler_factory::new_instance();

        add_handler("setupReading", [this](const char* parameters, size_t& response_len)
        {
            read_setup_params params = parameters;
            println("setupReading received.");
            return return_http_response(response_len);
        });

        add_handler("startReading", [this](const char* parameters, size_t& response_len)
        {
            println("startReading received.");
            http_response_ = R"({"status": "starting reading", "params":{"dataStreamPort":81}})";
            is_sampling_.serial = true;
            return return_http_response(response_len);
        });

        add_handler("stopReading", [this](const char* parameters, size_t& response_len)
        {
            println("stopReading received.");
            http_response_ = R"({"status": "stopping"})";
            is_sampling_.serial = false;
            return return_http_response(response_len);
        });

        add_handler("connect_wifi", [this](const char* parameters, size_t& response_len)
        {
            println("connect_wifi");
            int err_code = update_wireless_services();
            if (err_code == 0)
                http_response_ = R"({"status": "WiFi connected."})";
            else
                http_response_ = R"({"status": "WiFi connection error."})";
            return return_http_response(response_len);
        });

        add_handler("disconnect_wifi", [this](const char* parameters, size_t& response_len)
        {
            println("disconnect_wifi");
            wifi_connection_.disconnect();
            http_response_ = R"({"status": "WiFi disconnected."})";
            return return_http_response(response_len);
        });

        add_handler("set_wifi_settings", [this](const char* parameters, size_t& response_len)
        {
            persistent_settings_.wifi_settings = parameters;
            persistent_settings_.wireless_connection_type = "wifi";
            print("set_wifi_settings. Params: ");
            println(parameters);
            save_persistent_settings();
            int err_code = 0;
            if (persistent_settings_.wifi_settings.reconnect)
                err_code = update_wireless_services();
            if (err_code == 0 && persistent_settings_.wifi_settings.reconnect)
                http_response_ = R"({"status": "WiFi settings set, WiFi successfully connected."})";
            else if (err_code == 0 && !persistent_settings_.wifi_settings.reconnect)
                http_response_ = R"({"status": "WiFi settings set, but settings were not applied to initiate a WiFi connection."})";
            else
                http_response_ = R"({"status": "WiFi settings set, but WiFi connection failed."})";
            return return_http_response(response_len);
        });

        add_handler("set_bluetooth_settings", [this](const char* parameters, size_t& response_len)
        {
            persistent_settings_.wireless_connection_type = "bluetooth";
            println("set_bluetooth_settings.");
            save_persistent_settings();
            scheduler_->schedule([this]()
            {
                update_wireless_services();
            }, 1000, 1);
            http_response_ = R"({"status": "Bluetooth turned on."})";
            return return_http_response(response_len);
        });
    }

    void poll()
    {
        static const std::array<uint8_t, 8> delimiter = {0xAA, 0xAA, 0xAA, 0xAA, 0x00, 0x00, 0x00, 0x00};

        http_server_serial_->poll();
        if (http_server_bluetooth_)
            http_server_bluetooth_->poll();
        if (http_server_tcp_)
            http_server_tcp_->poll();
        if (data_server_tcp_) /// only TCP data service needs to be polled as it can have multiple sessions and should handle incoming connections
            data_server_tcp_->poll();
        scheduler_->poll();
        if (is_sampling_.serial || is_sampling_.bluetooth || is_sampling_.tcp)
        {
            while (uint8_t* buff = io_buffer_.get_next_filled_address())
            {
#if not defined(ARDUINO)
                sleep_ms(50);
#endif
                data_server_serial_->send_data(delimiter.data(), delimiter.size());
                data_server_serial_->send_data(buff, io_buffer_.NR_BYTES_PER_PACKET);

                int res = 0;
                if (data_server_bluetooth_)
                {
                    data_server_bluetooth_->send_data(delimiter.data(), delimiter.size());
                    res = data_server_bluetooth_->send_data(buff, io_buffer_.NR_BYTES_PER_PACKET);
                }
                if (data_server_tcp_ && data_server_tcp_->nr_sessions())
                {
                    data_server_tcp_->send_data(delimiter.data(), delimiter.size());
                    res = data_server_tcp_->send_data(buff, io_buffer_.NR_BYTES_PER_PACKET);
                }

                if (res != 0)
                {
                    print("error.");
                    buff = 0;
                }
                else
                    buff = io_buffer_.get_next_filled_address();
            }
        }
    }
    ~application()
    {
        i_data_server::delete_serial(data_server_serial_);
        i_data_server::delete_bluetooth(data_server_bluetooth_);
        i_data_server::delete_tcp(data_server_tcp_);
    }
};

#ifndef ARDUINO_ISR_ATTR
#define ARDUINO_ISR_ATTR
#endif // ARDUINO_ISR_ATTR

application* application_;
void ARDUINO_ISR_ATTR isr()
{
    //sleep_ms(1);
    if (application_->is_sampling_.serial || application_->is_sampling_.bluetooth || application_->is_sampling_.tcp)
        application_->analog_data_aq_.aquire_data();       
}

//Serial.print("Current CPU core ");
//Serial.println(xPortGetCoreID());

void setup()
{
    ZaxJsonParser::set_initial_alloc_size(1000);
    init_print(921600 /** 115200 */);
    sleep_ms(200);
    application_ = new application();
    application_->poll();
    sleep_ms(200);
#if defined(ARDUINO)
    attachInterrupt(analog_data_aq::PIN_DRDY, isr, FALLING);
#endif // ARDUINO
}

void loop()
{
    application_->poll();
#if not defined(ARDUINO)
    isr();
#endif // ARDUINO
}

#if not defined(ARDUINO)
int main()
{
    setup();
    static unsigned int count = 0;
    while(true)
    {
        if (!(count++ % 97))
            sleep_ms(20);
        loop();
    };
}
#endif
