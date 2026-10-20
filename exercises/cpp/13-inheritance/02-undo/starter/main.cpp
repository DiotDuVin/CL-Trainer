#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

// Interface (ne pas modifier)
class Command {
public:
    virtual ~Command() = default;
    virtual long apply(long v) const = 0;
    virtual long revert(long v) const = 0;
    virtual std::string describe() const = 0;
};

// Modèle (ne pas modifier)
class Add : public Command {
public:
    explicit Add(long amount) : amount_(amount) {}
    long apply(long v) const override { return v + amount_; }
    long revert(long v) const override { return v - amount_; }
    std::string describe() const override { return "add " + std::to_string(amount_); }
private:
    long amount_;
};

// TODO : écrire les classes Mul et Neg.

// TODO : reconnaître aussi « mul N » (N non nul) et « neg ».
std::unique_ptr<Command> make_command(const std::string &line) {
    std::istringstream in(line);
    std::string name;
    long n = 0;
    if (in >> name >> n && name == "add")
        return std::make_unique<Add>(n);
    return nullptr;
}

// Ne pas modifier
int main() {
    long value = 0;
    std::vector<std::unique_ptr<Command>> done;   // historique, du plus ancien au plus récent
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line == "undo") {
            if (done.empty()) {
                std::cout << "rien à annuler\n";
                continue;
            }
            value = done.back()->revert(value);
            done.pop_back();
        } else if (line == "history") {
            std::cout << "historique :";
            if (done.empty())
                std::cout << " (vide)";
            for (std::size_t i = 0; i < done.size(); ++i)
                std::cout << (i == 0 ? " " : " ; ") << done[i]->describe();
            std::cout << '\n';
            continue;
        } else {
            std::unique_ptr<Command> cmd = make_command(line);
            if (!cmd) {
                std::cout << "commande invalide : " << line << '\n';
                continue;
            }
            value = cmd->apply(value);
            done.push_back(std::move(cmd));
        }
        std::cout << "= " << value << '\n';
    }
}
