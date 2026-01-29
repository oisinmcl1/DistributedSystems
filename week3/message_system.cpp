//============================================================================
// Complete Message System Example - Demonstrates All Lab Concepts
// This file shows Parts 1-4 of Lab 02 working together in a complete system
//============================================================================

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <cstring>

//============================================================================
// Message class - represents a client message
//============================================================================
class Message {
    char* data;
    size_t length;
public:
    Message(const char* msg) {
        length = strlen(msg);
        data = new char[length + 1];  // Allocate memory
        strcpy(data, msg);
        std::cout << "Message created: " << data << std::endl;
    }
    
    ~Message() {
        delete[] data;
        std::cout << "Message destroyed" << std::endl;
    }
    
    const char* getData() const { return data; }
};

//============================================================================
// ClientConnection class - represents a client connection
//============================================================================
class ClientConnection {
    int id;
    std::string address;
    Message* greeting;  // Used for Part 2.5 shallow copy demonstration
public:
    ClientConnection(int conn_id, const std::string& addr) 
        : id(conn_id), address(addr), greeting(nullptr) {
        std::cout << "Connection " << id << " opened to " << address << std::endl;
    }
    
    ~ClientConnection() {
        std::cout << "Connection " << id << " closed" << std::endl;
        if (greeting) {
            delete greeting;
        }
    }
    
    void setGreeting(const char* msg) {
        greeting = new Message(msg);
    }
    
    void send(const char* msg) {
        std::cout << "Sending to " << address << ": " << msg << std::endl;
    }
};

//============================================================================
// PART 1: Manual Memory Management (BUGGY!)
//============================================================================

// Process a single client message - VERSION 1 (Buggy!)
void processMessage_v1(int client_id) {
    ClientConnection* conn = new ClientConnection(client_id, "192.168.1.100");
    Message* msg = new Message("Hello from client");
    
    conn->send(msg->getData());
    
    // BUG: Forgot to delete conn and msg!
    // Memory leaks every time this function is called
}

// Process multiple messages - VERSION 2 (Still buggy!)
void processMultipleMessages_v2(int count) {
    for (int i = 0; i < count; i++) {
        ClientConnection* conn = new ClientConnection(i, "192.168.1.100");
        Message* msg = new Message("Request data");
        
        conn->send(msg->getData());
        
        // Simulate error on message 3
        if (i == 3) {
            std::cout << "ERROR: Processing failed!" << std::endl;
            return;  // Early return - leaks connections 0, 1, 2, 3!
        }
        
        delete conn;  // Only reached for i >= 4
        delete msg;
    }
}

//============================================================================
// PART 2: Stack Allocation + Destructors (SAFE!)
//============================================================================

void processMessage_v3(int client_id) {
    ClientConnection conn(client_id, "192.168.1.100");  // Stack allocation
    Message msg("Hello from client");                    // Stack allocation
    
    conn.send(msg.getData());
    
    // No delete needed - destructors called automatically!
}  // Destructors run here: first msg, then conn

//============================================================================
// PART 2.5: The Shallow Copy Problem (CRASH!)
//============================================================================

void demonstrateShallowCopyBug() {
    std::cout << "\n=== DANGER: Shallow Copy Bug ===" << std::endl;
    std::cout << "Creating conn1 with greeting..." << std::endl;
    
    ClientConnection conn1(1, "192.168.1.100");
    conn1.setGreeting("Welcome");
    
    // Default copy constructor does SHALLOW copy
    // Both conn1 and conn2 now point to SAME greeting!
    ClientConnection conn2 = conn1;
    
    std::cout << "Both connections point to same Message in memory!" << std::endl;
    std::cout << "When destructors run, double-delete will crash..." << std::endl;
    
    // When this function ends:
    // conn2 destructor deletes greeting ✓
    // conn1 destructor tries to delete greeting again ✗ CRASH!
}

//============================================================================
// PART 3: unique_ptr - Exclusive Ownership (SAFE!)
//============================================================================

void processMessage_v4(int client_id) {
    auto conn = std::make_unique<ClientConnection>(client_id, "192.168.1.100");
    auto msg = std::make_unique<Message>("Hello with smart pointer");
    
    conn->send(msg->getData());
    
    // No delete needed! Smart pointers clean up automatically
}  // Destructors called automatically

// Demonstrate move semantics - transferring ownership
void demonstrateMoveSemantics() {
    std::cout << "\n=== Move Semantics: Transferring Ownership ===" << std::endl;
    
    auto msg1 = std::make_unique<Message>("Original message");
    std::cout << "msg1 owns the Message" << std::endl;
    
    // Transfer ownership with std::move
    auto msg2 = std::move(msg1);
    
    std::cout << "After std::move(msg1) to msg2:" << std::endl;
    std::cout << "  msg1.get() = " << msg1.get() << " (nullptr)" << std::endl;
    std::cout << "  msg2.get() = " << msg2.get() << " (owns the Message)" << std::endl;
    std::cout << "  msg2->getData() = " << msg2->getData() << std::endl;
    
    // unique_ptr PREVENTS copying at compile time
    // auto msg3 = msg2;  // ERROR: Cannot copy unique_ptr
    std::cout << "\nunique_ptr prevents copying - avoids Part 2.5 double-delete!" << std::endl;
}

// Message queue using unique_ptr
void processMessageQueue() {
    std::vector<std::unique_ptr<Message>> queue;
    
    queue.push_back(std::make_unique<Message>("Task 1"));
    queue.push_back(std::make_unique<Message>("Task 2"));
    queue.push_back(std::make_unique<Message>("Task 3"));
    
    std::cout << "Processing queue..." << std::endl;
    for (auto& msg : queue) {
        std::cout << "  - " << msg->getData() << std::endl;
    }
    
    // All messages automatically deleted when queue goes out of scope!
}

//============================================================================
// PART 4: shared_ptr - Shared Ownership (SAFE!)
//============================================================================

// Connection pool - multiple owners can share connections
class ConnectionPool {
    std::vector<std::shared_ptr<ClientConnection>> connections;
public:
    std::shared_ptr<ClientConnection> getConnection() {
        if (connections.empty()) {
            auto conn = std::make_shared<ClientConnection>(100, "pool.server.com");
            connections.push_back(conn);  // Pool owns it
            std::cout << "Created new connection (ref count = " << conn.use_count() << ")" << std::endl;
            return conn;  // Caller also owns it
        }
        std::cout << "Reusing existing connection (ref count = " << connections[0].use_count() << ")" << std::endl;
        return connections[0];  // Share existing connection
    }
};

void worker(std::shared_ptr<ClientConnection> conn, int id) {
    std::cout << "Worker " << id << " using connection (ref count = " << conn.use_count() << ")" << std::endl;
    conn->send("Worker data");
}

void testConnectionPool() {
    std::cout << "Creating connection pool..." << std::endl;
    ConnectionPool pool;
    
    auto conn1 = pool.getConnection();  // Create connection
    std::cout << "After getting conn1, ref count = " << conn1.use_count() << std::endl;
    
    {
        auto conn2 = conn1;  // Share ownership
        std::cout << "After copying to conn2, ref count = " << conn1.use_count() << std::endl;
        
        worker(conn2, 1);    // Worker also shares
        std::cout << "After passing to worker, ref count = " << conn1.use_count() << std::endl;
    }  // conn2 destroyed, but connection still alive
    
    std::cout << "After conn2 scope ends, ref count = " << conn1.use_count() << std::endl;
}  // Connection destroyed when last shared_ptr goes away

// Broadcasting: Share same message with multiple clients
void demonstrateBroadcast() {
    std::cout << "\n=== Broadcasting Message to Multiple Clients ===" << std::endl;
    
    // Create shared message
    auto broadcast_msg = std::make_shared<Message>("System announcement");
    std::cout << "Broadcast message ref count = " << broadcast_msg.use_count() << std::endl;
    
    // Create client connections
    std::vector<std::shared_ptr<ClientConnection>> clients;
    clients.push_back(std::make_shared<ClientConnection>(10, "192.168.1.110"));
    clients.push_back(std::make_shared<ClientConnection>(11, "192.168.1.111"));
    clients.push_back(std::make_shared<ClientConnection>(12, "192.168.1.112"));
    
    // All clients share same message - no copying needed!
    for (auto& client : clients) {
        client->send(broadcast_msg->getData());
    }
    
    std::cout << "All clients received shared message (no copying!)" << std::endl;
}  // Message deleted when all shared_ptrs destroyed

//============================================================================
// MAIN - Run all demonstrations
//============================================================================

int main() {
    std::cout << "╔═══════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  Complete Message System - All Lab Parts Demonstrated    ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════╝" << std::endl;
    
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "PART 1: Manual new/delete - Memory Leaks" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    processMessage_v1(1);
    std::cout << "⚠️  Notice: No \"destroyed\" messages - MEMORY LEAK!" << std::endl;
    
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "PART 1: Multiple messages with error handling" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    processMultipleMessages_v2(5);
    std::cout << "⚠️  Notice: Connections 0-3 leaked when error occurred!" << std::endl;
    
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "PART 2: Stack allocation - Automatic cleanup" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    processMessage_v3(10);
    std::cout << "✓ Notice: Destructors called automatically - NO LEAK!" << std::endl;
    
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "PART 2.5: Shallow Copy Bug - DANGER!" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "⚠️  COMMENTED OUT: Would crash with double-delete!" << std::endl;
    std::cout << "⚠️  Uncomment demonstrateShallowCopyBug() to see the crash" << std::endl;
    // demonstrateShallowCopyBug();  // UNCOMMENT TO SEE CRASH!
    
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "PART 3: unique_ptr - Heap + Automatic cleanup" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    processMessage_v4(20);
    std::cout << "✓ Notice: Smart pointers cleaned up automatically!" << std::endl;
    
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "PART 3: Move semantics - Ownership transfer" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    demonstrateMoveSemantics();
    
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "PART 3: Message queue with unique_ptr" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    processMessageQueue();
    std::cout << "✓ All messages cleaned up when queue destroyed" << std::endl;
    
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "PART 4: shared_ptr - Connection pool with reference counting" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    testConnectionPool();
    std::cout << "✓ Connection shared safely between multiple owners" << std::endl;
    
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "PART 4: Broadcasting - Shared message example" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    demonstrateBroadcast();
    std::cout << "✓ One message shared by all clients - efficient!" << std::endl;
    
    std::cout << "\n╔═══════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  Program ending - All resources cleaned up automatically ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════╝" << std::endl;
    
    return 0;
}
