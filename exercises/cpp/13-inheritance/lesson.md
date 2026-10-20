id: cpp.lesson.inheritance
title: Héritage et polymorphisme
lang: cpp
topic: inheritance
estimated_min: 30
---
## Ce que tu connais en C

trainer lui-même gère plusieurs langages avec une **table de pointeurs de
fonctions** : chaque langage fournit sa propre version de « compiler » et
« exécuter », et le moteur appelle la bonne sans savoir de quel langage il
s'agit.

```c
struct shape_ops {
    double (*area)(const void *self);
    const char *(*name)(const void *self);
};

struct shape {
    const struct shape_ops *ops;   /* « quel genre de forme suis-je ? » */
    void *data;                    /* les données propres à ce genre */
};

double a = s->ops->area(s->data);  /* appelle la bonne fonction */
```

Ça marche, mais tout est à la main : remplir la table, caster `void *`,
vérifier qu'aucune entrée n'est `NULL`. Le C++ fait exactement ce montage
pour toi, avec vérification des types : c'est le **polymorphisme** par
**fonctions virtuelles**.

## L'héritage : une classe qui en étend une autre

```cpp example
#include <iostream>
#include <string>

class Animal {
public:
    explicit Animal(const std::string &name) : name_(name) {}
    const std::string &name() const { return name_; }
private:
    std::string name_;
};

class Dog : public Animal {             // Dog EST UN Animal
public:
    explicit Dog(const std::string &name) : Animal(name) {}   // construit la partie Animal
    void fetch() const { std::cout << name() << " rapporte la balle\n"; }
};

int main() {
    Dog rex("Rex");
    std::cout << rex.name() << '\n';    // hérité d'Animal
    rex.fetch();                        // propre à Dog
}
```

```output
Rex
Rex rapporte la balle
```

- **`class Dog : public Animal`** : `Dog` possède tout ce qu'a `Animal`
  (données et fonctions), plus ce qu'il ajoute. On écrit presque toujours
  `public` ; l'héritage `private` existe mais est rare.
- **Le constructeur de la classe dérivée appelle celui de la base** dans sa
  liste d'initialisation : `: Animal(name)`.
- **`private` reste privé** : `Dog` ne peut pas lire `name_` directement, il
  passe par `name()`. Le mot-clé `protected` ouvre l'accès aux classes
  dérivées seulement (à utiliser avec modération).

## `virtual` : choisir la fonction à l'exécution

Tout l'intérêt apparaît quand on manipule des objets dérivés **à travers une
référence ou un pointeur vers la base** :

```cpp example
#include <iostream>

class Shape {
public:
    virtual ~Shape() = default;                       // indispensable, voir plus bas
    virtual double area() const = 0;                  // virtuelle PURE : pas de corps
    virtual const char *name() const { return "forme"; }   // virtuelle avec un défaut
};

class Square : public Shape {
public:
    explicit Square(double side) : side_(side) {}
    double area() const override { return side_ * side_; }
    const char *name() const override { return "carré"; }
private:
    double side_;
};

class Circle : public Shape {
public:
    explicit Circle(double r) : r_(r) {}
    double area() const override { return 3.14 * r_ * r_; }
    // name() n'est pas redéfinie : la version de Shape est utilisée
private:
    double r_;
};

void describe(const Shape &s) {        // ne connaît QUE Shape
    std::cout << s.name() << " : " << s.area() << '\n';
}

int main() {
    Square sq(3);
    Circle c(1);
    describe(sq);
    describe(c);
}
```

```output
carré : 9
forme : 3.14
```

- **`virtual`** : l'appel `s.area()` est résolu **à l'exécution**, selon le
  type réel de l'objet. Le compilateur construit pour chaque classe une table
  de pointeurs de fonctions (la *vtable*), exactement comme `shape_ops`.
- **`= 0`** rend la fonction **virtuelle pure** : la classe devient
  **abstraite**, on ne peut pas créer d'objet `Shape`. Chaque classe dérivée
  doit fournir `area()`, sinon elle est abstraite à son tour.
- **`override`** demande au compilateur de **vérifier** que la fonction
  redéfinit bien une fonction virtuelle de la base. Une faute de frappe ou un
  `const` oublié devient une erreur au lieu d'un bug silencieux.

### Sans `virtual`, pas de polymorphisme

```cpp example
#include <iostream>

struct Base {
    void hello() const { std::cout << "Base\n"; }            // PAS virtuelle
    virtual void bye() const { std::cout << "Base\n"; }
    virtual ~Base() = default;
};

struct Derived : Base {
    void hello() const { std::cout << "Derived\n"; }         // masque, ne redéfinit pas
    void bye() const override { std::cout << "Derived\n"; }
};

int main() {
    Derived d;
    const Base &ref = d;
    ref.hello();    // choisi d'après le type de la RÉFÉRENCE : Base
    ref.bye();      // choisi d'après le type de l'OBJET : Derived
}
```

```output
Base
Derived
```

## Une collection d'objets différents

Pour stocker des formes de genres différents dans un même conteneur, on stocke
des **pointeurs vers la base**. Avec le chapitre 7 : des `std::unique_ptr`.

```cpp example
#include <iostream>
#include <memory>
#include <vector>

class Shape {
public:
    virtual ~Shape() = default;
    virtual double area() const = 0;
};

class Rect : public Shape {
public:
    Rect(double w, double h) : w_(w), h_(h) {}
    double area() const override { return w_ * h_; }
private:
    double w_, h_;
};

class Square : public Rect {           // un carré est un rectangle particulier
public:
    explicit Square(double side) : Rect(side, side) {}
};

int main() {
    std::vector<std::unique_ptr<Shape>> shapes;
    shapes.push_back(std::make_unique<Rect>(2, 5));
    shapes.push_back(std::make_unique<Square>(3));

    double total = 0;
    for (const auto &s : shapes)
        total += s->area();             // appel virtuel
    std::cout << "aire totale : " << total << '\n';
}   // chaque unique_ptr détruit sa forme... via le destructeur virtuel
```

```output
aire totale : 19
```

`std::vector<Shape>` serait impossible (`Shape` est abstraite) et, avec une
base concrète, couperait les objets (voir « slicing » plus bas).

## Le destructeur virtuel

Quand on détruit un objet dérivé **à travers un pointeur vers la base**
(ce que fait `unique_ptr<Shape>`), le destructeur appelé est celui de la base…
sauf s'il est **virtuel**. Sans `virtual`, le destructeur de la classe dérivée
n'est jamais appelé : ses membres fuient, et c'est un comportement indéfini.

```cpp example
#include <iostream>
#include <memory>

struct Base {
    virtual ~Base() { std::cout << "~Base\n"; }
};

struct Derived : Base {
    ~Derived() override { std::cout << "~Derived\n"; }
};

int main() {
    std::unique_ptr<Base> p = std::make_unique<Derived>();
}   // ~Derived PUIS ~Base : la destruction se fait dans l'ordre inverse de la construction
```

```output
~Derived
~Base
```

**Règle : toute classe qui a une fonction virtuelle a un destructeur
virtuel**, en général `virtual ~Base() = default;`. GCC prévient sur un
`delete` direct avec `-Wdelete-non-virtual-dtor` (inclus dans `-Wall`), mais
pas toujours quand la destruction a lieu dans un `unique_ptr` : ne compte pas
sur l'avertissement.

## Le *slicing* : copier une dérivée dans une base

Une variable de type `Base` (ni référence ni pointeur) ne contient **que** la
partie `Base`. Y copier un objet dérivé « tranche » tout le reste :

```cpp example
#include <iostream>

struct Animal {
    virtual ~Animal() = default;
    virtual const char *sound() const { return "..."; }
};

struct Cat : Animal {
    const char *sound() const override { return "miaou"; }
};

void by_value(Animal a) { std::cout << a.sound() << '\n'; }        // copie TRONQUÉE
void by_ref(const Animal &a) { std::cout << a.sound() << '\n'; }   // l'objet réel

int main() {
    Cat c;
    by_value(c);
    by_ref(c);
}
```

```output
...
miaou
```

C'est pour cela qu'on attrape les exceptions par `const &` (chapitre 12) et
qu'on manipule les objets polymorphes par référence ou par pointeur.

## Composition ou héritage ?

L'héritage public signifie « **est un** » : un `Square` est une `Shape`, et
tout code qui attend une `Shape` doit pouvoir recevoir un `Square` sans
surprise. S'il s'agit seulement de **réutiliser du code**, on préfère la
**composition**, c'est-à-dire un membre :

```cpp
class Stack : public std::vector<int> {};   // MAUVAIS : une pile n'est pas un vecteur
                                            // (insert au milieu permis !)
class Stack {                               // BON : une pile A un vecteur
    std::vector<int> items_;
public:
    void push(int x) { items_.push_back(x); }
};
```

## Pièges

- **Oublier `virtual`** dans la base : la dérivée « redéfinit » sans effet
  dès qu'on passe par une référence vers la base.
- **Une signature légèrement différente** (`double area()` sans `const`) :
  sans `override`, c'est une **nouvelle** fonction, et la virtuelle de la base
  reste appelée. Mets toujours `override`, le compilateur détectera l'erreur.
- **Oublier le destructeur virtuel** : fuite et comportement indéfini à la
  destruction via un pointeur vers la base.
- **Passer un objet polymorphe par valeur** : *slicing*.
- **Appeler une fonction virtuelle dans un constructeur** : pendant la
  construction de la base, l'objet n'est pas encore un dérivé, c'est la
  version de la base qui est appelée (pure : plantage).
- **Des hiérarchies profondes** (`Animal` → `Mammal` → `Dog` → `Labrador`…) :
  difficiles à modifier. Deux niveaux suffisent presque toujours.

## En entreprise

- **Interfaces = classes abstraites** : uniquement des fonctions virtuelles
  pures et un destructeur virtuel. Le code dépend de l'interface, pas des
  implémentations, ce qui permet aussi de les remplacer par des « faux » dans
  les tests.
- **`override` systématique** (beaucoup de projets l'imposent avec
  `-Wsuggest-override`), et `final` pour interdire une redéfinition.
- **Composition d'abord**, héritage seulement pour un vrai « est un » avec du
  polymorphisme.
- **Le polymorphisme a un coût faible mais réel** : un appel indirect, qui
  empêche souvent le compilateur d'intégrer la fonction. Les templates
  (chapitre 11) choisissent à la compilation quand le type est connu.

## Vérifie ta compréhension

1. Que signifie `= 0` après une fonction virtuelle ?
2. Pourquoi `override` est-il utile, puisque le code compile sans ?
3. Qu'affiche `by_value(c)` dans l'exemple du *slicing*, et pourquoi ?

### Réponses

1. La fonction est virtuelle pure : la classe est abstraite (pas d'objet
   possible), et chaque classe dérivée concrète doit la définir.
2. Il fait vérifier que la fonction redéfinit vraiment une virtuelle de la
   base. Une différence de signature (un `const` oublié) devient une erreur de
   compilation au lieu d'une fonction qui n'est jamais appelée.
3. `...` : le paramètre est un `Animal` copié depuis `c`, seule la partie
   `Animal` a été copiée, et c'est donc `Animal::sound` qui est appelée.
