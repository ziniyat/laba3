#include "client.hpp"

Client::Client(boost::asio::io_context& io_context, const std::string& host, const std::string& port)
    : socket_(io_context) {
    tcp::resolver resolver(io_context);
    auto endpoints = resolver.resolve(host, port);
    boost::asio::connect(socket_, endpoints);
}

void Client::start() {
    do_read();
    do_write();
}

void Client::do_read() {
    auto self(shared_from_this());
    boost::asio::async_read_until(socket_, buffer_, '\n',
        [this, self](boost::system::error_code ec, std::size_t length) {
            if (!ec) {
                std::istream is(&buffer_);
                std::string line;
                std::getline(is, line);
                std::cout << "Сервер: " << line << std::endl;
                do_read();
            } else {
                std::cerr << "Ошибка чтения: " << ec.message() << std::endl;
            }
        });
}

void Client::do_write() {
    std::cout << "Введите сообщение: ";
    std::getline(std::cin, input_);
    
    if (input_.empty()) {
        return;
    }
    
    auto self(shared_from_this());
    boost::asio::async_write(socket_, boost::asio::buffer(input_ + "\n"),
        [this, self](boost::system::error_code ec, std::size_t) {
            if (!ec) {
                do_write();
            } else {
                std::cerr << "Ошибка записи: " << ec.message() << std::endl;
            }
        });
}
