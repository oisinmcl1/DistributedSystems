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

// Add to main() before return:
int x = 10, y = 20;

swap_ref(x, y);
std::cout << "After swap_ref: x=" << x << ", y=" << y << "\n";

int p = 30, q = 40;
swap_ptr(&p, &q);
std::cout << "After swap_ptr: p=" << p << ", q=" << q << "\n";
