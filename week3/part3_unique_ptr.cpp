// Part 3: Smart Pointers (unique_ptr) - Modern C++!
// Compile: g++ -std=c++17 part3_unique_ptr.cpp -o part3_unique_ptr
// Run: ./part3_unique_ptr

#include <iostream>
#include <string>
#include <cstring>
#include <memory>  // For smart pointers
#include <queue>

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
};

// Handle client with unique_ptr - No manual 'delete' needed!
void handleClientRequest_v4(int client_id, const std::string& client_ip) {
    auto conn = std::make_unique<ClientConnection>(client_id, client_ip);  // Smart pointer!
    auto msg = std::make_unique<Message>("Client request");                 // Smart pointer!
    
    conn->sendToClient("Processing...");
    
    // No delete needed! unique_ptr automatically deletes when out of scope
}  // <-- conn and msg automatically deleted HERE!

//-----------------------------------------------------------------------------
// MOVE SEMANTICS: Why unique_ptr Can't Be Copied
//-----------------------------------------------------------------------------
void demonstrateNoCopying() {
    std::cout << "\n=== unique_ptr prevents copying ===" << std::endl;
    
    auto msg1 = std::make_unique<Message>("Original message");
    
    // This would NOT compile:
    // auto msg2 = msg1;  // ERROR! Cannot copy unique_ptr
    
	std::cout << "  unique_ptr has deleted copy constructor" << std::endl;
    std::cout << "  This prevents the double-delete problem from Part 2.5!" << std::endl;
}

//-----------------------------------------------------------------------------
// MOVE SEMANTICS: Transferring Ownership with std::move
//-----------------------------------------------------------------------------
void demonstrateOwnershipTransfer() {
    std::cout << "\n=== Transferring ownership with std::move ===" << std::endl;
    
    auto msg1 = std::make_unique<Message>("Original message");
    std::cout << "  msg1 owns the Message" << std::endl;
    std::cout << "  msg1.get() = " << msg1.get() << std::endl;
    
    // Transfer ownership using std::move
    auto msg2 = std::move(msg1);  // OK! Ownership transferred
    
    std::cout << "  After std::move(msg1) to msg2:" << std::endl;
    std::cout << "  msg1.get() = " << msg1.get() << " (nullptr)" << std::endl;
    std::cout << "  msg2.get() = " << msg2.get() << " (owns the Message)" << std::endl;
    std::cout << "  msg2->getData() = " << msg2->getData() << std::endl;
    
    // Only msg2's destructor will delete the Message
    // msg1 is now safe (it's nullptr, so deleting it is a no-op)
}

//-----------------------------------------------------------------------------
// MOVE SEMANTICS: Returning unique_ptr from Functions
//-----------------------------------------------------------------------------
std::unique_ptr<Message> createMessage(const char* text) {
    auto msg = std::make_unique<Message>(text);
    std::cout << "  [Created message in createMessage()]" << std::endl;
    return msg;  // Automatic move (RVO - Return Value Optimization)
}

void demonstrateReturningUniquePtr() {
    std::cout << "\n=== Returning unique_ptr from functions ===" << std::endl;
    
    auto msg = createMessage("Hello from function");
    std::cout << "  [Received ownership in caller]" << std::endl;
    std::cout << "  msg->getData() = " << msg->getData() << std::endl;
    
    // Message will be deleted when msg goes out of scope
}

// Message queue simulation - unique_ptr for ownership
void simulateMessageQueue() {
    std::cout << "\n=== Message queue with move semantics ===" << std::endl;
    
    std::queue<std::unique_ptr<Message>> messageQueue;
    
    // Add messages to queue
    for (int i = 0; i < 3; i++) {
        std::string msg_text = "Message " + std::to_string(i);
        messageQueue.push(std::make_unique<Message>(msg_text.c_str()));
    }
    
    std::cout << "\n  [Queue has " << messageQueue.size() << " messages]" << std::endl;
    
    // Process messages (they get automatically deleted when removed from queue)
    while (!messageQueue.empty()) {
        std::unique_ptr<Message> msg = std::move(messageQueue.front());  // Transfer ownership!
        messageQueue.pop();
        
        std::cout << "  [Processing: " << msg->getData() << "]" << std::endl;
        std::cout << "    (Ownership moved from queue to local variable)" << std::endl;
        
        // msg automatically deleted at end of loop iteration
    }
}

int main() {
    std::cout << "\n╔═══════════════════════════════════════════════════════════╗\n";
    std::cout << "║  Part 3: Smart Pointers (unique_ptr)                     ║\n";
    std::cout << "║  Solution to Part 2.5's copy problem!                    ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════╝\n";
    
    std::cout << "\n[SERVER RUNNING ON: 192.168.1.1:8080]" << std::endl;
    std::cout << "[Waiting for client connections...]\n" << std::endl;
    
    std::cout << "=== Test 1: Single client with unique_ptr ===" << std::endl;
    std::cout << "(Client connecting FROM 192.168.1.100)\n" << std::endl;
    handleClientRequest_v4(1, "192.168.1.100");
    std::cout << "  [handleClientRequest_v4 ended - no delete needed!]" << std::endl;
    
    // NEW: Demonstrate why copying is prevented
    demonstrateNoCopying();
    
    // NEW: Show how to transfer ownership
    demonstrateOwnershipTransfer();
    
    // NEW: Show returning from functions
    demonstrateReturningUniquePtr();
    
    std::cout << "\n=== Test 2: Message queue with move semantics ===" << std::endl;
    simulateMessageQueue();
    
    std::cout << "\n╔═══════════════════════════════════════════════════════════╗\n";
    std::cout << "║  Key Lessons from Part 3:                                ║\n";
    std::cout << "║  • unique_ptr prevents copying (no double-delete!)       ║\n";
    std::cout << "║  • Use std::move() to transfer ownership                 ║\n";
    std::cout << "║  • Only ONE owner at a time (exclusive ownership)        ║\n";
    std::cout << "║  • Automatic cleanup when owner goes out of scope        ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════╝\n";
    
    std::cout << "\n=== Server shutting down ===" << std::endl;
    std::cout << "All memory automatically cleaned up by unique_ptr!" << std::endl;
    std::cout << "No manual delete calls anywhere in the code!\n" << std::endl;
    
    return 0;
}
