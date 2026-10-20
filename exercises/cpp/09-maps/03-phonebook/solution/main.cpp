#include <iostream>
#include <string>
#include <unordered_map>

int main() {
    std::unordered_map<std::string, std::string> book; // nom -> numéro
    std::string cmd, name, number;
    while (std::cin >> cmd) {
        if (cmd == "ajout") {
            std::cin >> name >> number;
            const bool existed = book.count(name) != 0;
            book[name] = number; // [] : insère ou remplace
            std::cout << (existed ? "mis à jour" : "ajouté") << '\n';
        } else if (cmd == "cherche") {
            std::cin >> name;
            auto it = book.find(name); // find : ne modifie JAMAIS l'annuaire
            std::cout << (it == book.end() ? "inconnu" : it->second) << '\n';
        } else if (cmd == "suppr") {
            std::cin >> name;
            std::cout << (book.erase(name) ? "supprimé" : "inconnu") << '\n';
        } else if (cmd == "taille") {
            std::cout << book.size() << '\n';
        }
    }
}
