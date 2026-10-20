#include <iostream>
#include <string>
#include <vector>

template <typename T>
class MinStack {
public:
    void push(const T &value) {
        items_.push_back(value);
        // mins_ suit items_ : son sommet est le minimum de toute la pile
        if (mins_.empty() || value < mins_.back())
            mins_.push_back(value);
        else
            mins_.push_back(mins_.back());
    }
    void pop() {
        items_.pop_back();
        mins_.pop_back(); // retrouve le minimum d'avant
    }
    const T &top() const { return items_.back(); }
    const T &min() const { return mins_.back(); }
    bool empty() const { return items_.empty(); }

private:
    std::vector<T> items_;
    std::vector<T> mins_; // invariant : mins_[i] = minimum de items_[0..i]
};

template <typename T>
void run(MinStack<T> &stack, const std::string &cmd) {
    if (cmd == "push") {
        T value{};
        std::cin >> value;
        stack.push(value);
    } else if (cmd == "pop") {
        stack.pop();
    } else if (cmd == "top") {
        std::cout << stack.top() << '\n';
    } else if (cmd == "min") {
        std::cout << stack.min() << '\n';
    }
}

// Ne pas modifier
int main() {
    MinStack<int> numbers;
    MinStack<std::string> words;
    std::string type, cmd;
    while (std::cin >> type >> cmd) {
        if (type == "int")
            run(numbers, cmd);
        else
            run(words, cmd);
    }
}
