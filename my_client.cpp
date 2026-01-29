#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

// Structure to pass socket to receive thread
struct SocketInfo {
    int socket;
};

// Thread function to receive messages from server
void* receiveMessages(void* arg) {
    SocketInfo* info = (SocketInfo*)arg;
    int sock = info->socket;
    char buffer[1024];
    
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytes = recv(sock, buffer, 1023, 0);
        
        if (bytes <= 0) {
            // Server disconnected
            break;
        }
        
        buffer[bytes] = '\0';
        // Print received message on a new line
        std::cout << "\n" << buffer << "> " << std::flush;
    }
    
    delete info;
    return NULL;
}

 

int main(int argc, char* argv[]) {
    
    if (argc != 4){
        std::cerr << "Usage: " << argv[0] << " <server_ip> <port>" << std::endl;
        return 1;
    }

    const char* serverIP = argv[1];
    int port = std::atoi(argv[2]);
    std::string  userMode = argv[3]; 

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
    
    //send usermode
    std::string modeMsg = userMode + "\n";
    send(sock, modeMsg.c_str(), modeMsg.size(), 0);

    // Create thread to receive messages from server
    SocketInfo* sockInfo = new SocketInfo;
    sockInfo->socket = sock;
    pthread_t receiveThread;
    pthread_create(&receiveThread, NULL, receiveMessages, (void*)sockInfo);
    pthread_detach(receiveThread);

    std::cout << "Connected as " + userMode + "! Type messages (or 'terminate' to quit):" << std::endl;
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