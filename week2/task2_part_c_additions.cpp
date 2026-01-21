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

// Call performance_test() at the end of main()
