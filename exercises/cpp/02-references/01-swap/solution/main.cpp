#include <iostream>

void swap_values(int &a, int &b) {
    int tmp = a;
    a = b;
    b = tmp;
}

int main() {
    int x = 0, y = 0;
    while (std::cin >> x >> y) {
        swap_values(x, y);
        std::cout << x << ' ' << y << '\n';
    }
}
