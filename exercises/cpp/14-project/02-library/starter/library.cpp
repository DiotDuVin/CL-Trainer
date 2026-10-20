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

// TODO : écrire borrow, give_back et print.
bool Library::borrow(const std::string &title) {
    (void)title;
    return false;
}

bool Library::give_back(const std::string &title) {
    (void)title;
    return false;
}

void Library::print(std::ostream &out) const {
    (void)out;
}
