#include <cctype>
#include <iostream>
#include <string>

// TODO : choisir le bon type de paramètre (ni copie, ni modification)
//        et compter les voyelles (a e i o u y, majuscules comprises).
int count_vowels(std::string text) {
    (void)text;
    return 0;
}

int main() {
    std::string line;
    while (std::getline(std::cin, line))
        std::cout << count_vowels(line) << '\n';
}
