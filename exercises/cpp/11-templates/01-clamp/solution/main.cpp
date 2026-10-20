#include <algorithm>
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

// N'utilise que < : la seule exigence imposée au type T.
template <typename T>
T clamp_to(const T &value, const T &lo, const T &hi) {
    if (value < lo)
        return lo;
    if (hi < value)
        return hi;
    return value;
}

template <typename T>
std::size_t count_in_range(const std::vector<T> &values, const T &lo, const T &hi) {
    return static_cast<std::size_t>(std::count_if(values.begin(), values.end(),
        [&lo, &hi](const T &x) { return !(x < lo) && !(hi < x); })); // lo <= x <= hi
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
