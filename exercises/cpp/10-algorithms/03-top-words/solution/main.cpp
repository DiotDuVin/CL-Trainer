#include <algorithm>
#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>

int main() {
    std::size_t k = 0;
    if (!(std::cin >> k))
        return 1;
    std::map<std::string, int> counts;
    std::string word;
    while (std::cin >> word)
        ++counts[word];

    // Un map est trié par clé : pour trier par fréquence, on passe à un vecteur.
    std::vector<std::pair<std::string, int>> v(counts.begin(), counts.end());
    std::sort(v.begin(), v.end(), [](const auto &a, const auto &b) {
        if (a.second != b.second)
            return a.second > b.second; // fréquence décroissante
        return a.first < b.first;       // puis alphabétique
    });

    for (std::size_t i = 0; i < std::min(k, v.size()); ++i)
        std::cout << v[i].first << ' ' << v[i].second << '\n';
}
