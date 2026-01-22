#include <iostream>
#include <chrono>

class Object {

private:
    int id;
    int data[100];  // Some data to make it non-trivial

public:
    Object(int id) : id(id) {
        std::cout << "  [Constructor] Object " << id << " created\n\n";
    }

    ~Object() {
        std::cout << "  [Destructor] Object " << id << " destroyed\n\n";
    }

    void doWork() {
        data[0] = id * 10;
        std::cout << "Object " << id << " is doing work: data[0] = " << data[0] << "\n\n";
    }
};

void stack_example() {
    std::cout << "\n=== Stack Allocation ===\n\n";
    std::cout << "Creating stack object...\n\n";

    Object obj1(1);
    obj1.doWork();

    std::cout << "End of function - obj1 will be destroyed automatically\n\n";
}


void heap_example() {
    std::cout << "\n=== Heap Allocation ===\n";
    std::cout << "Creating heap object...\n";

    Object* obj2 = new Object(2);
    obj2->doWork();

    std::cout << "End of function - obj2 still exists!\n";
    // Memory leak if we don't delete!
}

void heap_with_cleanup() {
    std::cout << "\n=== Heap with Proper Cleanup ===\n";
    std::cout << "Creating heap object...\n";

    Object* obj3 = new Object(3);
    obj3->doWork();

    std::cout << "Manually deleting object...\n";
    delete obj3;

    std::cout << "End of function - obj3 properly cleaned up\n";
}


void performance_test() {
    std::cout << "\n=== Performance Test ===\n";
    const int iterations = 1000000;

    // Stack allocation timing
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; i++) {
        Object obj(i);  // Stack
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto stack_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // Heap allocation timing
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; i++) {
        Object* obj = new Object(i);  // Heap
        delete obj;
    }
    end = std::chrono::high_resolution_clock::now();
    auto heap_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Stack: " << stack_time.count() << "ms\n";
    std::cout << "Heap:  " << heap_time.count() << "ms\n";
    std::cout << "Heap is ~" << (heap_time.count() / (float)stack_time.count())
              << "x slower\n";
}


int main() {
    std::cout << "Program started\n";
    stack_example();

    std::cout << "Back in main - stack object is already destroyed\n\n";

    heap_example();
    std::cout << "Back in main - heap object still alive (leaked!)\n";

    heap_with_cleanup();
    std::cout << "Back in main - heap object properly destroyed\n\n";

    performance_test();

    return 0;
}
