// library.h — déclaration de la classe (ne pas modifier)
#ifndef LIBRARY_H
#define LIBRARY_H

#include <map>
#include <ostream>
#include <string>

class Library {
public:
    // Ajoute `copies` exemplaires (crée le livre s'il est nouveau).
    void add(const std::string &title, int copies);

    // Emprunte un exemplaire ; false si aucun n'est disponible.
    bool borrow(const std::string &title);

    // Rend un exemplaire ; false si aucun n'est emprunté.
    bool give_back(const std::string &title);

    // Nombre d'exemplaires disponibles (0 pour un livre inconnu).
    int available(const std::string &title) const;

    // « titre : disponibles/total » par ordre alphabétique, ou « (aucun livre) ».
    void print(std::ostream &out) const;

private:
    struct Stock {
        int total = 0;
        int borrowed = 0;
    };
    std::map<std::string, Stock> books_;
};

#endif
