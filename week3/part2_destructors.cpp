// Part 2: Stack Allocation + Destructors (Better!)
// Compile: g++ -std=c++17 part2_destructors.cpp -o part2_destructors
// Run: ./part2_destructors

#include <iostream>
#include <string>
#include <cstring>

// Message class - WITH destructor
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
    
    // DESTRUCTOR - automatically frees memory!
    ~Message() {
        delete[] data;
        std::cout << "  [Message destroyed]" << std::endl;
    }
    
    const char* getData() const { return data; }
};

// ClientConnection - WITH destructor
class ClientConnection {
    int client_id;
    std::string client_ip;
public:
    ClientConnection(int id, const std::string& ip) 
        : client_id(id), client_ip(ip) {
        std::cout << "  [Client " << client_id << " connected from " << client_ip << "]" << std::endl;
    }
    
    // DESTRUCTOR - automatically closes connection!
    ~ClientConnection() {
        std::cout << "  [Client " << client_id << " disconnected]" << std::endl;
    }
    
    void sendToClient(const char* response) {
        std::cout << "  [Sending to client " << client_ip << ": " << response << "]" << std::endl;
    }
};

// Handle a single client - STACK ALLOCATION (No 'new'!)
void handleClientRequest_v3(int client_id, const std::string& client_ip) {
    ClientConnection conn(client_id, client_ip);  // Stack allocation - no 'new'!
    Message msg("Hello from client");              // Stack allocation - no 'new'!
    
    conn.sendToClient("Welcome to server!");
    
    // No delete needed! Destructors called automatically when function ends
}  // <-- conn and msg destructors called HERE automatically!

// Handle multiple clients with early return
void handleMultipleClients_v3(int num_clients) {
    for (int i = 0; i < num_clients; i++) {
        std::string client_ip = "192.168.1." + std::to_string(100 + i);
        
        ClientConnection conn(i, client_ip);  // Stack allocation
        Message msg("Request data");          // Stack allocation
        
        conn.sendToClient("Processing your request...");
        
        // Simulate error on client 3
        if (i == 3) {
            std::cout << "\n  [ERROR: Server error while processing client " << i << "!]" << std::endl;
            continue;  // Skip to next iteration - but destructors STILL run!
        }
        
        std::cout << "  [Finished processing client " << i << "]" << std::endl;
        // Destructors called automatically at end of loop iteration
    }
}

int main() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "PART 2: Stack Allocation + Destructors" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    std::cout << "\n[SERVER RUNNING ON: 192.168.1.1:8080]" << std::endl;
    std::cout << "[Waiting for client connections...]\n" << std::endl;
    
    std::cout << "=== Test 1: Single client (stack allocation) ===" << std::endl;
    std::cout << "(Client connecting FROM 192.168.1.100)\n" << std::endl;
    handleClientRequest_v3(1, "192.168.1.100");
    std::cout << "  [handleClientRequest_v3 ended]" << std::endl;
    
    std::cout << "\n=== Test 2: Multiple clients with error ===" << std::endl;
    handleMultipleClients_v3(5);
    
    std::cout << "\n=== Server shutting down ===" << std::endl;
    std::cout << "Notice: ALL clients disconnected and messages destroyed!" << std::endl;
    std::cout << "Even client 3 (where error occurred) was cleaned up automatically!\n" << std::endl;
    
    return 0;
}
