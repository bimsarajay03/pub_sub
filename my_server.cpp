#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>



class Client{
    public:
    int socket;
    int clientNum;
    int clietnMode; // 0: Publisher | 1: Subscriber
};


void* handleClient(void* arg){
    Client* client = (Client*)arg;
    int clientSocket = client->socket;
    int clientNum = client->clientNum;

    std::cout << "Client #" << clientNum << " connected!" << std::endl;

    char buffer[1024];
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytes = read(clientSocket, buffer, 1023);
        
        // Client disconnected or error
        if (bytes <= 0) {
            std::cout << "Client #" << clientNum << " disconnected." << std::endl;
            break;
        }
        
        buffer[bytes] = '\0';
        std::cout << "Client #" << clientNum << ": " << buffer << std::flush;
        
        if (std::string(buffer) == "terminate\n") {
            std::cout << "Client #" << clientNum << " sent terminate." << std::endl;
            break;
        }
    }
    
    close(clientSocket);
    delete client;  // Clean up allocated memory
    pthread_exit(NULL);
    return NULL;
}




// create server
int main(int argc, char* argv[]) {
    if (argc != 2) return 1;

    int port = std::stoi(argv[1]);

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return 1;
    }

    // Allow socket reuse(for multiple connections)
    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Failed to set socket options" << std::endl;
        return 1;
    }

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        std::cerr << "Failed to bind to port " << port << std::endl;
        return 1;
    }
    
    if (listen(serverSocket, 5) < 0) {
        std::cerr << "Failed to listen on socket" << std::endl;
        return 1;
    }
    
    std::cout << "Server listening on port " << port << "..." << std::endl;
    int clientCounter = 0;


    while (true) {
        socklen_t addrlen = sizeof(serverAddress);
        int newSocket = accept(serverSocket, (struct sockaddr*)&serverAddress, &addrlen);
        
        if (newSocket < 0) {
            std::cerr << "Failed to accept client" << std::endl;
            continue;
        }

        Client* newClient = new Client;
        newClient->socket = newSocket;
        newClient->clientNum = clientCounter++;

        //create thread
        pthread_t thread;
        pthread_create(&thread, NULL, handleClient, (void*)newClient);

        //detatch thread
        pthread_detach(thread);
        
       
    }

    close(serverSocket);
    return 0;
}