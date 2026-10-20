#include <iostream>
#include <sstream>
#include <string>

std::string describe(int x) {
    return "entier : " + std::to_string(x);
}

// TODO : ajouter les surcharges describe(double) et describe(bool).

std::string describe(const std::string &text) {
    // TODO : "texte : <text> (<n> caractères)"
    (void)text;
    return "";
}

int main() {
    std::string type;
    while (std::cin >> type) {
        if (type == "int") {
            int x = 0;
            std::cin >> x;
            std::cout << describe(x) << '\n';
        } else if (type == "double") {
            double x = 0;
            std::cin >> x;
            std::cout << describe(x) << '\n';
        } else if (type == "bool") {
            bool b = false;
            std::cin >> b; // lit 0 ou 1
            std::cout << describe(b) << '\n';
        } else if (type == "text") {
            std::string text;
            std::getline(std::cin >> std::ws, text); // reste de la ligne
            std::cout << describe(text) << '\n';
        }
    }
}
