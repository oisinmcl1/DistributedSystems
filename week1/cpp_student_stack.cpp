// C++ Example: Student Class (Stack Allocation - Automatic Cleanup)
// Week 1 Setup Lab - Understanding C++'s stack-based memory management

#include <iostream>
#include <string>

class Student {
    std::string name;
    int age;
    
public:
    // Constructor
    Student(std::string n, int a) : name(n), age(a) {
        std::cout << "  → Constructor called for " << name << std::endl;
    }
    
    // Destructor (called automatically when object goes out of scope)
    ~Student() {
        std::cout << "  → Destructor called for " << name << std::endl;
    }
    
    void display() {
        std::cout << "Student: " << name << ", " << age << std::endl;
    }
};

int main() {
    std::cout << "=== C++ Student Example (Stack Allocation) ===\n" << std::endl;
    
    std::cout << "Creating student on STACK:" << std::endl;
    // NO 'new' keyword - object created on STACK
    Student s("Alice", 20);
    
    std::cout << "\nDisplaying student:" << std::endl;
    s.display();
    
    std::cout << "\n✓ Object created on stack (no 'new' keyword)" << std::endl;
    std::cout << "✓ Destructor will be called AUTOMATICALLY at end of scope" << std::endl;
    std::cout << "✓ No manual cleanup needed (RAII - Resource Acquisition Is Initialization)" << std::endl;
    std::cout << "✓ Deterministic cleanup (exactly when '}' is reached)" << std::endl;
    
    std::cout << "\nEnd of main() - destructor will be called now:" << std::endl;
    
    return 0;
    // Destructor automatically called here when 's' goes out of scope
}
