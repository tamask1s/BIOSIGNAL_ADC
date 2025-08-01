class i_data_server
{
public:
    virtual int send_data(const uint8_t* data, size_t len) = 0;
    virtual void poll() = 0;
    virtual void start_accept() = 0;
    virtual int nr_sessions() = 0;
public:
    static i_data_server* new_tcp(const char* ip, uint32_t port);
    static void delete_tcp(i_data_server* instance);

    static i_data_server* new_serial(const char* params = 0);
    static void delete_serial(i_data_server* instance);

    static i_data_server* new_bluetooth(const char* params = 0);
    static void delete_bluetooth(i_data_server* instance);
};
