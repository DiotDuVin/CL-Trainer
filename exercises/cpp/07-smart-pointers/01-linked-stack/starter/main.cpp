#include <cstddef>
#include <iostream>
#include <memory>
#include <string>

class Stack {
public:
    void push(int v) {
        (void)v; // TODO
    }
    bool pop() {
        return false; // TODO
    }
    bool empty() const { return true; }    // TODO
    std::size_t size() const { return 0; } // TODO
    int top() const { return 0; }          // TODO (pile non vide)

private:
    struct Node {
        int value;
        std::unique_ptr<Node> next; // chaque nœud possède le suivant
    };
    // TODO : le sommet, et la taille
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
