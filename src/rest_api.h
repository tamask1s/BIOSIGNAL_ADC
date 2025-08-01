#ifndef _REST_API_H_
#define _REST_API_H_

class i_rest_api
{
public:
    virtual void add_handler(const char* command, const char* description, std::function<const char* (const char*, size_t&)>) = 0;
    virtual const char* handle_api_msg(const char* message, size_t& response_len) = 0;

public:
    static i_rest_api* new_instance();
    static void delete_instance(i_rest_api* instance);
};

#endif /// _REST_API_H_
