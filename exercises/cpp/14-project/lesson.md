id: cpp.lesson.project
title: Organiser un projet : en-têtes, compilation séparée, namespaces
lang: cpp
topic: project
estimated_min: 30
---
## Ce que tu connais en C

```c
/* point.h */                         /* point.c */
#ifndef POINT_H                       #include "point.h"
#define POINT_H                       double point_norm(struct point p) {
struct point { double x, y; };            return sqrt(p.x * p.x + p.y * p.y);
double point_norm(struct point p);    }
#endif
```

```
gcc -c point.c -o point.o      # compile chaque .c séparément
gcc -c main.c  -o main.o
gcc point.o main.o -o app -lm  # puis l'éditeur de liens les assemble
```

**Tout cela reste vrai en C++.** Les fichiers s'appellent `.h` (ou `.hpp`) et
`.cpp`, et on compile avec `g++`. Le C++ ajoute quatre choses : les classes
dans les en-têtes, les **namespaces**, des règles précises sur ce qu'un
en-tête peut définir, et la décoration des noms (*name mangling*).

## Le partage des rôles

| Fichier | Contient | Rôle |
|---|---|---|
| `point.h` | déclarations, définition des classes | l'**interface** : ce que les autres fichiers peuvent utiliser |
| `point.cpp` | définitions des fonctions | l'**implémentation** : compilée une seule fois |
| `main.cpp` | `#include "point.h"` | un **utilisateur** de l'interface |

```cpp
// point.h
#pragma once                     // inclusion unique (voir plus bas)
#include <string>

class Point {
public:
    Point(double x, double y);
    double norm() const;
    std::string to_string() const;
private:
    double x_, y_;
};
```

```cpp
// point.cpp
#include "point.h"               // toujours inclure SON propre en-tête en premier
#include <cmath>

Point::Point(double x, double y) : x_(x), y_(y) {}

double Point::norm() const {     // Point:: : « la fonction norm de la classe Point »
    return std::sqrt(x_ * x_ + y_ * y_);
}

std::string Point::to_string() const {
    return "(" + std::to_string(x_) + ", " + std::to_string(y_) + ")";
}
```

- **`Point::norm`** : hors de la classe, on préfixe chaque fonction membre par
  le nom de la classe. Sans ce préfixe, on définirait une fonction libre
  `norm`, sans accès à `x_`.
- **On répète `const`** : il fait partie de la signature. On ne répète pas
  les valeurs par défaut des paramètres, ni `override`, ni `virtual`, ni
  `static` : ceux-là ne s'écrivent que dans la déclaration.
- **Inclure son propre en-tête en premier** dans le `.cpp` vérifie que
  l'en-tête se suffit à lui-même (qu'il inclut bien `<string>`, par exemple).

## Compilation séparée : deux sortes d'erreurs

Chaque `.cpp` est compilé **seul** : le compilateur ne voit que ce fichier et
les en-têtes qu'il inclut. Ensuite, l'éditeur de liens (`ld`) relie les
appels aux définitions trouvées dans les autres fichiers objets.

```
main.cpp ──(g++ -c)──> main.o ─┐
                               ├──(éditeur de liens)──> app
point.cpp ─(g++ -c)──> point.o ┘
```

Savoir **à quelle étape** une erreur se produit dit tout de suite où chercher :

| Message | Étape | Cause habituelle |
|---|---|---|
| `error: 'add' was not declared in this scope` | compilation | l'en-tête n'est pas inclus, ou faute de frappe |
| `undefined reference to 'add(int, int)'` | édition de liens | déclarée mais **définie nulle part** : `.cpp` oublié dans la commande, ou fonction libre définie avec une signature différente |
| `no declaration matches 'double Point::norm()'` | compilation | fonction membre définie avec une signature différente de sa déclaration (souvent un `const` oublié) |
| `multiple definition of 'twice(int)'` | édition de liens | une fonction **définie** dans un en-tête inclus par plusieurs `.cpp` |
| `error: redefinition of 'struct Point'` | compilation | en-tête inclus deux fois sans protection |

(Les messages ci-dessus sont ceux de GCC, reproduits tels quels.)

## Protéger les en-têtes

Un en-tête est souvent inclus plusieurs fois dans un même `.cpp`, par des
chemins détournés (`main.cpp` inclut `a.h` et `b.h`, qui incluent tous deux
`point.h`). Sans protection, la classe est définie deux fois.

```cpp
#pragma once           // non standard, mais reconnu par tous les compilateurs courants
```

ou la forme classique, 100 % standard, que tu connais du C :

```cpp
#ifndef POINT_H
#define POINT_H
// ...
#endif
```

**Tout en-tête commence par l'un des deux.** Les deux se valent en pratique ;
suis la convention du projet.

## Ce qu'un en-tête peut contenir : la règle de la définition unique

La *One Definition Rule* (ODR) : dans tout le programme, une fonction ou une
variable globale non `inline` est définie **une seule fois**. Un en-tête étant
copié dans chaque `.cpp` qui l'inclut, il ne doit contenir que des choses
qu'on a le droit de voir plusieurs fois :

| Dans un `.h` | Autorisé ? |
|---|---|
| déclarations de fonctions, `class`/`struct`, `enum`, `using` | oui |
| fonctions membres **définies dans la classe** | oui (implicitement `inline`) |
| fonctions marquées **`inline`**, **templates** (chapitre 11) | oui |
| constantes `constexpr`, variables `inline` (C++17) | oui |
| définition d'une fonction libre ordinaire | **non** : *multiple definition* |
| variable globale ordinaire (`int counter = 0;`) | **non** : *multiple definition* |

`inline` ne veut plus dire « recopie le code à l'appel » : il signifie
« plusieurs définitions identiques sont permises, l'éditeur de liens en garde
une ». C'est ce qui rend les fonctions courtes et les templates possibles dans
les en-têtes.

## Les namespaces

Le C n'a qu'un seul espace de noms : toutes les fonctions d'un programme et de
ses bibliothèques se partagent les mêmes noms. D'où les préfixes, comme
`sb_append` et `sv_push` dans trainer. Le C++ range les noms dans des
**namespaces** :

```cpp example
#include <iostream>
#include <string>

namespace geometry {
    double area(double w, double h) { return w * h; }
}

namespace text {
    std::string area(const std::string &s) { return "[" + s + "]"; }
}

namespace {                              // namespace anonyme : visible dans CE fichier seulement
    int helper_calls = 0;
    int helper() { return ++helper_calls; }
}

int main() {
    std::cout << geometry::area(2, 3) << ' ' << text::area("ok") << '\n';

    using geometry::area;                // importe UN nom, localement
    std::cout << area(4, 5) << '\n';

    helper();
    std::cout << "helper appelé " << helper() << " fois\n";
}
```

```output
6 [ok]
20
helper appelé 2 fois
```

- **`std::`**, depuis le premier chapitre, c'est le namespace de la
  bibliothèque standard.
- **Le namespace anonyme** remplace le `static` du C pour les fonctions et
  variables internes à un `.cpp` : deux fichiers peuvent avoir chacun leur
  `helper` sans conflit à l'édition de liens.
- **On définit les fonctions d'un namespace** en rouvrant le namespace dans le
  `.cpp` (`namespace geometry { double area(...) { ... } }`) ou avec le nom
  qualifié (`double geometry::area(...) { ... }`).

## Pourquoi `extern "C"` : la décoration des noms

En C++, plusieurs fonctions peuvent porter le même nom (surcharge,
namespaces). Le compilateur encode donc les types dans le nom du symbole :
`add(int, int)` devient `_Z3addii`. C'est pour cela que le message
*undefined reference* affiche les types des paramètres.

Le C, lui, n'encode rien : le symbole de `add` est `add`. Pour qu'un programme
C++ appelle une bibliothèque C, ses en-têtes déclarent les fonctions
`extern "C"`. C'est exactement ce que font les en-têtes du moteur de trainer,
utilisés par l'interface graphique en C++ :

```cpp
#ifdef __cplusplus
extern "C" {
#endif
int progress_load(const char *path, progress *p);   // include/progress.h
#ifdef __cplusplus
}
#endif
```

## Compiler un projet : de la commande à CMake

À la main, on compile tous les `.cpp` (les `.h` ne se compilent pas, ils sont
inclus) :

```
g++ -std=c++17 -Wall -Wextra main.cpp point.cpp -o app
```

Au-delà de quelques fichiers, on utilise un outil de construction. En C++,
c'est presque toujours **CMake** (l'interface graphique de trainer l'utilise) :

```
# CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(app CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_executable(app main.cpp point.cpp)
target_compile_options(app PRIVATE -Wall -Wextra -pedantic)
```

```
cmake -S . -B build      # configure (une fois)
cmake --build build      # recompile seulement ce qui a changé
./build/app
```

Dans VS Code, l'extension **CMake Tools** lit ce fichier et fournit les
boutons « Build » et « Debug ».

## Pièges

- **Définir une fonction ordinaire dans un `.h`** : *multiple definition* dès
  que deux `.cpp` l'incluent. Déplace-la dans le `.cpp`, ou marque-la
  `inline`.
- **Oublier `Classe::`** devant une fonction membre dans le `.cpp` : on
  définit une fonction libre. Erreur : `'x_' was not declared in this scope`
  dans le corps (ou, si le corps n'utilise aucun membre, *undefined
  reference* pour la vraie fonction membre).
- **Oublier un `.cpp` dans la commande** de compilation : *undefined
  reference*.
- **`using namespace std;` dans un en-tête** : il s'impose à tous les fichiers
  qui l'incluent, et peut provoquer des conflits de noms difficiles à
  comprendre. Dans un `.cpp`, c'est un choix d'équipe ; dans un `.h`, jamais.
- **Inclure un `.cpp`** (`#include "point.cpp"`) : *multiple definition*, et
  la compilation séparée ne sert plus à rien.
- **Un en-tête qui dépend de l'ordre d'inclusion** : chaque en-tête inclut
  lui-même tout ce qu'il utilise.

## En entreprise

- **Un en-tête et un `.cpp` par classe ou module**, de même nom
  (`point.h` / `point.cpp`).
- **Des en-têtes minimaux** : moins un `.h` inclut de choses, moins un
  changement force à tout recompiler. On peut déclarer une classe sans
  l'inclure (`class Point;`) quand on n'utilise que des pointeurs ou
  références vers elle.
- **Un namespace par projet ou bibliothèque**, et un namespace anonyme pour
  les détails internes d'un `.cpp`.
- **CMake** pour construire, et la même commande en local et dans la CI.

## Vérifie ta compréhension

1. `undefined reference to 'Point::norm() const'` : quelle étape échoue, et
   quelles sont les causes possibles ?
2. Pourquoi une fonction membre définie à l'intérieur de la classe, dans le
   `.h`, ne provoque-t-elle pas de *multiple definition* ?
3. Par quoi remplace-t-on le `static` d'une fonction interne en C ?

### Réponses

1. L'édition de liens : `norm` est déclarée mais aucun fichier objet ne la
   définit. Le plus souvent, `point.cpp` n'est pas compilé avec le reste (ou
   `Point::` a été oublié devant la définition). Un `const` oublié, lui,
   serait détecté plus tôt, à la compilation : *no declaration matches*.
2. Elle est implicitement `inline` : plusieurs définitions identiques sont
   permises, l'éditeur de liens n'en garde qu'une.
3. Par un namespace anonyme (`namespace { ... }`) ; `static` fonctionne aussi,
   mais le namespace anonyme s'applique également aux classes.
