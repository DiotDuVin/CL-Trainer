#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

struct Player {
    std::string name;
    int score;
};

int main() {
    std::vector<Player> players;
    Player p;
    while (std::cin >> p.name >> p.score)
        players.push_back(p);

    // TODO : trier avec std::sort et une lambda (score décroissant, puis nom)

    for (std::size_t i = 0; i < players.size(); ++i)
        std::cout << i + 1 << ". " << players[i].name << ' ' << players[i].score << '\n';
}
