#include <iostream>
#include <string>
#include <vector>

template <typename T>
class MinStack {
public:
    void push(const T &value) {
        items_.push_back(value); // TODO : maintenir aussi le minimum
    }
    void pop() { items_.pop_back(); } // TODO
    const T &top() const { return items_.back(); }
    const T &min() const { return items_.back(); } // TODO : en temps constant
    bool empty() const { return items_.empty(); }

private:
    std::vector<T> items_;
    // TODO : de quoi connaître le minimum en O(1)
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
