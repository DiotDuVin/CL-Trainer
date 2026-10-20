#include <algorithm>
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

// TODO : lo si value < lo, hi si hi < value, sinon value (n'utiliser que <)
template <typename T>
T clamp_to(const T &value, const T &lo, const T &hi) {
    (void)lo;
    (void)hi;
    return value;
}

// TODO : nombre de valeurs dans [lo, hi] (n'utiliser que <)
template <typename T>
std::size_t count_in_range(const std::vector<T> &values, const T &lo, const T &hi) {
    (void)values;
    (void)lo;
    (void)hi;
    return 0;
}

// Ne pas modifier
template <typename T>
void process() {
    T lo{}, hi{};
    std::cin >> lo >> hi;
    std::vector<T> values;
    T x{};
    while (std::cin >> x)
        values.push_back(x);
    for (std::size_t i = 0; i < values.size(); ++i)
        std::cout << (i ? " " : "") << clamp_to(values[i], lo, hi);
    std::cout << '\n' << count_in_range(values, lo, hi) << " dans [" << lo << ", " << hi << "]\n";
}

int main() {
    std::string type;
    std::cin >> type;
    if (type == "int")
        process<int>();
    else if (type == "double")
        process<double>();
    else
        process<std::string>();
}
