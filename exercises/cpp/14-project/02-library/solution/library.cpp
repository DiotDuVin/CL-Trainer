// library.cpp — définition des fonctions membres
#include "library.h"

// Modèle : chaque définition est préfixée par Library::
void Library::add(const std::string &title, int copies) {
    books_[title].total += copies;     // [] crée le livre s'il est nouveau : voulu ici
}

int Library::available(const std::string &title) const {
    auto it = books_.find(title);      // const : [] est interdit, et find n'insère rien
    if (it == books_.end())
        return 0;
    return it->second.total - it->second.borrowed;
}

bool Library::borrow(const std::string &title) {
    auto it = books_.find(title);
    if (it == books_.end() || it->second.borrowed == it->second.total)
        return false;
    ++it->second.borrowed;
    return true;
}

bool Library::give_back(const std::string &title) {
    auto it = books_.find(title);
    if (it == books_.end() || it->second.borrowed == 0)
        return false;
    --it->second.borrowed;
    return true;
}

void Library::print(std::ostream &out) const {
    if (books_.empty())
        out << "(aucun livre)\n";
    for (const auto &[title, stock] : books_)
        out << title << " : " << stock.total - stock.borrowed << '/' << stock.total << '\n';
}
