#include <algorithm>
#include <cstddef>
#include <iostream>
#include <utility>

class Buffer {
public:
    explicit Buffer(std::size_t n) : size_(n), data_(new int[n]()) {}
    ~Buffer() { delete[] data_; }

    // Constructeur de copie : un NOUVEAU tableau, avec les mêmes valeurs.
    Buffer(const Buffer &other) : size_(other.size_), data_(new int[other.size_]) {
        std::copy(other.data_, other.data_ + size_, data_);
    }

    // Affectation, idiome copy-and-swap : other est déjà une copie (passage
    // par valeur) ; on échange, et son destructeur libère l'ancien tableau.
    Buffer &operator=(Buffer other) {
        std::swap(size_, other.size_);
        std::swap(data_, other.data_);
        return *this;
    }

    int &operator[](std::size_t i) { return data_[i]; }
    int operator[](std::size_t i) const { return data_[i]; }
    std::size_t size() const { return size_; }

private:
    std::size_t size_;
    int *data_;
};

void print(const char *name, const Buffer &b) {
    std::cout << name << " :";
    for (std::size_t i = 0; i < b.size(); ++i)
        std::cout << ' ' << b[i];
    std::cout << '\n';
}

// Ne pas modifier
int main() {
    std::size_t n = 0;
    if (!(std::cin >> n) || n == 0)
        return 1;
    Buffer a(n);
    for (std::size_t i = 0; i < n; ++i)
        std::cin >> a[i];

    Buffer b = a;          // constructeur de copie
    b[0] += 100;           // ne doit pas modifier a

    Buffer c(1);
    c = a;                 // affectation par copie
    c[n - 1] *= 2;         // ne doit pas modifier a

    Buffer &same = a;
    a = same;              // auto-affectation : a ne doit pas changer

    print("a", a);
    print("b", b);
    print("c", c);
}
