#include <array>
#include <iostream>
#include <memory>
#include <string>

class Resource {
public:
    explicit Resource(const std::string &name) : name_(name) {}
    ~Resource() { std::cout << "libération " << name_ << '\n'; }
    const std::string &name() const { return name_; }

private:
    std::string name_;
};

int main() {
    std::array<std::shared_ptr<Resource>, 5> slots; // 5 emplacements, vides au départ
    std::string cmd, name;
    std::size_t i = 0, j = 0;
    while (std::cin >> cmd) {
        // TODO : traiter new / copy / drop / count
        (void)name;
        (void)i;
        (void)j;
    }
}
