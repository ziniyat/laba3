#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <memory>

using boost::asio::ip::tcp;

class Client : public std::enable_shared_from_this<Client> {
public:
    Client(boost::asio::io_context& io_context, const std::string& host, const std::string& port);
    void start();

private:
    void do_read();
    void do_write();

    tcp::socket socket_;
    boost::asio::streambuf buffer_;
    std::string input_;
};

#endif // CLIENT_HPP
