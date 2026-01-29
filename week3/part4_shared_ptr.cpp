// Part 4: Shared Pointers (shared_ptr) - Shared Ownership!
// Compile: g++ -std=c++17 part4_shared_ptr.cpp -o part4_shared_ptr
// Run: ./part4_shared_ptr

#include <iostream>
#include <string>
#include <cstring>
#include <memory>  // For smart pointers
#include <vector>

// Message class - same as before
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
    
    ~Message() {
        delete[] data;
        std::cout << "  [Message destroyed]" << std::endl;
    }
    
    const char* getData() const { return data; }
};

// ClientConnection - same as before
class ClientConnection {
    int client_id;
    std::string client_ip;
public:
    ClientConnection(int id, const std::string& ip) 
        : client_id(id), client_ip(ip) {
        std::cout << "  [Client " << client_id << " connected from " << client_ip << "]" << std::endl;
    }
    
    ~ClientConnection() {
        std::cout << "  [Client " << client_id << " disconnected]" << std::endl;
    }
    
    void sendToClient(const char* response) {
        std::cout << "  [Sending to client " << client_ip << ": " << response << "]" << std::endl;
    }
    
    int getId() const { return client_id; }
    std::string getIp() const { return client_ip; }
};

// Connection pool - multiple threads can share same connection
void simulateConnectionPool() {
    std::vector<std::shared_ptr<ClientConnection>> activeConnections;
    
    // Create shared connections
    auto conn1 = std::make_shared<ClientConnection>(1, "192.168.1.100");
    auto conn2 = std::make_shared<ClientConnection>(2, "192.168.1.101");
    auto conn3 = std::make_shared<ClientConnection>(3, "192.168.1.102");
    
    std::cout << "\n  [Reference count for conn1: " << conn1.use_count() << "]" << std::endl;
    
    // Add to pool (creates additional references)
    activeConnections.push_back(conn1);
    activeConnections.push_back(conn2);
    activeConnections.push_back(conn3);
    
    std::cout << "  [Reference count for conn1: " << conn1.use_count() << "]" << std::endl;
    
    // Another thread wants to use conn1
    std::shared_ptr<ClientConnection> worker_thread = conn1;
    std::cout << "  [Reference count for conn1: " << conn1.use_count() << "]" << std::endl;
    
    // Remove from pool
    std::cout << "\n  [Removing client 1 from pool...]" << std::endl;
    activeConnections.erase(activeConnections.begin());
    std::cout << "  [Reference count for conn1: " << conn1.use_count() << "]" << std::endl;
    std::cout << "  (Still alive because worker thread is using it!)" << std::endl;
    
    // Worker thread done
    std::cout << "\n  [Worker thread releasing conn1...]" << std::endl;
    worker_thread.reset();
    std::cout << "  [Reference count for conn1: " << conn1.use_count() << "]" << std::endl;
    
    // Original owner releases
    std::cout << "\n  [Original owner releasing conn1...]" << std::endl;
    conn1.reset();
    std::cout << "  (conn1 destroyed when reference count reached 0!)" << std::endl;
}

// Broadcast message - multiple connections share same message
void broadcastMessage() {
    // KEY CONCEPT: broadcast_msg is NOT part of ClientConnection class!
    // It's a SEPARATE shared resource that all clients will READ FROM.
    // The Message is owned by this function, not by any individual client.
    // All clients access the SAME Message object without copying its data.
    auto broadcast_msg = std::make_shared<Message>("System announcement");
    
    std::vector<std::shared_ptr<ClientConnection>> clients;
    clients.push_back(std::make_shared<ClientConnection>(10, "192.168.1.110"));
    clients.push_back(std::make_shared<ClientConnection>(11, "192.168.1.111"));
    clients.push_back(std::make_shared<ClientConnection>(12, "192.168.1.112"));
    
    std::cout << "\n  [Broadcasting to " << clients.size() << " clients]" << std::endl;
    std::cout << "  [Message reference count: " << broadcast_msg.use_count() << "]" << std::endl;  // use_count() returns current number of shared_ptr instances sharing ownership
    
    // Share message with all clients (without copying!)
    for (auto& client : clients) {
        client->sendToClient(broadcast_msg->getData());  // getData() returns const char* pointer to message data
    }
    
    // Message stays alive as long as at least one shared_ptr references it
    std::cout << "  [Message still alive until all clients are done]" << std::endl;  // Automatic lifetime management via reference counting
    
}  // <-- broadcast_msg and all clients destroyed here!

int main() {
    std::cout << "\n=====================================" << std::endl;
    std::cout << "PART 4: Shared Pointers (shared_ptr)" << std::endl;
    std::cout << "=====================================\n" << std::endl;
    
    std::cout << "\n[SERVER RUNNING ON: 192.168.1.1:8080]" << std::endl;
    std::cout << "[Managing multiple client connections...]\n" << std::endl;
    
    std::cout << "=== Test 1: Connection Pool (shared ownership) ===" << std::endl;
    simulateConnectionPool();
    
    std::cout << "\n\n=== Test 2: Broadcast message (shared data) ===" << std::endl;
    broadcastMessage();
    
    std::cout << "\n=== Server shutting down ===" << std::endl;
    std::cout << "All memory automatically cleaned up by shared_ptr!" << std::endl;
    std::cout << "Reference counting ensures no memory leaks!\n" << std::endl;
    
    return 0;
}

/*
 * When to use shared_ptr (shared ownership)?
 * -------------------------------------------
 * Use shared_ptr when:
 *    Multiple owners need access to the same object
 *    Lifetime is unclear - object should live until last user is done
 *    Sharing data across threads (e.g., connection pools, caches)
 *    Broadcasting same message to multiple clients
 *    Graph structures with cycles (nodes point to each other)
 * 
 * When to use unique_ptr (exclusive ownership)?
 * ----------------------------------------------
 * Use unique_ptr when:
 *    Only ONE owner at a time (clear ownership)
 *    No sharing needed - object belongs to single owner
 *    Performance matters - zero overhead compared to raw pointer
 *    Transferring ownership (via std::move)

*/
