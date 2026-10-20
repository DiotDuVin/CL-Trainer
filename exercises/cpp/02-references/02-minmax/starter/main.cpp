#include <iostream>

// TODO : mettre à jour low et high avec value.
//        Quels paramètres doivent être des références ?
void update_min_max(int value, int low, int high) {
    (void)value;
    (void)low;
    (void)high;
}

int main() {
    int value = 0;
    if (!(std::cin >> value)) {
        std::cout << "vide\n";
        return 0;
    }
    int low = value, high = value; // la première valeur, et pas 0 !
    while (std::cin >> value)
        update_min_max(value, low, high);
    std::cout << low << ' ' << high << '\n';
}
