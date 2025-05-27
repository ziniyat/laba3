#ifndef SERVER_HPP
#define SERVER_HPP

#include <boost/asio.hpp>
#include <vector>
#include <memory>
#include <string>
#include <sstream>
#include <iomanip>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>
#include <numeric>
#include <algorithm>

using boost::asio::ip::tcp;

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(tcp::socket socket, boost::asio::io_context& io_context, 
            boost::asio::strand<boost::asio::io_context::executor_type>& strand);
    void start();
    
private:
    void do_read();
    void handle_request(const std::string& request);
    void handle_calculator(const std::string& request);
    void handle_average(const std::string& request);
    void handle_reminder(const std::string& request);
    void do_write(const std::string& response);
    void send_reminder(const std::string& message, int delay);
    void log(const std::string& message);

    tcp::socket socket_;
    boost::asio::streambuf buffer_;
    boost::asio::io_context& io_context_;
    boost::asio::strand<boost::asio::io_context::executor_type>& strand_;
    std::vector<std::string> log_;
};

class Server {
public:
    Server(boost::asio::io_context& io_context, short port, int num_threads);
    ~Server();

private:
    void do_accept();

    tcp::acceptor acceptor_;
    boost::asio::io_context& io_context_;
    boost::asio::strand<boost::asio::io_context::executor_type> strand_;
    std::vector<std::thread> threads_;
    std::atomic<bool> stop_{false};
};

#endif // SERVER_HPP
