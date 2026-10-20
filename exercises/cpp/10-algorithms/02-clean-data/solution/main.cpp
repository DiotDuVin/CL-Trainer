#include <algorithm>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <vector>

int main() {
    int lo = 0, hi = 0;
    if (!(std::cin >> lo >> hi))
        return 1;
    std::vector<int> v;
    int x = 0;
    while (std::cin >> x)
        v.push_back(x);

    // 1. Idiome erase-remove : la lambda désigne les valeurs à SUPPRIMER.
    v.erase(std::remove_if(v.begin(), v.end(),
                           [lo, hi](int value) { return value < lo || value > hi; }),
            v.end());

    // 2. Conversion : le vecteur de destination doit avoir la bonne taille.
    std::vector<double> deg(v.size());
    std::transform(v.begin(), v.end(), deg.begin(), [](int value) { return value / 10.0; });

    // 3. 0.0 et pas 0 : le type de la valeur initiale est celui de la somme.
    const double sum = std::accumulate(deg.begin(), deg.end(), 0.0);

    std::cout << std::fixed << std::setprecision(1);
    for (std::size_t i = 0; i < deg.size(); ++i)
        std::cout << (i ? " " : "") << deg[i];
    if (!deg.empty())
        std::cout << '\n';
    std::cout << "somme " << sum << '\n';
}
