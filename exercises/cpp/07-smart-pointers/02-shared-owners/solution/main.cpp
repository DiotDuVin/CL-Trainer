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
        if (cmd == "new") {
            std::cin >> i >> name;
            slots[i] = std::make_shared<Resource>(name); // lâche l'ancienne
        } else if (cmd == "copy") {
            std::cin >> i >> j;
            slots[j] = slots[i];                          // partage : compteur + 1
        } else if (cmd == "drop") {
            std::cin >> i;
            slots[i].reset();                             // compteur - 1
        } else if (cmd == "count") {
            std::cin >> i;
            if (!slots[i])
                std::cout << "vide\n";
            else
                std::cout << slots[i]->name() << " : " << slots[i].use_count()
                          << " propriétaire(s)\n";
        }
    }
}
