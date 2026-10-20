#include <iostream>
#include <sstream>
#include <string>

std::string describe(int x) {
    return "entier : " + std::to_string(x);
}

std::string describe(double x) {
    std::ostringstream os;
    os << "réel : " << x;
    return os.str();
}

std::string describe(bool b) {
    return std::string("booléen : ") + (b ? "vrai" : "faux");
}

std::string describe(const std::string &text) {
    return "texte : " + text + " (" + std::to_string(text.size()) + " caractères)";
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
