#include <vector>
#include <set>
#include "print.h"
#include "data_server.h"
using namespace std;

#if defined(ARDUINO)

#include <WiFi.h>
class data_server: public i_data_server
{
    WiFiServer server;
    std::set<WiFiClient*> sessions_;
public:
    data_server(const char* ip, uint32_t port)
        : server(port)
    {
        print("data_server listening on port: ");
        println(port);
    }

    int send_data(const uint8_t* data, size_t len)
    {
        for (auto it = sessions_.begin(); it != sessions_.end();)
        {
            if ((*it)->connected())
            {
                (*it)->write(data, len);
                (*it)->flush();
                ++it;
            }
            else
            {
                println("CLOSING AND DELETING SESSION");
                (*it)->stop();
                delete (*it);
                it = sessions_.erase(it);
            }
        }
        return sessions_.size() == 0 ? 1 : 0;
    }

    void poll()
    {
        WiFiClient session = server.available();
        if (session)
        {
            println("New data session connected.");
            sessions_.insert(new WiFiClient(session));
        }
    }

    virtual void start_accept()
    {
        server.begin();
    }

    virtual int nr_sessions()
    {
        return sessions_.size();
    }
};
#else

#include <asio.hpp>
asio::io_context* iocontext();

class tcp_session
{
    asio::ip::tcp::socket socket_;

public:
    tcp_session(asio::ip::tcp::socket socket)
        : socket_(std::move(socket))
    {}

    int res = 0;
    int send_data(const uint8_t* data, size_t len)
    {
        print(">");
        if (socket_.is_open())
        {
            asio::async_write(socket_, asio::buffer(data, len), [this, len](const asio::error_code & error, std::size_t bytes_transferred)
            {
                if (error || bytes_transferred != len)
                {
                    print("-");
                    res = 1;
                }
                else
                {
                    print("+");
                }
            });
        }
        else
            res = 1;
        if (res)
        {
            res = 0;
            return 1;
        }
        else
            return 0;
    }

    void close()
    {
        println("socket close called. ");
        socket_.close();
    }
};

class data_server: public i_data_server
{
    asio::io_context* io_context_;
    asio::ip::tcp::acceptor acceptor_;
    std::set<tcp_session*> sessions_;

public:
    data_server(const char* ip, uint32_t port)
        : io_context_(iocontext()),
          acceptor_(*(io_context_), asio::ip::tcp::endpoint(asio::ip::address::from_string(ip), port))
    {}

    virtual ~data_server()
    {
        acceptor_.cancel();
    }

    virtual void start_accept()
    {
        acceptor_.async_accept([this](const asio::error_code & error, asio::ip::tcp::socket socket)
        {
            if (!error)
            {
                tcp_session* session = new tcp_session(std::move(socket));
                sessions_.insert(session);
            }
            start_accept();
            std::cout << "restart accept" << endl;
        });
    }

    virtual void poll()
    {
        io_context_->poll();
    }

    int send_data(const uint8_t* data, size_t len)
    {
        for (auto it = sessions_.begin(); it != sessions_.end();)
        {
            if ((*it)->send_data(data, len))
            {
                println("CLOSING AND DELETING SESSION");
                (*it)->close();
                delete (*it);
                it = sessions_.erase(it);
            }
            else
                ++it;
        }
        return sessions_.size() == 0 ? 1 : 0;
    }

    virtual int nr_sessions()
    {
        return sessions_.size();
    }
};

#endif

i_data_server* i_data_server::new_tcp(const char* ip, uint32_t port)
{
    return new data_server(ip, port);
}

void i_data_server::delete_tcp(i_data_server* instance)
{
    delete((data_server*)instance);
}
