#include <string.h>
#include <functional>
#include "http_server.h"
#include "rest_api.h"
#include "http_rest_api.h"

const unsigned char* http_rest_api_handler::handle_http_message(const char* method, const char* uri, const char* extension, const unsigned char* body, const size_t body_len, size_t& response_len)
{
    if (!strcmp(extension, "api"))
        return (const unsigned char*)restapi->handle_api_msg((const char*)body, response_len);
    else
        return nullptr;
}

http_rest_api_handler::~http_rest_api_handler()
{
    i_rest_api::delete_instance(restapi);
}
