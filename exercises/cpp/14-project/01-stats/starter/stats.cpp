// stats.cpp — implémentation du module (à compléter)
#include "stats.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace {  // détails internes : invisibles hors de ce fichier

double square(double x) { return x * x; }

}  // namespace

namespace stats {

// TODO : écrire les trois fonctions (et utiliser square dans stddev).
double mean(const std::vector<double> &values) {
    (void)values;
    return 0;
}

double median(std::vector<double> values) {
    (void)values;
    return 0;
}

double stddev(const std::vector<double> &values) {
    (void)values;
    return 0;
}

}  // namespace stats
