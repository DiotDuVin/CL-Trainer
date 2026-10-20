#include <iostream>
#include <numeric>

class Fraction {
public:
    Fraction(long num, long den) : num_(num), den_(den) {
        if (den_ < 0) { // le signe est porté par le numérateur
            num_ = -num_;
            den_ = -den_;
        }
        const long g = std::gcd(num_, den_); // toujours positif ; gcd(0, d) == d
        num_ /= g;
        den_ /= g;
    }
    long num() const { return num_; }
    long den() const { return den_; }

private:
    long num_, den_; // invariant : den_ > 0 et pgcd(num_, den_) == 1
};

// Les opérateurs construisent une Fraction : c'est le constructeur qui simplifie.
Fraction operator+(const Fraction &a, const Fraction &b) {
    return Fraction(a.num() * b.den() + b.num() * a.den(), a.den() * b.den());
}

Fraction operator*(const Fraction &a, const Fraction &b) {
    return Fraction(a.num() * b.num(), a.den() * b.den());
}

std::ostream &operator<<(std::ostream &out, const Fraction &f) {
    out << f.num();
    if (f.den() != 1)
        out << '/' << f.den();
    return out;
}

int main() {
    long a = 0, b = 1, c = 0, d = 1;
    char slash1 = 0, op = 0, slash2 = 0;
    while (std::cin >> a >> slash1 >> b >> op >> c >> slash2 >> d) {
        Fraction x(a, b), y(c, d);
        std::cout << (op == '+' ? x + y : x * y) << '\n';
    }
}
