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

int main() {
    int a = 42;
    std::cout << "Original value: " << a << "\n";
    modify_value(a);
    std::cout << "After modify_value: " << a << "\n";
    modify_reference(a);
    std::cout << "After modify_reference: " << a << "\n";
    return 0;
}