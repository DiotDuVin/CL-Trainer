id: cpp.lesson.classes
title: Classes : données et comportement ensemble
lang: cpp
topic: classes
estimated_min: 25
---
## Ce que tu connais en C

```c
struct account {
    char owner[32];
    long balance;   /* en centimes ; ne doit JAMAIS être négatif */
};

void account_deposit(struct account *a, long amount);
int  account_withdraw(struct account *a, long amount);  /* 0 si refusé */
```

C'est déjà de la programmation « orientée objet » : une structure, et des
fonctions préfixées par son nom qui reçoivent un pointeur vers elle. Le moteur
de trainer est écrit exactement ainsi (`strbuf` + `sb_append`…).

Le problème : rien n'empêche d'écrire `a.balance = -500;` n'importe où dans le
programme. La règle « le solde n'est jamais négatif » n'est qu'un commentaire.

## Une classe : la structure et ses fonctions, réunies

```cpp example
#include <iostream>
#include <string>

class Account {
public:                                        // utilisable de l'extérieur
    Account(const std::string &owner, long initial)   // constructeur
        : owner_(owner), balance_(initial < 0 ? 0 : initial) {}

    void deposit(long amount) {                // méthode
        if (amount > 0)
            balance_ += amount;
    }

    bool withdraw(long amount) {
        if (amount <= 0 || amount > balance_)
            return false;                      // refusé : le solde reste valide
        balance_ -= amount;
        return true;
    }

    long balance() const { return balance_; }  // const : ne modifie pas l'objet
    const std::string &owner() const { return owner_; }

private:                                       // inaccessible de l'extérieur
    std::string owner_;
    long balance_;                             // invariant : jamais négatif
};

int main() {
    Account a("Ada", 100);
    a.deposit(50);
    std::cout << a.owner() << " : " << a.balance() << '\n';
    std::cout << std::boolalpha << a.withdraw(500) << ' ' << a.balance() << '\n';
    // a.balance_ = -500;   // ERREUR de compilation : balance_ est privé
}
```

```output
Ada : 150
false 150
```

Le vocabulaire :

- **Les données** (`owner_`, `balance_`) s'appellent les *membres* ou
  *attributs*. Le suffixe `_` est une convention répandue pour les
  reconnaître ; d'autres écrivent `m_balance`.
- **Les fonctions** (`deposit`, `withdraw`…) s'appellent les *méthodes*. On
  les appelle avec un point : `a.deposit(50)`. Elles accèdent directement aux
  membres de l'objet, sans pointeur à passer : le compilateur passe en
  coulisse un pointeur caché, nommé `this`.
- **Un objet** est une variable de type classe : `a` est un objet de la
  classe `Account`.

## `public` et `private` : l'encapsulation

- **`public`** : utilisable par tout le monde. C'est l'**interface** de la
  classe.
- **`private`** : réservé aux méthodes de la classe. C'est l'**implémentation**.

En rendant `balance_` privé, **seules** `deposit` et `withdraw` peuvent le
modifier, et elles vérifient la règle. La règle « jamais négatif » devient un
**invariant** garanti par le compilateur, et non plus un commentaire.
L'encapsulation sert exactement à ça : protéger les invariants.

**`struct` contre `class`** : en C++, les deux sont presque identiques. La
seule différence est que les membres d'une `struct` sont `public` par défaut,
et ceux d'une `class` `private` par défaut. Par convention, on utilise
`struct` pour un simple regroupement de données sans règle à protéger, et
`class` dès qu'il y a un invariant.

## Le constructeur

Le **constructeur** porte le nom de la classe et n'a pas de type de retour.
Il est appelé **automatiquement** à la création de l'objet : impossible
d'oublier l'initialisation, contrairement à une `struct` C.

```cpp
Account(const std::string &owner, long initial)
    : owner_(owner), balance_(initial < 0 ? 0 : initial)   // liste d'initialisation
{
    // corps (souvent vide)
}
```

La partie après `:` est la **liste d'initialisation**. Elle construit
directement chaque membre avec sa valeur. **Préfère-la à des affectations
dans le corps** : une affectation construirait d'abord le membre vide, puis
le modifierait. Les membres `const` ou références, eux, *ne peuvent être*
initialisés que là.

Les membres sont initialisés dans **l'ordre de leur déclaration dans la
classe**, pas dans l'ordre de la liste. g++ prévient avec `-Wreorder` si les
deux ordres diffèrent.

Une classe peut avoir plusieurs constructeurs (c'est de la surcharge,
chapitre 3) :

```cpp example
#include <iostream>

class Point {
public:
    Point() : x_(0), y_(0) {}                     // constructeur par défaut
    Point(double x, double y) : x_(x), y_(y) {}
    double x() const { return x_; }
    double y() const { return y_; }
private:
    double x_, y_;
};

int main() {
    Point origin;              // constructeur par défaut (pas de parenthèses !)
    Point p(3, 4);
    Point q{1.5, -2};          // accolades : même chose, syntaxe moderne
    std::cout << origin.x() << ' ' << p.y() << ' ' << q.x() << '\n';
}
```

```output
0 4 1.5
```

Attention : `Point origin();` (avec des parenthèses vides) ne crée **pas**
d'objet. C'est la déclaration d'une fonction `origin` qui renvoie un `Point` !
C'est un piège célèbre du C++, surnommé *most vexing parse*.

## Les méthodes `const`

```cpp
long balance() const { return balance_; }
```

Le `const` après les parenthèses promet que la méthode **ne modifie pas
l'objet**. Le compilateur le vérifie, et c'est indispensable : sur un objet
`const` (par exemple un paramètre `const Account &`), on ne peut appeler
**que** des méthodes `const`.

```cpp
void print(const Account &a) {
    std::cout << a.balance();   // OK : balance() est const
    a.deposit(10);              // ERREUR : deposit n'est pas const
}
```

**Règle : toute méthode qui ne modifie pas l'objet doit être `const`.**
L'oublier rend la classe inutilisable avec des `const &`, c'est-à-dire avec
la façon normale de passer des objets (chapitre 2).

## Afficher un objet avec `<<`

On peut définir le sens de `<<` pour nos propres classes. C'est la
**surcharge d'opérateur** : une simple fonction au nom spécial.

```cpp example
#include <iostream>

struct Vec2 {
    double x, y;
};

// « écrire un Vec2 dans un flux » : renvoie le flux pour pouvoir enchaîner
std::ostream &operator<<(std::ostream &out, const Vec2 &v) {
    return out << '(' << v.x << ", " << v.y << ')';
}

Vec2 operator+(const Vec2 &a, const Vec2 &b) {
    return {a.x + b.x, a.y + b.y};
}

int main() {
    Vec2 a{1, 2}, b{0.5, -1};
    std::cout << a << " + " << b << " = " << a + b << '\n';
}
```

```output
(1, 2) + (0.5, -1) = (1.5, 1)
```

`std::cout << a` appelle `operator<<(std::cout, a)`. La fonction renvoie le
flux reçu (`std::ostream &`), ce qui permet d'enchaîner les `<<`, comme on
l'a vu au chapitre 1.

## Pièges

- **Oublier `const` sur une méthode de lecture.** La classe devient
  inutilisable à travers un `const &`, avec une erreur parfois déroutante
  (« passing 'const Account' as 'this' argument discards qualifiers »).
- **Initialiser dans le corps du constructeur** au lieu de la liste
  d'initialisation : moins efficace, et impossible pour les membres `const`
  ou références.
- **Écrire `Point p();`**, qui déclare une fonction. Écris `Point p;` ou `Point p{};`.
- **Tout mettre en `public` avec des *getters* et *setters* pour chaque
  membre.** Ça ne protège rien : l'interface doit exprimer des actions
  (`withdraw`), pas exposer les données (`setBalance`).
- **Oublier le point-virgule après l'accolade fermante d'une classe** :
  `class A { ... };`. Comme pour une `struct` en C.

## En entreprise

- **Les membres de données sont privés**, et l'interface publique est petite.
- **Le constructeur établit l'invariant**, et chaque méthode le préserve. Un
  objet n'est jamais dans un état invalide.
- **const-correctness** : les méthodes de lecture sont `const`, et les objets
  se passent en `const &`.
- **Une classe = une responsabilité**, avec un nom qui la décrit.

## Vérifie ta compréhension

1. Quelle est la seule différence entre `struct` et `class` ?
2. Pourquoi `balance()` doit-elle être `const` ?
3. Dans `class A { int b_; int a_; public: A() : a_(1), b_(a_ + 1) {} };`,
   que vaut `b_` ?

### Réponses

1. La visibilité par défaut : `public` pour `struct`, `private` pour `class`.
2. Elle ne modifie pas l'objet. Sans `const`, on ne pourrait pas l'appeler sur
   un `const Account &`, donc pas dans une fonction qui reçoit le compte en
   lecture seule.
3. N'importe quoi (comportement indéfini) : les membres sont initialisés dans
   l'ordre de **déclaration**. `b_` est donc initialisé en premier, avec
   `a_ + 1`, alors que `a_` ne l'est pas encore. g++ le signale avec
   `-Wreorder` (activé par `-Wall`).
