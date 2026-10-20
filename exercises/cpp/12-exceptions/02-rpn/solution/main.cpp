#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

// Convertit un jeton en nombre, ou lance « jeton inconnu ». On traduit les
// exceptions de bas niveau de std::stol en une erreur qui a du sens ici.
long to_number(const std::string &token) {
    try {
        std::size_t pos = 0;
        const long value = std::stol(token, &pos);
        if (pos == token.size())
            return value;
    } catch (const std::logic_error &) { // invalid_argument ou out_of_range
    }
    throw std::runtime_error("jeton inconnu : " + token);
}

long evaluate(const std::string &expression) {
    std::vector<long> stack;
    std::istringstream in(expression);
    std::string token;
    while (in >> token) {
        if (token == "+" || token == "-" || token == "*" || token == "/") {
            if (stack.size() < 2)
                throw std::runtime_error("pile insuffisante");
            const long b = stack.back(); // b d'abord : 5 3 - vaut 2
            stack.pop_back();
            const long a = stack.back();
            stack.pop_back();
            if (token == "/" && b == 0)
                throw std::runtime_error("division par zéro");
            stack.push_back(token == "+" ? a + b
                            : token == "-" ? a - b
                            : token == "*" ? a * b
                                           : a / b);
        } else {
            stack.push_back(to_number(token));
        }
    }
    if (stack.size() != 1)
        throw std::runtime_error("expression incomplète");
    return stack.back();
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
