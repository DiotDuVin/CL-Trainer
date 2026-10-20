#include <array>
#include <cstddef>
#include <iostream>

int main() {
    std::array<int, 10> counts{}; // {} : tous les compteurs à zéro
    char c = 0;
    while (std::cin.get(c))
        if (c >= '0' && c <= '9')
            ++counts[static_cast<std::size_t>(c - '0')];

    bool any = false;
    for (std::size_t d = 0; d < counts.size(); ++d) {
        if (counts[d] > 0) {
            std::cout << d << " : " << counts[d] << '\n';
            any = true;
        }
    }
    if (!any)
        std::cout << "aucun chiffre\n";
}
