#include <iostream>
#include <string>

class Account {
public:
    explicit Account(long initial) : balance_(initial < 0 ? 0 : initial), operations_(0) {}

    bool deposit(long amount) {
        if (amount <= 0)
            return false;
        balance_ += amount;
        ++operations_;
        return true;
    }

    bool withdraw(long amount) {
        if (amount <= 0 || amount > balance_) // tout vérifier AVANT de modifier
            return false;
        balance_ -= amount;
        ++operations_;
        return true;
    }

    long balance() const { return balance_; }
    int operations() const { return operations_; }

private:
    long balance_;   // invariant : jamais négatif
    int operations_; // opérations acceptées
};

// Reçoit le compte en lecture seule : n'appelle que des méthodes const.
void report(const Account &a) {
    std::cout << "solde " << a.balance() << " (" << a.operations() << " opérations)\n";
}

int main() {
    long initial = 0;
    if (!(std::cin >> initial))
        return 1;
    Account account(initial);
    std::string command;
    while (std::cin >> command) {
        if (command == "solde") {
            report(account);
            continue;
        }
        long amount = 0;
        std::cin >> amount;
        bool ok = command == "depot" ? account.deposit(amount) : account.withdraw(amount);
        std::cout << (ok ? "ok" : "refusé") << '\n';
    }
}
