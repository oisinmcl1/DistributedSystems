// C++ Example: Arrays and Explicit Pointers
// Week 1 Setup Lab - Understanding C++'s explicit pointer syntax

#include <iostream>

// VERSION 1: Array notation in parameter
void modifyArray(int data[]) {
    // Despite the [] syntax, 'data' is still just a pointer!
    data[0] = 999;
}

// VERSION 2: Pointer notation in parameter (currently used)
// void modifyArray(int* data) {
//     // 'data' is a pointer to the first element
//     // VERSION 1 and VERSION 2 are IDENTICAL in C++!
//     data[0] = 999;
// }

int main() {
    // Create an array on the stack
    int arr[3] = {10, 20, 30};
    
    std::cout << "=== C++ Array Pointer Example ===" << std::endl;
    std::cout << "Array address: " << &arr << std::endl;
    std::cout << "Before modifyArray:" << std::endl;
    std::cout << "arr[0] = " << arr[0] << std::endl;
    
    // Pass pointer to the array (array name decays to pointer)
    modifyArray(arr);
    
    std::cout << "\nAfter modifyArray:" << std::endl;
    std::cout << "arr[0] = " << arr[0] << std::endl;
    
    // Demonstrate explicit pointer operations
    int* ptr = arr;  // ptr now points to arr[0]
    std::cout << "\nPointer demonstrations:" << std::endl;
    std::cout << "ptr points to: " << *ptr << " (arr[0])" << std::endl;
    std::cout << "ptr[1] = " << ptr[1] << " (same as arr[1])" << std::endl;
    
    std::cout << "\n✓ The array was modified!" << std::endl;
    std::cout << "✓ C++ uses EXPLICIT pointers - you can see and manipulate addresses" << std::endl;
    std::cout << "✓ Array names decay to pointers when passed to functions" << std::endl;
    std::cout << "✓ You CANNOT pass raw arrays by value in C++ (always becomes pointer)" << std::endl;
    std::cout << "\n Try changing the function signature to: void modifyArray(int data[])" << std::endl;
    std::cout << "   What happens? NOTHING changes! Both are identical in C++." << std::endl;
    return 0;
}
