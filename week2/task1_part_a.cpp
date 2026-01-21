#include <iostream>

// Pass by value - makes a copy
void modify_value(int x) {
    x = 100;
    std::cout << "Inside modify_value: " << x << "\n";
}

// Pass by reference - modifies original
void modify_reference(int& x) {
    x = 100;
    std::cout << "Inside modify_reference: " << x << "\n";
}


// Using reference - cannot be null, must be initialized
void swap_ref(int& a, int& b) {
    int temp = a;
    a = b;
    b = temp;
}

// Using pointers - can be null, can be reassigned
void swap_ptr(int* a, int* b) {
    if (a == nullptr || b == nullptr) return;
    int temp = *a;
    *a = *b;
    *b = temp;
}


int main() {
    int a = 42;
    
    std::cout << "Original value: " << a << "\n";
    
    modify_value(a);
    std::cout << "After modify_value: " << a << "\n";
    
    modify_reference(a);
    std::cout << "After modify_reference: " << a << "\n";
    
    return 0;
}
