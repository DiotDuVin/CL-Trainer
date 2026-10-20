#include <iomanip>
#include <iostream>

class Clock {
public:
    Clock(int hours, int minutes) : total_(normalize(hours * 60 + minutes)) {}

    void add_minutes(int minutes) { total_ = normalize(total_ + minutes); }
    int hours() const { return total_ / 60; }
    int minutes() const { return total_ % 60; }

private:
    static constexpr int kDay = 24 * 60;
    // ((m % n) + n) % n : toujours entre 0 et n - 1, même si m est négatif
    static int normalize(int m) { return ((m % kDay) + kDay) % kDay; }
    int total_; // minutes depuis minuit ; invariant : 0 <= total_ < kDay
};

std::ostream &operator<<(std::ostream &out, const Clock &c) {
    const char fill = out.fill('0'); // fill renvoie l'ancien caractère
    out << std::setw(2) << c.hours() << ':' << std::setw(2) << c.minutes();
    out.fill(fill);                  // on rend le flux dans l'état reçu
    return out;
}

int main() {
    int h = 0, m = 0, delta = 0;
    char colon = 0;
    while (std::cin >> h >> colon >> m >> delta) {
        Clock clock(h, m);
        clock.add_minutes(delta);
        std::cout << clock << '\n';
    }
}
