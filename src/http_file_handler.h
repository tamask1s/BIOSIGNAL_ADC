class http_file_handler: public i_http_msg_handler
{
public:
    virtual const unsigned char* handle_http_message(const char* method, const char* uri, const char* extension, const unsigned char* body, const size_t body_len, size_t& response_len);
    virtual ~http_file_handler();
};
