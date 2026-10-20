#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

std::string format_price(double amount, const std::string &currency = "EUR",
                         int decimals = 2) {
    std::ostringstream os;
    // fixed + setprecision(n) : n chiffres après la virgule (comme %.nf en C)
    os << std::fixed << std::setprecision(decimals) << amount << ' ' << currency;
    return os.str();
}

int main() {
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream in(line);
        double amount = 0;
        std::string currency;
        int decimals = 0;
        if (!(in >> amount))
            continue;
        if (!(in >> currency))
            std::cout << format_price(amount) << '\n';
        else if (!(in >> decimals))
            std::cout << format_price(amount, currency) << '\n';
        else
            std::cout << format_price(amount, currency, decimals) << '\n';
    }
}
