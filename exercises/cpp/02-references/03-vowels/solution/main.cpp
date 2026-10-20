#include <cctype>
#include <cstddef>
#include <iostream>
#include <string>

// const & : on lit le texte sans le copier, et sans pouvoir le modifier.
int count_vowels(const std::string &text) {
    int count = 0;
    for (std::size_t i = 0; i < text.size(); ++i) {
        int c = std::tolower(static_cast<unsigned char>(text[i]));
        if (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u' || c == 'y')
            ++count;
    }
    return count;
}

int main() {
    std::string line;
    while (std::getline(std::cin, line))
        std::cout << count_vowels(line) << '\n';
}
