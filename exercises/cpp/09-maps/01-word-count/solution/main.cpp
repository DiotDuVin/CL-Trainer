#include <cctype>
#include <iostream>
#include <map>
#include <string>

std::string clean(const std::string &word) {
    std::string result;
    for (char c : word) {
        unsigned char u = static_cast<unsigned char>(c);
        if (std::isalpha(u))
            result += static_cast<char>(std::tolower(u));
    }
    return result;
}

int main() {
    std::map<std::string, int> counts; // trié par clé : ordre alphabétique
    std::string word;
    while (std::cin >> word) {
        const std::string w = clean(word);
        if (!w.empty())
            ++counts[w];               // absent : créé à 0, puis incrémenté
    }
    if (counts.empty())
        std::cout << "aucun mot\n";
    for (const auto &[w, n] : counts)
        std::cout << w << " : " << n << '\n';
}
