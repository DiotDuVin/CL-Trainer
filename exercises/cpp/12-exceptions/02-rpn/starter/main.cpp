#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

// TODO : évaluer l'expression ; lancer std::runtime_error en cas d'erreur.
long evaluate(const std::string &expression) {
    (void)expression;
    return 0;
}

// Ne pas modifier
int main() {
    std::string line;
    while (std::getline(std::cin, line)) {
        try {
            std::cout << evaluate(line) << '\n';
        } catch (const std::runtime_error &e) {
            std::cout << "erreur : " << e.what() << '\n';
        }
    }
}
