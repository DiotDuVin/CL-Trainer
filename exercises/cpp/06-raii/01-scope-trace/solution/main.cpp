#include <iostream>
#include <string>

class Scope {
public:
    explicit Scope(const std::string &name) : name_(name) {
        std::cout << std::string(2 * depth_, ' ') << "> " << name_ << '\n';
        ++depth_; // après l'affichage
    }
    ~Scope() {
        --depth_; // avant l'affichage : même niveau que la ligne d'entrée
        std::cout << std::string(2 * depth_, ' ') << "< " << name_ << '\n';
    }

private:
    std::string name_;
    static inline int depth_ = 0; // C++17 : un seul compteur pour toute la classe
};

// Ne pas modifier
void visit(int n, int stop) {
    Scope s("visit " + std::to_string(n));
    if (n == stop)
        return;
    if (n > 0)
        visit(n - 1, stop);
}

int main() {
    int n = 0, stop = 0;
    while (std::cin >> n >> stop)
        visit(n, stop);
}
