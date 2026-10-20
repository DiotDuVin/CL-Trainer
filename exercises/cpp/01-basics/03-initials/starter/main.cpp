#include <cctype>
#include <iostream>
#include <string>

int main() {
    std::string line;
    while (std::getline(std::cin, line)) {
        std::string initials;
        // TODO : ajouter à initials la première lettre de chaque mot
        //        (en majuscule), suivie d'un point.
        std::cout << initials << '\n';
    }
}
