#include <cctype>
#include <iostream>
#include <string>

std::string letters_only(const std::string &word) {
    std::string result;
    for (char c : word) // copie de chaque caractère : suffisant ici
        if (std::isalpha(static_cast<unsigned char>(c)))
            result += c;
    return result;
}

int main() {
    std::string word, best;
    while (std::cin >> word) {
        const auto cleaned = letters_only(word);
        if (cleaned.size() > best.size()) // strict : le premier gagne
            best = cleaned;
    }
    if (best.empty())
        std::cout << "aucun mot\n";
    else
        std::cout << best << " (" << best.size() << ")\n";
}
