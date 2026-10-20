#include <iostream>
#include <stdexcept>
#include <string>

// TODO : refuser les caractères en trop après le nombre (std::invalid_argument)
int parse_int(const std::string &text) {
    return std::stoi(text);
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
