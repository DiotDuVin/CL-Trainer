#include <iostream>
#include <string>

int main() {
    std::string name;
    if (!(std::cin >> name)) {
        return 1;  // entrée vide : on signale l'échec
    }
    std::cout << "Bonjour, " << name << " !\n";
    return 0;
}
