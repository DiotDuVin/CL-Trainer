#include <iostream>
#include <string>
#include <utility>
#include <vector>

class Tracked {
public:
    explicit Tracked(const std::string &value) : value_(value) {}

    Tracked(const Tracked &other) : value_(other.value_) { ++copies; }

    // noexcept : permet à std::vector de déplacer lors d'un agrandissement
    Tracked(Tracked &&other) noexcept : value_(std::move(other.value_)) {
        other.value_.clear(); // notre contrat : la source est vide
        ++moves;
    }

    Tracked &operator=(const Tracked &other) {
        value_ = other.value_;
        ++copies;
        return *this;
    }

    Tracked &operator=(Tracked &&other) noexcept {
        value_ = std::move(other.value_);
        other.value_.clear();
        ++moves;
        return *this;
    }

    const std::string &value() const { return value_; }

    static inline int copies = 0;
    static inline int moves = 0;

private:
    std::string value_;
};

std::string show(const Tracked &t) {
    return t.value().empty() ? "(vide)" : t.value();
}

// Ne pas modifier
int main() {
    std::vector<std::string> words;
    std::string w;
    while (std::cin >> w)
        words.push_back(w);
    if (words.size() < 2)
        return 1;

    Tracked a(words[0]);
    Tracked b = a;                 // ?
    Tracked c = std::move(a);      // ?

    std::vector<Tracked> v;
    v.reserve(words.size() + 1);   // pas d'agrandissement : comptage exact
    v.push_back(b);                // ?
    v.push_back(std::move(c));     // ?
    for (std::size_t i = 1; i < words.size(); ++i)
        v.emplace_back(words[i]);  // ?

    Tracked d("temporaire");
    d = v[0];                      // ?
    Tracked e("autre");
    e = std::move(d);              // ?

    std::cout << "a=" << show(a) << " b=" << show(b) << " c=" << show(c)
              << " e=" << show(e) << '\n';
    std::cout << "v=";
    for (std::size_t i = 0; i < v.size(); ++i)
        std::cout << (i ? " " : "") << show(v[i]);
    std::cout << "\ncopies=" << Tracked::copies << " déplacements=" << Tracked::moves << '\n';
}
