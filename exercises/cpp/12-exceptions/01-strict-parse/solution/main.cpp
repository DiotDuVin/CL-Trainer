#include <iostream>
#include <stdexcept>
#include <string>

int parse_int(const std::string &text) {
    std::size_t pos = 0;
    const int value = std::stoi(text, &pos); // lance invalid_argument ou out_of_range
    if (pos != text.size())
        throw std::invalid_argument("caractères en trop après le nombre");
    return value;
}

// Ne pas modifier
int main() {
    std::string line;
    while (std::getline(std::cin, line)) {
        try {
            std::cout << parse_int(line) << '\n';
        } catch (const std::invalid_argument &) {
            std::cout << "pas un nombre\n";
        } catch (const std::out_of_range &) {
            std::cout << "hors limites\n";
        }
    }
}
