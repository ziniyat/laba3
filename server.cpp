#include "server.hpp"
#include <iostream>

Session::Session(tcp::socket socket, boost::asio::io_context& io_context, 
                 boost::asio::strand<boost::asio::io_context::executor_type>& strand)
    : socket_(std::move(socket)), io_context_(io_context), strand_(strand) {}

void Session::start() {
    log("Новое подключение");
    do_read();
}

void Session::do_read() {
    auto self(shared_from_this());
    boost::asio::async_read_until(socket_, buffer_, '\n',
        [this, self](boost::system::error_code ec, std::size_t length) {
            if (!ec) {
                std::istream is(&buffer_);
                std::string request;
                std::getline(is, request);
                
                log("Получено: " + request);
                handle_request(request);
            } else {
                log("Ошибка чтения: " + ec.message());
            }
        });
}

void Session::handle_request(const std::string& request) {
    if (request.find('+') != std::string::npos || 
        request.find('-') != std::string::npos || 
        request.find('*') != std::string::npos || 
        request.find('/') != std::string::npos) {
        handle_calculator(request);
    }
    else if (request.find("remind") == 0) {
        handle_reminder(request);
    }
    else {
        handle_average(request);
    }
}

void Session::handle_calculator(const std::string& request) {
    std::istringstream iss(request);
    double a, b;
    char op;
    iss >> a >> op >> b;
    
    double result = 0;
    switch (op) {
        case '+': result = a + b; break;
        case '-': result = a - b; break;
        case '*': result = a * b; break;
        case '/': 
            if (b == 0) {
                do_write("Ошибка: деление на ноль");
                return;
            }
            result = a / b; 
            break;
    }
    
    std::ostringstream oss;
    if (op == '/') {
        oss << std::fixed << std::setprecision(2) << result;
    } else {
        oss << result;
    }
    
    log("Результат вычисления: " + oss.str());
    do_write(oss.str());
}

void Session::handle_average(const std::string& request) {
    auto self(shared_from_this());
    boost::asio::post(io_context_, [this, self, request]() {
        std::istringstream iss(request);
        std::vector<double> numbers;
        double num;
        
        while (iss >> num) {
            numbers.push_back(num);
        }
        
        if (numbers.empty()) {
            boost::asio::post(strand_, [this, self]() {
                do_write("Ошибка: нет чисел для вычисления среднего");
            });
            return;
        }
        
        double sum = std::accumulate(numbers.begin(), numbers.end(), 0.0);
        double average = sum / numbers.size();
        
        std::ostringstream oss;
        oss << "Среднее: " << std::fixed << std::setprecision(2) << average;
        
        boost::asio::post(strand_, [this, self, oss = oss.str()]() {
            log("Результат среднего: " + oss);
            do_write(oss);
        });
    });
}

void Session::handle_reminder(const std::string& request) {
    std::istringstream iss(request);
    std::string cmd;
    int delay;
    std::string message;
    
    iss >> cmd >> delay;
    std::getline(iss, message);
    message = message.substr(1); // remove leading space
    
    std::ostringstream oss;
    oss << "Напоминание будет через " << delay << " секунд";
    do_write(oss.str());
    
    send_reminder(message, delay);
}

void Session::send_reminder(const std::string& message, int delay) {
    auto self(shared_from_this());
    auto timer = std::make_shared<boost::asio::steady_timer>(io_context_, 
        std::chrono::seconds(delay));
    
    timer->async_wait([this, self, message, timer](const boost::system::error_code& ec) {
        if (!ec) {
            log("Отправка напоминания: " + message);
            do_write(message);
        }
    });
}

void Session::do_write(const std::string& response) {
    auto self(shared_from_this());
    boost::asio::async_write(socket_, boost::asio::buffer(response + "\n"),
        [this, self](boost::system::error_code ec, std::size_t) {
            if (!ec) {
                do_read();
            } else {
                log("Ошибка записи: " + ec.message());
            }
        });
}

void Session::log(const std::string& message) {
    auto self(shared_from_this());
    boost::asio::post(strand_, [this, self, message]() {
        log_.push_back(message);
        std::cout << "Лог: " << message << std::endl;
    });
}

Server::Server(boost::asio::io_context& io_context, short port, int num_threads)
    : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)),
      io_context_(io_context),
      strand_(io_context.get_executor()) {
    do_accept();
    
    for (int i = 0; i < num_threads; ++i) {
        threads_.emplace_back([&io_context]() {
            try {
                io_context.run();
            } catch (const std::exception& e) {
                std::cerr << "Ошибка в потоке: " << e.what() << std::endl;
            }
        });
    }
}

Server::~Server() {
    stop_ = true;
    io_context_.stop();
    for (auto& thread : threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

void Server::do_accept() {
    acceptor_.async_accept(
        [this](boost::system::error_code ec, tcp::socket socket) {
            if (!ec) {
                std::make_shared<Session>(std::move(socket), io_context_, strand_)->start();
            }
            
            if (!stop_) {
                do_accept();
            }
        });
}
