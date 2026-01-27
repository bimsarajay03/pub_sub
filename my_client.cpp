#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
    
    if (argc != 3){
        std::cerr << "Usage: " << argv[0] << " <server_ip> <port>" << std::endl;
        return 1;
    }

    const char* serverIP = argv[1];
    int port = std::atoi(argv[2]);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return 1;
    }

    sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    inet_pton(AF_INET, serverIP, &serv_addr.sin_addr);

    std::cout << "Connecting to " << serverIP << ":" << port << "..." << std::endl;
    
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Connection failed! Is the server running?" << std::endl;
        return 1;
    }
    
    std::cout << "Connected! Type messages (or 'terminate' to quit):" << std::endl;

    std::string input;
    while (true) {
        std::cout << "> " << std::flush;
        std::getline(std::cin, input);
        input += '\n';
        send(sock, input.c_str(), input.size(), 0);
        if (input == "terminate\n") break;
    }

    std::cout << "Disconnected." << std::endl;
    close(sock);
    return 0;
}