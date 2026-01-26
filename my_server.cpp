#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
    if (argc != 2) return 1;

    int port = std::stoi(argv[1]);

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
    listen(serverSocket, 5);
    
    std::cout << "Server listening on port " << port << "..." << std::endl;

    socklen_t addrlen = sizeof(serverAddress);
    int newSocket = accept(serverSocket, (struct sockaddr*)&serverAddress, &addrlen);
    
    std::cout << "Client connected! Receiving messages:" << std::endl;

    char buffer[1024];
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytes = read(newSocket, buffer, 1023);
        if (bytes <= 0) break;
        buffer[bytes] = '\0';
        std::cout << buffer << std::flush;
        if (std::string(buffer) == "terminate\n") break;
    }

    close(newSocket);
    close(serverSocket);
    return 0;
}