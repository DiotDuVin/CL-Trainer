#include <iostream>
#include <vector>

int main() {
    std::vector<int> notes;
    int n = 0;
    while (std::cin >> n)
        notes.push_back(n);

    long long sum = 0;
    for (int x : notes)
        sum += x;
    const double average = notes.empty() ? 0.0
                           : static_cast<double>(sum) / static_cast<double>(notes.size());

    bool first = true;
    for (int x : notes) {
        if (x > average) {
            std::cout << (first ? "" : " ") << x;
            first = false;
        }
    }
    std::cout << (first ? "aucune" : "") << '\n';
}
