#include <algorithm>
#include <cstddef>
#include <iostream>
#include <utility>
#include <vector>

class Buffer {
public:
    explicit Buffer(std::size_t n) : size_(n), data_(new int[n]()) { ++allocations; }
    ~Buffer() { delete[] data_; }

    Buffer(const Buffer &other) : size_(other.size_), data_(new int[other.size_]) {
        std::copy(other.data_, other.data_ + size_, data_);
        ++allocations;
    }

    Buffer &operator=(const Buffer &other) {
        if (this != &other) {
            Buffer copy(other);           // copie, puis échange
            std::swap(size_, copy.size_);
            std::swap(data_, copy.data_);
        }
        return *this;
    }

    // Déplacement : on vole le tableau, sans allocation, et on vide la source.
    Buffer(Buffer &&other) noexcept : size_(other.size_), data_(other.data_) {
        other.size_ = 0;
        other.data_ = nullptr;
    }

    Buffer &operator=(Buffer &&other) noexcept {
        if (this != &other) {
            delete[] data_;      // le tableau que l'on possédait
            size_ = other.size_; // on vole
            data_ = other.data_;
            other.size_ = 0;     // la source est vide
            other.data_ = nullptr;
        }
        return *this;
    }

    int &operator[](std::size_t i) { return data_[i]; }
    int operator[](std::size_t i) const { return data_[i]; }
    std::size_t size() const { return size_; }

    static inline int allocations = 0;

private:
    std::size_t size_;
    int *data_;
};

void print(const char *name, const Buffer &b) {
    std::cout << name << " :";
    if (b.size() == 0)
        std::cout << " 0";
    for (std::size_t i = 0; i < b.size(); ++i)
        std::cout << ' ' << b[i];
    std::cout << '\n';
}

// Ne pas modifier
int main() {
    std::size_t n = 0;
    if (!(std::cin >> n) || n == 0)
        return 1;
    Buffer a(n);                          // allocation 1
    for (std::size_t i = 0; i < n; ++i)
        std::cin >> a[i];

    Buffer b = std::move(a);              // déplacement : aucune allocation
    Buffer c(1);                          // allocation 2
    c = std::move(b);                     // affectation par déplacement

    std::vector<Buffer> v;
    v.reserve(2);
    v.push_back(std::move(c));            // déplacement dans le vecteur
    v.push_back(Buffer(2));               // allocation 3, puis déplacement

    std::cout << "a : " << a.size() << '\n';
    std::cout << "b : " << b.size() << '\n';
    std::cout << "c : " << c.size() << '\n';
    print("v[0]", v[0]);
    print("v[1]", v[1]);
    std::cout << "allocations : " << Buffer::allocations << '\n';
}
