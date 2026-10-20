#include <cstddef>
#include <iostream>
#include <vector>

int main() {
    std::size_t r = 0, c = 0;
    if (!(std::cin >> r >> c))
        return 1;
    std::vector<std::vector<int>> m(r, std::vector<int>(c));
    for (auto &row : m)          // & : on remplit les vraies lignes, pas des copies
        for (int &cell : row)
            std::cin >> cell;

    for (std::size_t j = 0; j < c; ++j) {
        for (std::size_t i = 0; i < r; ++i)
            std::cout << (i ? " " : "") << m[i][j];
        std::cout << '\n';
    }
}
