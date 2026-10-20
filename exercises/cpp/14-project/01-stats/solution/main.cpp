// main.cpp — utilise le module stats (ne pas modifier)
#include "stats.h"

#include <iomanip>
#include <iostream>
#include <vector>

int main() {
    std::vector<double> values;
    double x;
    while (std::cin >> x)
        values.push_back(x);

    std::cout << "valeurs : " << values.size() << '\n' << std::fixed << std::setprecision(2)
              << "moyenne " << stats::mean(values) << '\n'
              << "médiane " << stats::median(values) << '\n'
              << "écart-type " << stats::stddev(values) << '\n';
}
