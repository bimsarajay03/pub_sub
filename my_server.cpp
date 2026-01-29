#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>

struct TOPICS{
    int topicID;
    std::string topicName;
};


//client table[user_id][type 0:publisher | 1:subscriber][topic_id]
int CLIENT_TABLE[100][2];
int CLIENT_ACTIVE[100];  // Track active clients: 1=active, 0=empty
int CLIENT_SOCKETS[100];  // Store socket file descriptors for each client
TOPICS TOPIC_TABLE[100]; //store conversation topics
int TOPIC_COUNT = 0;  // Track number of topics


// Global mutex for protecting shared resources
pthread_mutex_t coutMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t clientTableMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t topicTableMutex = PTHREAD_MUTEX_INITIALIZER;

class Client{
    public:
    int socket;
    int clientNum;
};

// Function to find or create a topic in TOPIC_TABLE
int getOrCreateTopicId(const std::string& topicName) {
    pthread_mutex_lock(&topicTableMutex);
    
    // Search for existing topic
    for (int i = 0; i < TOPIC_COUNT; i++) {
        if (TOPIC_TABLE[i].topicName == topicName) {
            pthread_mutex_unlock(&topicTableMutex);
            return TOPIC_TABLE[i].topicID;
        }
    }
    
    // Create new topic if not found
    if (TOPIC_COUNT < 100) {
        int newTopicId = TOPIC_COUNT;
        TOPIC_TABLE[newTopicId].topicID = newTopicId;
        TOPIC_TABLE[newTopicId].topicName = topicName;
        TOPIC_COUNT++;
        
        pthread_mutex_lock(&coutMutex);
        std::cout << "New topic created: '" << topicName << "' (ID: " << newTopicId << ")" << std::endl;
        pthread_mutex_unlock(&coutMutex);
        
        pthread_mutex_unlock(&topicTableMutex);
        return newTopicId;
    }
    
    pthread_mutex_unlock(&topicTableMutex);
    return -1;  // Topic table full
}

// Function to broadcast message to all subscribers ON THE SAME TOPIC
void broadcastToSubscribers(int publisherId, const std::string& message) {
    pthread_mutex_lock(&clientTableMutex);
    
    // Get publisher's topic
    int publisherTopicId = CLIENT_TABLE[publisherId][1];
    
    for (int i = 0; i < 100; i++) {
        // Send to active subscribers on the SAME topic only
        if (CLIENT_ACTIVE[i] == 1 && 
            CLIENT_TABLE[i][0] == 1 && 
            CLIENT_TABLE[i][1] == publisherTopicId) {
            
            std::string formattedMsg = "[Publisher #" + std::to_string(publisherId) + "]: " + message;
            send(CLIENT_SOCKETS[i], formattedMsg.c_str(), formattedMsg.size(), 0);
        }
    }
    
    pthread_mutex_unlock(&clientTableMutex);
}


// Server command handler function
void* handleServerCommands(void* arg) {
    std::string command;
    while (true) {
        std::getline(std::cin, command);
        
        if (command == "show_users") {
            pthread_mutex_lock(&clientTableMutex);
            pthread_mutex_lock(&coutMutex);
            
            std::cout << "\n========== ACTIVE CLIENTS ==========\n";
            bool hasActiveClients = false;
            for (int i = 0; i < 100; i++) {
                if (CLIENT_ACTIVE[i] == 1) {
                    hasActiveClients = true;
                    std::string type = (CLIENT_TABLE[i][0] == 0) ? "PUBLISHER" : "SUBSCRIBER";
                    int topicId = CLIENT_TABLE[i][1];
                    std::string topicName = (topicId >= 0 && topicId < TOPIC_COUNT) ? 
                                           TOPIC_TABLE[topicId].topicName : "UNKNOWN";
                    std::cout << "Client #" << i << " - " << type << " - Topic: " << topicName << std::endl;
                }
            }
            if (!hasActiveClients) {
                std::cout << "No active clients.\n";
            }
            std::cout << "====================================\n\n";
            
            pthread_mutex_unlock(&coutMutex);
            pthread_mutex_unlock(&clientTableMutex);
        }
        else if (command == "help") {
            pthread_mutex_lock(&coutMutex);
            std::cout << "\n========== SERVER COMMANDS ==========\n";
            std::cout << "show_users  - Display all active clients and their types\n";
            std::cout << "show_topics - Display all active topics\n";
            std::cout << "help        - Show this help message\n";
            std::cout << "=====================================\n\n";
            pthread_mutex_unlock(&coutMutex);
        }
        else if (command == "show_topics") {
            pthread_mutex_lock(&topicTableMutex);
            pthread_mutex_lock(&coutMutex);
            
            std::cout << "\n========== ACTIVE TOPICS ==========\n";
            if (TOPIC_COUNT == 0) {
                std::cout << "No topics created yet.\n";
            } else {
                for (int i = 0; i < TOPIC_COUNT; i++) {
                    std::cout << "Topic ID " << TOPIC_TABLE[i].topicID << ": " 
                             << TOPIC_TABLE[i].topicName << std::endl;
                }
            }
            std::cout << "===================================\n\n";
            
            pthread_mutex_unlock(&coutMutex);
            pthread_mutex_unlock(&topicTableMutex);
        }
        else if (!command.empty()) {
            pthread_mutex_lock(&coutMutex);
            std::cout << "Unknown command: " << command << " (type 'help' for commands)\n";
            pthread_mutex_unlock(&coutMutex);
        }
    }
    return NULL;
}

void* handleClient(void* arg){
    char buffer[1024];
    Client* client = (Client*)arg;
    int clientSocket = client->socket;
    int clientNum = client->clientNum;

    // Lock before writing 
    pthread_mutex_lock(&coutMutex);
    std::cout << "Client #" << clientNum << " connected!" << std::endl;
    pthread_mutex_unlock(&coutMutex);

    //add client - find first empty space in CLIENT_TABLE
    int clientId = -1;
    pthread_mutex_lock(&clientTableMutex);
    for (int i = 0; i < 100; i++) {
        if (CLIENT_ACTIVE[i] == 0) {  // Found empty spot
            clientId = i;
            CLIENT_ACTIVE[i] = 1;  // Mark as active
            break;
        }
    }
    pthread_mutex_unlock(&clientTableMutex);

    if (clientId == -1) {
        pthread_mutex_lock(&coutMutex);
        std::cout << "Client #" << clientNum << " rejected - server full!" << std::endl;
        pthread_mutex_unlock(&coutMutex);
        close(clientSocket);
        delete client;
        pthread_exit(NULL);
        return NULL;
    }

    // Update client with assigned ID
    client->clientNum = clientId;
    clientNum = clientId;

    //check type - read only the first line
    memset(buffer, 0, sizeof(buffer));
    std::string clientType = "";
    char ch;
    while (read(clientSocket, &ch, 1) > 0) {
        if (ch == '\n') break;  // Stop at newline
        clientType += ch;
    }
    
    //read topic - read the second line
    std::string clientTopic = "";
    while (read(clientSocket, &ch, 1) > 0) {
        if (ch == '\n') break;  // Stop at newline
        clientTopic += ch;
    }
    
    // Get or create topic ID
    int topicId = getOrCreateTopicId(clientTopic);
    if (topicId == -1) {
        pthread_mutex_lock(&coutMutex);
        std::cout << "Client #" << clientId << " rejected - topic table full!" << std::endl;
        pthread_mutex_unlock(&coutMutex);
        
        pthread_mutex_lock(&clientTableMutex);
        CLIENT_ACTIVE[clientId] = 0;
        pthread_mutex_unlock(&clientTableMutex);
        
        close(clientSocket);
        delete client;
        pthread_exit(NULL);
        return NULL;
    }
    
    if (!clientType.empty()) {
        pthread_mutex_lock(&clientTableMutex);
        if (clientType == "PUBLISHER") {
            CLIENT_TABLE[clientId][0] = 0;
            CLIENT_TABLE[clientId][1] = topicId;  // Store topic ID
            CLIENT_SOCKETS[clientId] = clientSocket;  // Store socket
            pthread_mutex_lock(&coutMutex);
            std::cout << "Client #" << clientId << " registered as PUBLISHER on topic '" 
                     << clientTopic << "'" << std::endl;
            pthread_mutex_unlock(&coutMutex);
        } else if (clientType == "SUBSCRIBER") {
            CLIENT_TABLE[clientId][0] = 1;
            CLIENT_TABLE[clientId][1] = topicId;  // Store topic ID
            CLIENT_SOCKETS[clientId] = clientSocket;  // Store socket
            pthread_mutex_lock(&coutMutex);
            std::cout << "Client #" << clientId << " registered as SUBSCRIBER on topic '" 
                     << clientTopic << "'" << std::endl;
            pthread_mutex_unlock(&coutMutex);
        } else {
            pthread_mutex_lock(&coutMutex);
            std::cout << "Client #" << clientId << " sent invalid type: " << clientType << " - rejecting connection" << std::endl;
            pthread_mutex_unlock(&coutMutex);
            
            // Release the client slot
            CLIENT_ACTIVE[clientId] = 0;
            pthread_mutex_unlock(&clientTableMutex);
            
            close(clientSocket);
            delete client;
            pthread_exit(NULL);
            return NULL;
        }
        pthread_mutex_unlock(&clientTableMutex);
    }
    
    // Clear buffer after reading type
    memset(buffer, 0, sizeof(buffer));

    // Check if this is a publisher - store for message routing
    bool isPublisher = false;
    pthread_mutex_lock(&clientTableMutex);
    isPublisher = (CLIENT_TABLE[clientNum][0] == 0);
    pthread_mutex_unlock(&clientTableMutex);

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytes = read(clientSocket, buffer, 1023);
        
        // Client disconnected or error
        if (bytes <= 0) {
            pthread_mutex_lock(&coutMutex);
            std::cout << "Client #" << clientNum << " disconnected." << std::endl;
            pthread_mutex_unlock(&coutMutex);
            break;
        }
        
        buffer[bytes] = '\0';
        
        // Lock before writing
        pthread_mutex_lock(&coutMutex);
        std::cout << "Client #" << clientNum << ": " << buffer << std::flush;
        pthread_mutex_unlock(&coutMutex);
        
        if (std::string(buffer) == "terminate\n") {
            pthread_mutex_lock(&coutMutex);
            std::cout << "Client #" << clientNum << " sent terminate." << std::endl;
            pthread_mutex_unlock(&coutMutex);
            break;
        }
        
        // If this is a publisher, broadcast message to all subscribers
        if (isPublisher) {
            broadcastToSubscribers(clientNum, std::string(buffer));
        }
    }
    
    // Remove client from CLIENT_TABLE
    pthread_mutex_lock(&clientTableMutex);
    CLIENT_ACTIVE[clientNum] = 0;  // Mark as empty
    CLIENT_TABLE[clientNum][0] = -1;  // Clear type
    CLIENT_TABLE[clientNum][1] = -1;  // Clear topic ID
    CLIENT_SOCKETS[clientNum] = -1;  // Clear socket
    pthread_mutex_unlock(&clientTableMutex);
    
    pthread_mutex_lock(&coutMutex);
    std::cout << "Client #" << clientNum << " removed from CLIENT_TABLE" << std::endl;
    pthread_mutex_unlock(&coutMutex);
    
    close(clientSocket);
    delete client;  // Clean up allocated memory
    pthread_exit(NULL);
    return NULL;
}




// create server
int main(int argc, char* argv[]) {
    if (argc != 2) return 1;

    int port = std::stoi(argv[1]);
    
    // Initialize CLIENT_ACTIVE array to 0 (all empty)
    memset(CLIENT_ACTIVE, 0, sizeof(CLIENT_ACTIVE));
    memset(CLIENT_TABLE, -1, sizeof(CLIENT_TABLE));
    memset(CLIENT_SOCKETS, -1, sizeof(CLIENT_SOCKETS));

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
    std::cout << "Type 'help' for server commands.\n" << std::endl;
    
    int clientCounter = 0;

    // Create server command handler thread
    pthread_t commandThread;
    pthread_create(&commandThread, NULL, handleServerCommands, NULL);
    pthread_detach(commandThread);

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

    // Cleanup mutex before exit
    pthread_mutex_destroy(&coutMutex);
    pthread_mutex_destroy(&clientTableMutex);
    pthread_mutex_destroy(&topicTableMutex);
    close(serverSocket);
    return 0;
}