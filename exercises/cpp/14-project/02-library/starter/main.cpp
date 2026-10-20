// main.cpp — lit les commandes (ne pas modifier)
#include "library.h"

#include <iostream>
#include <sstream>
#include <string>

int main() {
    Library lib;
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream in(line);
        std::string command, title;
        in >> command;
        if (command == "add") {
            int copies = 0;
            in >> copies >> std::ws;          // ws : saute les espaces avant le titre
            std::getline(in, title);
            lib.add(title, copies);
            std::cout << "ok\n";
            continue;
        }
        in >> std::ws;
        std::getline(in, title);
        if (command == "borrow")
            std::cout << (lib.borrow(title) ? "ok" : "indisponible : " + title) << '\n';
        else if (command == "return")
            std::cout << (lib.give_back(title) ? "ok" : "pas emprunté : " + title) << '\n';
        else if (command == "list")
            lib.print(std::cout);
    }
}
