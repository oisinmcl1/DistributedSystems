// Part 2.5: The Copy Constructor Trap
// =====================================
// This demonstrates why destructors alone aren't enough.
// Even with stack allocation and automatic destructors,
// we can still have serious problems with COPYING objects.

#include <iostream>
#include <cstring>

//-----------------------------------------------------------------------------
// Message Class - Now WITH a destructor (from Part 2)
//-----------------------------------------------------------------------------
class Message {
private:
    char* data;
    size_t length;

public:
    // Constructor: allocates memory
    Message(const char* msg) {
        length = strlen(msg);
        data = new char[length + 1];
        strcpy(data, msg);
        std::cout << "  [Message created: \"" << data << "\"]" << std::endl;
    }

    // Destructor: frees memory
    ~Message() {
        std::cout << "  [Message destroyed: \"" << data << "\"]" << std::endl;
        delete[] data;
    }

    const char* getData() const {
        return data;
    }
};

//-----------------------------------------------------------------------------
// ClientConnection Class - WITH destructor
//-----------------------------------------------------------------------------
class ClientConnection {
private:
    int client_id;
    std::string client_ip;
    Message* greeting;  // Pointer to dynamically allocated Message

public:
    // Constructor
    ClientConnection(int id, const std::string& ip) 
        : client_id(id), client_ip(ip) {
        greeting = new Message("Welcome to server!");
        std::cout << "  [Client " << client_id << " (" << client_ip 
                  << ") connected]" << std::endl;
    }

    // Destructor: cleans up greeting
    ~ClientConnection() {
        std::cout << "  [Client " << client_id << " disconnecting...]" << std::endl;
        delete greeting;
        std::cout << "  [Client " << client_id << " disconnected]" << std::endl;
    }

    void sendToClient(const char* message) {
        std::cout << "    → Sending to client " << client_id 
                  << ": " << message << std::endl;
    }

    const char* getGreeting() const {
        return greeting->getData();
    }
};

//-----------------------------------------------------------------------------
// PROBLEM 1: The Shallow Copy Disaster
//-----------------------------------------------------------------------------
void demonstrateShallowCopyProblem() {
    std::cout << "\n=== PROBLEM: Shallow Copy with Pointers ===" << std::endl;
    std::cout << "Creating first connection..." << std::endl;
    
    ClientConnection conn1(1, "192.168.1.100");
    
    std::cout << "\nCopying connection (shallow copy)..." << std::endl;
    ClientConnection conn2 = conn1;  // DEFAULT COPY CONSTRUCTOR!
    
    std::cout << "\nBoth connections exist:" << std::endl;
    std::cout << "  conn1 greeting: " << conn1.getGreeting() << std::endl;
    std::cout << "  conn2 greeting: " << conn2.getGreeting() << std::endl;
	
	std::cout << "\nBUT WAIT! Both point to SAME Message object!" << std::endl;
    std::cout << "  conn1.greeting points to: " << (void*)conn1.getGreeting() << std::endl;
    std::cout << "  conn2.greeting points to: " << (void*)conn2.getGreeting() << std::endl;
    
    
    std::cout << "\nNow watch what happens when scope ends..." << std::endl;
    std::cout << "conn2 destructor will run first, deleting the Message." << std::endl;
    std::cout << "Then conn1 destructor will try to delete it AGAIN!" << std::endl;
    std::cout << "CRASH INCOMING..." << std::endl;
    
    // CRASH! When this scope ends:
    // 1. conn2's destructor runs → deletes greeting
    // 2. conn1's destructor runs → tries to delete SAME greeting
    // 3. DOUBLE DELETE = UNDEFINED BEHAVIOR (usually crashes)
}

//-----------------------------------------------------------------------------
// PROBLEM 2: Why the Default Copy Constructor Fails
//-----------------------------------------------------------------------------
void explainDefaultCopyBehavior() {
    std::cout << "\n=== Why This Happens: Default Copy Constructor ===" << std::endl;
    std::cout << "\nC++ automatically generates a copy constructor:" << std::endl;
    std::cout << "  ClientConnection(const ClientConnection& other)" << std::endl;
    std::cout << "      : client_id(other.client_id)," << std::endl;
    std::cout << "        client_ip(other.client_ip)," << std::endl;
    std::cout << "        greeting(other.greeting)  // SHALLOW COPY!" << std::endl;
    std::cout << "\nThis copies the POINTER VALUE, not the object it points to!" << std::endl;
    std::cout << "Result: Both objects have greeting pointing to SAME memory." << std::endl;
}

//-----------------------------------------------------------------------------
// The Problem Visualized
//-----------------------------------------------------------------------------
void visualizeProblem() {
    std::cout << "\n=== Memory Layout Visualization ===" << std::endl;
    std::cout << R"(
Before Copy:
  Stack                    Heap
  ┌─────────────┐         ┌──────────────────┐
  │ conn1       │         │ Message          │
  │  client_id=1│         │  data="Welcome"  │
  │  greeting ──┼────────→│                  │
  └─────────────┘         └──────────────────┘

After Shallow Copy:
  Stack                    Heap
  ┌─────────────┐         ┌──────────────────┐
  │ conn1       │         │ Message          │
  │  client_id=1│    ┌───→│  data="Welcome"  │
  │  greeting ──┼────┘    └──────────────────┘
  └─────────────┘              ↑
  ┌─────────────┐              │
  │ conn2       │              │
  │  client_id=1│              │
  │  greeting ──┼──────────────┘
  └─────────────┘

When Destructors Run:
  1. conn2 destructor: delete greeting → Message freed
  2. conn1 destructor: delete greeting → DOUBLE DELETE!
     )" << std::endl;
}

//-----------------------------------------------------------------------------
// What We Learn
//-----------------------------------------------------------------------------
void whatWeLearn() {
    std::cout << "\n=== Key Lessons from Part 2.5 ===" << std::endl;
    std::cout << "\n1. Stack allocation + destructors solve manual memory leaks" << std::endl;
    std::cout << "2. BUT destructors create a NEW problem with copies" << std::endl;
    std::cout << "3. Default copy constructor does SHALLOW copy of pointers" << std::endl;
    std::cout << "4. Multiple destructors deleting same memory = CRASH" << std::endl;
    std::cout << "\n=== Solutions (Coming in Part 3) ===" << std::endl;
    std::cout << "• Option 1: Write custom copy constructor (deep copy)" << std::endl;
    std::cout << "• Option 2: Delete copy constructor (prevent copying)" << std::endl;
    std::cout << "• Option 3: Use unique_ptr (prevents copying automatically!)" << std::endl;
    std::cout << "• Option 4: Use shared_ptr (reference counting)" << std::endl;
}

//-----------------------------------------------------------------------------
// Main Function
//-----------------------------------------------------------------------------
int main() {
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════════╗\n";
    std::cout << "║  Part 2.5: The Copy Constructor Trap                     ║\n";
    std::cout << "║  Discovering why stack allocation isn't the full answer  ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════╝\n";

    // First, explain the problem conceptually
    visualizeProblem();
    explainDefaultCopyBehavior();

    // WARNING: Commenting out the crash to keep program runnable
    std::cout << "\n=== DEMONSTRATION (Crash Code Commented Out) ===" << std::endl;
    std::cout << "The following code WOULD crash if uncommented:" << std::endl;
    std::cout << R"(
    ClientConnection conn1(1, "192.168.1.100");
    ClientConnection conn2 = conn1;  // Shallow copy
    // Both destructors try to delete same Message → CRASH
    )" << std::endl;

    std::cout << "\nTo see the crash yourself, uncomment the function call:" << std::endl;
    std::cout << "  // demonstrateShallowCopyProblem();  // ← WILL CRASH!" << std::endl;

    // Uncomment this line to see the actual crash:
    // demonstrateShallowCopyProblem();  // WARNING: WILL CRASH!

    whatWeLearn();

    std::cout << "\n╔═══════════════════════════════════════════════════════════╗\n";
    std::cout << "║  Next: Part 3 - unique_ptr solves this automatically!    ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════╝\n\n";

    return 0;
}

/*
COMPILE AND RUN:
  g++ -std=c++17 -o part2.5_copy_problem part2.5_copy_problem.cpp
  ./part2.5_copy_problem

EXPECTED OUTPUT:
  - Visualization of shallow copy problem
  - Explanation of default copy constructor behavior
  - Warning about double-delete crash
  - Key lessons learned

KEY OBSERVATIONS:
1. Stack allocation solves manual memory leaks
2. But creates copy constructor problems
3. Destructors + shallow copies = double delete
4. This motivates smart pointers in Part 3

TRANSITION TO PART 3:
  "unique_ptr prevents copying entirely - no copy constructor!"
  "This eliminates the double-delete problem at compile time."
*/
