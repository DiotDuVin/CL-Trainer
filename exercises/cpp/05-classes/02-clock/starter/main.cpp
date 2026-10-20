#include <iomanip>
#include <iostream>

class Clock {
public:
    Clock(int hours, int minutes) {
        // TODO : normaliser et stocker
        (void)hours;
        (void)minutes;
    }

    void add_minutes(int minutes) {
        // TODO
        (void)minutes;
    }

    int hours() const { return 0; }   // TODO
    int minutes() const { return 0; } // TODO

private:
    // TODO : représentation interne (conseil : un seul entier)
};

// TODO : afficher « HH:MM » (deux chiffres chacun)
std::ostream &operator<<(std::ostream &out, const Clock &c) {
    return out << c.hours() << ':' << c.minutes();
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
