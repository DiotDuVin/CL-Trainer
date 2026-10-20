#include <iostream>
#include <map>
#include <set>
#include <string>

int main() {
    std::map<std::string, std::set<std::string>> days; // date -> visiteurs distincts
    std::set<std::string> everyone;                      // distincts sur toute la période
    std::string date, visitor;
    while (std::cin >> date >> visitor) {
        days[date].insert(visitor); // crée l'ensemble au premier accès ; ignore les doublons
        everyone.insert(visitor);
    }
    for (const auto &[day, visitors] : days)
        std::cout << day << " : " << visitors.size() << '\n';
    std::cout << "total : " << everyone.size() << '\n';
}
