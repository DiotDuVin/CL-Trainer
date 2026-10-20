#include <iostream>
#include <string>

class Scope {
public:
    explicit Scope(const std::string &name) : name_(name) {
        // TODO : afficher "> nom" indenté, puis augmenter la profondeur
    }
    ~Scope() {
        // TODO : diminuer la profondeur, puis afficher "< nom" indenté
    }

private:
    std::string name_;
    // TODO : la profondeur, partagée par tous les Scope
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
