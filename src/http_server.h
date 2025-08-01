#ifndef _HTTP_SERVER_H_
#define _HTTP_SERVER_H_

//class singletons
//{
//    static void* io_context();
//    static void* serial_context_bluetooth();
//};

class i_http_msg_handler
{
public:
    virtual const unsigned char* handle_http_message(const char* method, const char* uri, const char* extension, const unsigned char* body, const size_t body_len, size_t& response_len) = 0;
};

class i_http_server
{
public:
    virtual void start_accept() = 0;
    virtual void add_handler(i_http_msg_handler* http_msg_handler) = 0;
    virtual void poll() = 0;

public:
    static i_http_server* new_instance_tcp(const char* ip, uint32_t port);
    static void delete_instance_tcp(i_http_server* instance);

    static i_http_server* new_instance_serial();
    static void delete_instance_serial(i_http_server* instance);

    static i_http_server* new_instance_bluetooth();
    static void delete_instance_bluetooth(i_http_server* instance);
};

#endif ///_HTTP_SERVER_H_
