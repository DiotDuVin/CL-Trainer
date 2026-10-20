// stats.cpp — implémentation du module (à compléter)
#include "stats.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace {  // détails internes : invisibles hors de ce fichier

double square(double x) { return x * x; }

}  // namespace

namespace stats {

double mean(const std::vector<double> &values) {
    if (values.empty())
        return 0;
    return std::accumulate(values.begin(), values.end(), 0.0) / values.size();
}

double median(std::vector<double> values) {
    if (values.empty())
        return 0;
    std::sort(values.begin(), values.end());
    const std::size_t mid = values.size() / 2;
    if (values.size() % 2 == 1)
        return values[mid];
    return (values[mid - 1] + values[mid]) / 2;
}

double stddev(const std::vector<double> &values) {
    if (values.empty())
        return 0;
    const double m = mean(values);
    double sum = 0;
    for (double x : values)
        sum += square(x - m);
    return std::sqrt(sum / values.size());
}

}  // namespace stats
