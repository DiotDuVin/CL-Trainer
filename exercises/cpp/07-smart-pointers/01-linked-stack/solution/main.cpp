#include <cstddef>
#include <iostream>
#include <memory>
#include <string>

class Stack {
public:
    void push(int v) {
        // Le nouveau nœud prend possession de l'ancien sommet.
        head_ = std::make_unique<Node>(Node{v, std::move(head_)});
        ++size_;
    }
    bool pop() {
        if (!head_)
            return false;
        head_ = std::move(head_->next); // l'ancien sommet est libéré ici
        --size_;
        return true;
    }
    bool empty() const { return head_ == nullptr; }
    std::size_t size() const { return size_; }
    int top() const { return head_->value; }

private:
    struct Node {
        int value;
        std::unique_ptr<Node> next; // chaque nœud possède le suivant
    };
    std::unique_ptr<Node> head_;
    std::size_t size_ = 0;
};

// Ne pas modifier
int main() {
    Stack s;
    std::string cmd;
    while (std::cin >> cmd) {
        if (cmd == "push") {
            int v = 0;
            std::cin >> v;
            s.push(v);
        } else if (cmd == "pop") {
            if (!s.pop())
                std::cout << "vide\n";
        } else if (cmd == "top") {
            if (s.empty())
                std::cout << "vide\n";
            else
                std::cout << s.top() << '\n';
        } else if (cmd == "size") {
            std::cout << s.size() << '\n';
        }
    }
}
