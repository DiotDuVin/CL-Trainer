#include <cstddef>
#include <cstdint>
#include <iostream>

int main() {
    std::size_t n = 0;
    if (!(std::cin >> n)) {
        return 1;
    }
    std::int64_t sum = 0;  // 64 bits : la somme peut atteindre 10^17
    for (std::size_t i = 0; i < n; ++i) {
        std::int64_t x = 0;
        if (!(std::cin >> x)) {
            return 1;
        }
        sum += x;
    }
    std::cout << sum << '\n';
    return 0;
}
