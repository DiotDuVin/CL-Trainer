#include <cmath>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

const double kPi = std::acos(-1.0);

// Interface commune : une classe abstraite (ne pas modifier)
class Shape {
public:
    virtual ~Shape() = default;
    virtual std::string name() const = 0;
    virtual double area() const = 0;
    virtual double perimeter() const = 0;
};

// Modèle : une forme complète (ne pas modifier)
class Rectangle : public Shape {
public:
    Rectangle(double w, double h) : w_(w), h_(h) {}
    std::string name() const override { return "rectangle"; }
    double area() const override { return w_ * h_; }
    double perimeter() const override { return 2 * (w_ + h_); }
private:
    double w_, h_;
};

class Circle : public Shape {
public:
    explicit Circle(double r) : r_(r) {}
    std::string name() const override { return "cercle"; }
    double area() const override { return kPi * r_ * r_; }
    double perimeter() const override { return 2 * kPi * r_; }
private:
    double r_;
};

class Triangle : public Shape {
public:
    Triangle(double a, double b, double c) : a_(a), b_(b), c_(c) {}
    std::string name() const override { return "triangle"; }
    double perimeter() const override { return a_ + b_ + c_; }
    double area() const override {
        const double s = perimeter() / 2;     // formule de Héron
        return std::sqrt(s * (s - a_) * (s - b_) * (s - c_));
    }
private:
    double a_, b_, c_;
};

// Ne pas modifier
int main() {
    std::vector<std::unique_ptr<Shape>> shapes;
    std::string kind;
    while (std::cin >> kind) {
        if (kind == "rect") {
            double w, h;
            std::cin >> w >> h;
            shapes.push_back(std::make_unique<Rectangle>(w, h));
        } else if (kind == "circle") {
            double r;
            std::cin >> r;
            shapes.push_back(std::make_unique<Circle>(r));
        } else if (kind == "triangle") {
            double a, b, c;
            std::cin >> a >> b >> c;
            shapes.push_back(std::make_unique<Triangle>(a, b, c));
        }
    }

    double total = 0;
    std::cout << std::fixed << std::setprecision(2);
    for (const auto &s : shapes) {          // aucune connaissance du genre de forme
        std::cout << s->name() << " : aire " << s->area()
                  << ", périmètre " << s->perimeter() << '\n';
        total += s->area();
    }
    std::cout << "aire totale : " << total << '\n';
}
