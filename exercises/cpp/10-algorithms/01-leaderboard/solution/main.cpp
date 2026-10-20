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

    std::sort(players.begin(), players.end(), [](const Player &a, const Player &b) {
        if (a.score != b.score)
            return a.score > b.score; // score décroissant
        return a.name < b.name;       // puis nom croissant
    });

    for (std::size_t i = 0; i < players.size(); ++i)
        std::cout << i + 1 << ". " << players[i].name << ' ' << players[i].score << '\n';
}
