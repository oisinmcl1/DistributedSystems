// Part 1: Manual Memory Management (The Buggy Way)
// Compile: g++ -std=c++17 part1_manual.cpp -o part1_manual
// Run: ./part1_manual

#include <iostream>
#include <string>
#include <cstring>

// Message class - represents a message from a client
class Message {
    char* data;
    size_t length;
public:
    Message(const char* msg) {
        length = strlen(msg);
        data = new char[length + 1];
        strcpy(data, msg);
        std::cout << "  [Message created: " << data << "]" << std::endl;
    }
    
    // NO DESTRUCTOR - memory will leak!
    
    const char* getData() const { return data; }
};

// ClientConnection - represents a connection from a client to our server
class ClientConnection {
    int client_id;
    std::string client_ip;
public:
    ClientConnection(int id, const std::string& ip) 
        : client_id(id), client_ip(ip) {
        std::cout << "  [Client " << client_id << " connected from " << client_ip << "]" << std::endl;
    }
    
    // NO DESTRUCTOR - connection will leak!
    
    void sendToClient(const char* response) {
        std::cout << "  [Sending to client " << client_ip << ": " << response << "]" << std::endl;
    }
};

// Handle a single client request - VERSION 1 (Leaky!)
void handleClientRequest_v1(int client_id, const std::string& client_ip) {
    ClientConnection* conn = new ClientConnection(client_id, client_ip);
    Message* msg = new Message("Hello from client");
    
    conn->sendToClient("Welcome to server!");
    
    // BUG: Forgot to delete conn and msg!
    // Memory leaks every time this function is called
}

// Handle multiple client requests - VERSION 2 (Partially leaky!)
void handleMultipleClients_v2(int num_clients) {
    for (int i = 0; i < num_clients; i++) {
        std::string client_ip = "192.168.1." + std::to_string(100 + i);
        
        ClientConnection* conn = new ClientConnection(i, client_ip);
        Message* msg = new Message("Request data");
        // Simulate error on client 3
        if (i == 3) {
            std::cout << "\n  [ERROR: Server error while processing client " << i << "!]" << std::endl;
            continue;  // BUG: Skip cleanup - client 3 leaks!
        }
        conn->sendToClient("Processing your request...");
        // Proper cleanup (but only reached if no error)
        delete conn;
        delete msg;
        std::cout << "  [Cleaned up client " << i << "]" << std::endl;
    }
}

int main() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "PART 1: Manual Memory Management" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    std::cout << "\n[SERVER RUNNING ON: 192.168.1.1:8080]" << std::endl;
    std::cout << "[Waiting for client connections...]\n" << std::endl;
    
    std::cout << "=== Test 1: Single client request ===" << std::endl;
    std::cout << "(Client connecting FROM 192.168.1.100)\n" << std::endl;
    handleClientRequest_v1(1, "192.168.1.100");
    std::cout << "  [handleClientRequest_v1 ended - did we clean up?]" << std::endl;
    
    std::cout << "\n=== Test 2: Multiple clients with error ===" << std::endl;
    handleMultipleClients_v2(5);
    
    std::cout << "\n=== Server shutting down ===" << std::endl;
    std::cout << "MEMORY LEAK ANALYSIS:" << std::endl;
    std::cout << "  - Test 1 (client 1): LEAKED - never deleted!" << std::endl;
    std::cout << "  - Test 2 (clients 0-2, 4): Cleaned up properly" << std::endl;
    std::cout << "  - Test 2 (client 3): LEAKED - error skipped cleanup!\n" << std::endl;
    std::cout << "Total leaks: 2 connections + 2 messages\n" << std::endl;
    
    return 0;
}
