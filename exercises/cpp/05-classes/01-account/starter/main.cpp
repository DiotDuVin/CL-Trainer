#include <iostream>
#include <string>

class Account {
public:
    explicit Account(long initial) {
        // TODO : initialiser les membres (liste d'initialisation)
        (void)initial;
    }

    bool deposit(long amount) {
        // TODO
        (void)amount;
        return false;
    }

    bool withdraw(long amount) {
        // TODO
        (void)amount;
        return false;
    }

    long balance() const { return 0; }  // TODO
    int operations() const { return 0; } // TODO

private:
    // TODO : les membres de données
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
