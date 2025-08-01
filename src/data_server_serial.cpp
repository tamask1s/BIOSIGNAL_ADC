#include <vector>
#include <set>
#include "print.h"
#include "data_server.h"
using namespace std;

#if defined(ARDUINO)

#include "BluetoothSerial.h"

#if defined(CONFIG_BT_SPP_ENABLED)

BluetoothSerial* SerialBT();
void bluetooth_disconnect();

#endif /// CONFIG_BT_SPP_ENABLED

class data_server_serial: public i_data_server
{
public:
    data_server_serial(const char* params)
    {}

    virtual ~data_server_serial()
    {}

    virtual void start_accept()
    {}

    virtual void poll()
    {}

    int send_data(const uint8_t* data, size_t len)
    {
        Serial.write(data, len);
        return 0;
    }

    virtual int nr_sessions()
    {
        return 1;
    }
};

#if defined(CONFIG_BT_SPP_ENABLED)

class data_server_bluetooth: public i_data_server
{
    BluetoothSerial* serial_bt;
public:
    data_server_bluetooth(const char* params)
        :serial_bt(SerialBT())
    {}

    virtual ~data_server_bluetooth()
    {
        bluetooth_disconnect();
    }

    virtual void start_accept()
    {}

    virtual void poll()
    {}

    int send_data(const uint8_t* data, size_t len)
    {
        serial_bt->write(data, len);
        return 0;
    }

    virtual int nr_sessions()
    {
        return 1;
    }
};
#endif

#else

class data_server_serial: public i_data_server
{
public:
    data_server_serial(const char* params)
    {}

    virtual ~data_server_serial()
    {}

    virtual void start_accept()
    {}

    virtual void poll()
    {}

    int send_data(const uint8_t* data, size_t len)
    {
        return 0;
    }

    virtual int nr_sessions()
    {
        return 1;
    }
};

class data_server_bluetooth: public i_data_server
{
public:
    data_server_bluetooth(const char* params)
    {}

    virtual ~data_server_bluetooth()
    {}

    virtual void start_accept()
    {}

    virtual void poll()
    {}

    int send_data(const uint8_t* data, size_t len)
    {
        return 0;
    }

    virtual int nr_sessions()
    {
        return 1;
    }
};

#endif

i_data_server* i_data_server::new_serial(const char* params)
{
    return new data_server_serial(params);
}

void i_data_server::delete_serial(i_data_server* instance)
{
    delete((data_server_serial*)instance);
}

i_data_server* i_data_server::new_bluetooth(const char* params)
{
#if defined(CONFIG_BT_SPP_ENABLED)
    return new data_server_bluetooth(params);
#else
    return 0;
#endif
}

void i_data_server::delete_bluetooth(i_data_server* instance)
{
#if defined(CONFIG_BT_SPP_ENABLED)
    delete((data_server_bluetooth*)instance);
#endif
}
