#include <iostream>
#include <chrono>

class Object {
private:
    int id;
    int data[100];  // Some data to make it non-trivial
public:
    Object(int id) : id(id) {
        std::cout << "  [Constructor] Object " << id << " created\n";
    }
    
    ~Object() {
        std::cout << "  [Destructor] Object " << id << " destroyed\n";
    }
    
    void doWork() {
        data[0] = id * 10;
    }
};

void stack_example() {
    std::cout << "\n=== Stack Allocation ===\n";
    std::cout << "Creating stack object...\n";
    Object obj1(1);
    obj1.doWork();
    std::cout << "End of function - obj1 will be destroyed automatically\n";
}

int main() {
    std::cout << "Program started\n";
    stack_example();
    std::cout << "Back in main - stack object is already destroyed\n\n";
    return 0;
}
