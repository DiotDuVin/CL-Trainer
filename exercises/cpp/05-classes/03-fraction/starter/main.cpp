#include <iostream>
#include <numeric>

// TODO : la classe Fraction (toujours irréductible, dénominateur > 0),
//        les opérateurs + et *, et operator<<.
class Fraction {
public:
    Fraction(long num, long den) : num_(num), den_(den) {}
    long num() const { return num_; }
    long den() const { return den_; }

private:
    long num_, den_;
};

Fraction operator+(const Fraction &a, const Fraction &) { return a; } // TODO
Fraction operator*(const Fraction &a, const Fraction &) { return a; } // TODO

std::ostream &operator<<(std::ostream &out, const Fraction &f) {
    return out << f.num() << '/' << f.den(); // TODO
}

int main() {
    long a = 0, b = 1, c = 0, d = 1;
    char slash1 = 0, op = 0, slash2 = 0;
    while (std::cin >> a >> slash1 >> b >> op >> c >> slash2 >> d) {
        Fraction x(a, b), y(c, d);
        std::cout << (op == '+' ? x + y : x * y) << '\n';
    }
}
