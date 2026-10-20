#include <cctype>
#include <cstddef>
#include <iostream>
#include <string>

int main() {
    std::string line;
    while (std::getline(std::cin, line)) {
        std::string initials;
        for (std::size_t i = 0; i < line.size(); ++i) {
            bool starts_word = line[i] != ' ' && (i == 0 || line[i - 1] == ' ');
            if (starts_word) {
                // unsigned char : toupper sur un char négatif est indéfini
                unsigned char c = static_cast<unsigned char>(line[i]);
                initials += static_cast<char>(std::toupper(c));
                initials += '.';
            }
        }
        std::cout << initials << '\n';
    }
}
