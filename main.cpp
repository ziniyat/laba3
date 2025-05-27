#include "server.hpp"
#include "client.hpp"
#include <iostream>
#include <boost/asio.hpp>

int main(int argc, char* argv[]) {
    try {
        if (argc < 2) {
            std::cerr << "Использование: " << argv[0] << " <server|client> [аргументы...]\n";
            std::cerr << "Сервер: " << argv[0] << " server <порт> <потоки>\n";
            std::cerr << "Клиент: " << argv[0] << " client <хост> <порт>\n";
            return 1;
        }
        
        std::string mode(argv[1]);
        
        if (mode == "server") {
            if (argc != 4) {
                std::cerr << "Использование: " << argv[0] << " server <порт> <потоки>\n";
                return 1;
            }
            
            boost::asio::io_context io_context;
            Server server(io_context, std::atoi(argv[2]), std::atoi(argv[3]));
            
            std::cout << "Сервер запущен на порту " << argv[2] << " с " << argv[3] << " потоками\n";
            std::cout << "Нажмите Enter для остановки...\n";
            std::cin.get();
        }
        else if (mode == "client") {
            if (argc != 4) {
                std::cerr << "Использование: " << argv[0] << " client <хост> <порт>\n";
                return 1;
            }
            
            boost::asio::io_context io_context;
            auto client = std::make_shared<Client>(io_context, argv[2], argv[3]);
            client->start();
            
            io_context.run();
        }
        else {
            std::cerr << "Неизвестный режим: " << mode << "\n";
            return 1;
        }
    }
    catch (std::exception& e) {
        std::cerr << "Исключение: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
