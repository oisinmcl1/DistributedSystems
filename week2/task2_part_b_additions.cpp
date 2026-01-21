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

// Update main() to:
int main() {
    std::cout << "Program started\n";
    stack_example();
    std::cout << "Back in main - stack object is already destroyed\n";
    
    heap_example();
    std::cout << "Back in main - heap object still alive (leaked!)\n";
    
    heap_with_cleanup();
    std::cout << "Back in main - heap object properly destroyed\n\n";
    return 0;
}
