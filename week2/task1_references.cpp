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
    if (a == nullptr || b == nullptr) {
        std::cout << "Error: Null pointer passed to swap_ptr.\n";
        return;
    }

    int temp = *a;
    *a = *b;
    *b = temp;
}


int main() {
    int a = 42;

    std::cout << "Original value: " << a << "\n\n";

    modify_value(a);
    std::cout << "After modify_value: " << a << "\n\n";

    modify_reference(a);
    std::cout << "After modify_reference: " << a << "\n\n";


    // Part B
    int x = 10, y = 20;

    std::cout << "Before swap_ref: x=" << x << ", y=" << y << "\n";
    swap_ref(x, y);
    std::cout << "After swap_ref: x=" << x << ", y=" << y << "\n\n";

    int p = 30, q = 40;

    std::cout << "Before swap_ptr: p=" << p << ", q=" << q << "\n";
    swap_ptr(&p, &q);
    std::cout << "After swap_ptr: p=" << p << ", q=" << q << "\n\n";

    // Uninitialised reference:
    // int& ref;

    // Passing NULL pointer to swap_ptr
    std::cout << "Passing NULL pointer to swap_ptr:\n";
    swap_ptr(nullptr, nullptr);

    // Create null pointer, dereference it
    int* null_ptr = nullptr;
    std::cout << *null_ptr << "\n"; // Undefined behavior?

    return 0;
}
