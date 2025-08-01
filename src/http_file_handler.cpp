#include <string.h>
#include <inttypes.h>
#include "http_server.h"
#include "http_file_handler.h"
#include "html_main.h"
#include "html_favicon.h"

const unsigned char* http_file_handler::handle_http_message(const char* method, const char* uri, const char* extension, const unsigned char* body, const size_t body_len, size_t& response_len)
{
    if (strstr(uri, "main.html"))
    {
        response_len = strlen(html_main) + 1;
        return (unsigned char*)html_main;
    }
    if (strstr(uri, "favicon.ico"))
    {
        response_len = sizeof(html_favicon);
        return html_favicon;
    }
    else
    {
        return nullptr;
    }
}

http_file_handler::~http_file_handler() {}
