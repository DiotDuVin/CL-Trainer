#include <iostream>

// TODO : échanger les valeurs de a et b. Les paramètres doivent être des
//        références pour que l'échange soit visible dans main.
void swap_values(int a, int b) {
    (void)a; // (void) : évite l'avertissement « paramètre inutilisé »
    (void)b;
}

int main() {
    int x = 0, y = 0;
    while (std::cin >> x >> y) {
        swap_values(x, y);
        std::cout << x << ' ' << y << '\n';
    }
}
