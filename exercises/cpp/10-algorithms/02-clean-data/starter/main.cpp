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

    // TODO 1 : supprimer les valeurs hors de [lo, hi] (erase + remove_if)
    // TODO 2 : convertir en degrés (std::transform vers un std::vector<double>)
    std::vector<double> deg;
    // TODO 3 : la somme (std::accumulate)
    double sum = 0;

    std::cout << std::fixed << std::setprecision(1);
    for (std::size_t i = 0; i < deg.size(); ++i)
        std::cout << (i ? " " : "") << deg[i];
    if (!deg.empty())
        std::cout << '\n';
    std::cout << "somme " << sum << '\n';
    (void)lo;
    (void)hi;
}
