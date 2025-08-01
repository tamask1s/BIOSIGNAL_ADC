#if defined(ARDUINO)

#include <WiFi.h>
#include <ESPAsyncWebSrv.h>
#include <vector>
#include <functional>

#include "string_ext.h"
#include "http_server.h"

class http_server: public i_http_server
{
    AsyncWebServer acceptor_;
    std::vector<i_http_msg_handler*> msg_handlers_;

public:
    http_server(const char* ip, uint32_t port)
        : acceptor_(port)
    {}

    virtual ~http_server()
    {}

    void start_accept()
    {
        acceptor_.on("/api.api", HTTP_POST, [](AsyncWebServerRequest * request) {}, NULL, [this](AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total)
        {
            data[len] = 0;
            Serial.print(request->methodToString());
            Serial.println(request->url());
            size_t response_len = 0;
            const char* response = 0;
            for (auto handler : msg_handlers_)
                if ((response = (char*)handler->handle_http_message(request->methodToString(), request->url().c_str(), "api", data, len, response_len)))
                    break;

            if (response)
                request->send(200, "text/plain", response);
            else
                request->send(200, "text/plain", "Unhandled command");
        });
        acceptor_.on(0, HTTP_ANY, [this](AsyncWebServerRequest * request)
        {
            Serial.print(request->methodToString());
            Serial.println(request->url());
            size_t response_len = 0;
            const char* response = 0;
            for (auto handler : msg_handlers_)
                if ((response = (char*)handler->handle_http_message(request->methodToString(), request->url().c_str(), "html", 0, 0, response_len)))
                    break;

            if (response)
                request->send_P(200, "text/html", (const uint8_t*)response, response_len); ///todo: image/x-icon in case of media
            else
                request->send(200, "text/html", "Unhandled command");

        }, NULL, [this](AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total)
        {});
        acceptor_.begin();
    }

    virtual void add_handler(i_http_msg_handler* http_msg_handler) override
    {
        msg_handlers_.push_back(http_msg_handler);
    }

    virtual void poll()
    {}
};

#else /// ARDUINO

#include <asio.hpp>
using asio::ip::tcp;

#include "string_ext.h"
#include "http_server.h"

asio::io_context* iocontext()
{
    static asio::io_context io_context_;
    return &io_context_;
}

class http_session: public std::enable_shared_from_this<http_session>
{
    tcp::socket socket_;
    asio::streambuf buffer_;
    std::string method_;
    std::string uri_;
    size_t content_length_ = 0;
    std::vector<i_http_msg_handler*>& msg_handlers_;
    std::string response_str;

public:
    http_session(tcp::socket socket, std::vector<i_http_msg_handler*>& msg_handlers)
        : socket_(std::move(socket)),
          msg_handlers_(msg_handlers)
    {}

    void start()
    {
        read_headers();
    }

    virtual ~http_session() = default;

private:
    void read_headers()
    {
        auto self = shared_from_this();
        asio::async_read_until(socket_, buffer_, "\r\n\r\n", [this, self](const asio::error_code & error, std::size_t bytes_transferred)
        {
            if (!error)
            {
                std::string requestLine;
                std::istream requestStream(&buffer_);
                bytes_transferred = buffer_.size();
                size_t readed = 0;
                std::getline(requestStream, method_, ' ');
                readed += method_.length() + 1;
                std::getline(requestStream, uri_, ' ');
                readed += uri_.length() + 1;
                std::getline(requestStream, requestLine, ' ');
                readed += requestLine.length() + 1;
                while (std::getline(requestStream, requestLine))
                {
                    readed += requestLine.length() + 1;
                    if (requestLine.find("content-length", 0) == 0 || requestLine.find("Content-Length", 0) == 0 || requestLine.find("Content-length", 0) == 0)
                    {
                        content_length_ = stoi_compat(requestLine.c_str() + requestLine.find(":") + 1);
                        break;
                    }
                }
                while (std::getline(requestStream, requestLine))
                {
                    readed += requestLine.length() + 1;
                    if (requestLine.find("\r", 0) == 0)
                        break;
                }
                read_body(readed + content_length_ - bytes_transferred);
            }
        });
    }

    void read_body(size_t need_to_read_more_bytes)
    {
        if (need_to_read_more_bytes)
        {
            auto self = shared_from_this();
            asio::async_read(socket_, buffer_, asio::transfer_at_least(need_to_read_more_bytes), [this, self](const asio::error_code & error, std::size_t bytes_transferred)
            {
                if (!error)
                    handle_request();
            });
        }
        else
            handle_request();
    }

    static inline std::string get_extension_from_uri(const std::string& uri)
    {
        size_t last_dot_pos = uri.find_last_of('.');
        if (last_dot_pos != std::string::npos && last_dot_pos < uri.length() - 1)
            return uri.substr(last_dot_pos + 1);
        return "";
    }

    void handle_request()
    {
        buffer_.sputn("\0", 1);
        auto data = asio::buffer_cast<const unsigned char*>(buffer_.data());
        std::string extension = get_extension_from_uri(uri_);
        size_t response_len = 0;
        const unsigned char* response = 0;
        for (auto handler : msg_handlers_)
            if ((response = handler->handle_http_message(method_.c_str(), uri_.c_str(), extension.c_str(), data, content_length_, response_len)))
                break;

        response_str.resize(0);
        if (response)
        {
            response_str += "HTTP/1.1 200 OK\r\n";
            response_str += "Content-Length: " + to_string_compat(static_cast<int>(response_len)) + "\r\nContent-Type: text/html\r\n\r\n";
            response_str += std::string((char*)response, response_len);
        }
        else
        {
            //response_str += "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nContent-Type: text/html\r\n\r\n";
            response_str += "HTTP/1.1 200 OK\r\n";
            response_str += "Content-Length: " + to_string_compat(17) + "\r\nContent-Type: text/html\r\n\r\n";
            response_str += "Unhandled request";
        }

        auto self = shared_from_this();
        asio::async_write(socket_, asio::buffer(response_str), [this, self](const asio::error_code & error, std::size_t bytes_transferred)
        {
            if (!error)
                close();
        });
    }

    void close()
    {
        auto self = shared_from_this();
        asio::post(socket_.get_executor(), [this, self]()
        {
            socket_.close();
        });
    }
};

class http_server: public i_http_server
{
    asio::io_context* io_context_;
    tcp::acceptor acceptor_;
    std::vector<i_http_msg_handler*> msg_handlers_;

public:
    http_server(const char* ip, uint32_t port)
        : io_context_(iocontext()),
          acceptor_(*(io_context_), tcp::endpoint(asio::ip::address::from_string(ip), port))
    {}

    virtual ~http_server()
    {
        acceptor_.cancel();
    }

    virtual void start_accept() override
    {
        acceptor_.async_accept([this](const asio::error_code & error, tcp::socket socket)
        {
            if (!error)
            {
                std::make_shared<http_session>(std::move(socket), msg_handlers_)->start();
            }
            start_accept();
        });
    }

    virtual void add_handler(i_http_msg_handler* http_msg_handler) override
    {
        msg_handlers_.push_back(http_msg_handler);
    }

    virtual void poll()
    {
        io_context_->poll();
    }
};

#endif /// ARDUINO

i_http_server* i_http_server::new_instance_tcp(const char* ip, uint32_t port)
{
    i_http_server* server = 0;
    try
    {
        server = new http_server(ip, port);
    }
    catch (std::exception& e)
    {}
    return server;
}

void i_http_server::delete_instance_tcp(i_http_server* instance)
{
    delete((http_server*)instance);
}
