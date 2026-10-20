id: cpp.lesson.basics
title: Premier programme C++
lang: cpp
topic: basics
estimated_min: 15
---
## Ce que tu connais en C

```c
#include <stdio.h>

int main(void) {
    char name[32];
    printf("Ton prénom ? ");
    if (scanf("%31s", name) != 1)
        return 1;
    printf("Bonjour, %s !\n", name);
    return 0;
}
```

Ce programme a trois faiblesses que tu connais bien :

- **`printf` et `scanf` ne vérifient pas les types.** Un `%d` pour un `double`
  compile quand même, et le comportement est indéfini.
- **La taille du tampon est figée.** Il faut penser au `%31s`, sinon c'est un
  débordement mémoire.
- **Manipuler des chaînes demande `strcpy`, `strcat`, `strlen`**… et de gérer
  la mémoire à la main.

Le C++ garde tout le C (ou presque), mais propose mieux pour chacun de ces
points.

## Compiler du C++

Un fichier C++ porte l'extension `.cpp`. On le compile avec `g++` (et non
`gcc`, qui oublierait de lier la bibliothèque standard C++) :

    g++ -std=c++17 -Wall -Wextra -pedantic main.cpp -o main
    ./main

`-std=c++17` fixe la version du langage. Le C++ évolue tous les 3 ans (C++11,
14, 17, 20, 23) ; C++17 est très répandu en entreprise et c'est la version
utilisée par ce cours. C'est exactement ce que fait `trainer check`.

## Premier programme

```cpp example
#include <iostream>
#include <string>

int main() {
    std::string name = "Ada";
    int year = 1843;
    std::cout << "Bonjour, " << name << " !\n";
    std::cout << "Premier programme publié en " << year << '\n';
}
```

```output
Bonjour, Ada !
Premier programme publié en 1843
```

Ce qu'il faut remarquer :

- **`#include <iostream>`** : les en-têtes de la bibliothèque standard C++
  n'ont pas d'extension `.h`.
- **`std::cout << x`** affiche `x`, quel que soit son type. Il n'y a pas de
  format comme `%d` : le compilateur choisit la bonne façon d'afficher grâce
  au type. Une erreur de type devient impossible.
- **`<<` s'enchaîne** : `std::cout << a << b << c`.
- **`'\n'` passe à la ligne.**
- **`int main()`** : en C++, des parenthèses vides signifient « aucun
  paramètre » (en C, avant C23, il fallait écrire `void`).
- **`main` peut se terminer sans `return`** : c'est équivalent à `return 0;`.
  C'est la seule fonction pour laquelle c'est permis.

## Les espaces de noms (`namespace`)

`std::` signifie « qui vient de l'espace de noms `std` », celui de la
bibliothèque standard. Un espace de noms évite les collisions de noms entre
bibliothèques. En C, on préfixe les noms à la main (`SDL_Init`,
`png_read`…) ; en C++, le langage le fait pour toi.

```cpp example
#include <iostream>

namespace geometry {
    double square(double x) { return x * x; }
}

namespace finance {
    double square(double amount) { return amount; } // même nom, autre sens
}

int main() {
    std::cout << geometry::square(3.0) << ' ' << finance::square(3.0) << '\n';
}
```

```output
9 3
```

Tu verras souvent `using namespace std;` en haut des fichiers, pour écrire
`cout` au lieu de `std::cout`. C'est toléré dans un petit `.cpp`, mais **à
proscrire dans un en-tête** (`.h` / `.hpp`) : tous les fichiers qui
l'incluraient hériteraient de tous les noms de `std`, et les collisions
réapparaissent. Beaucoup d'entreprises l'interdisent complètement ; ce cours
écrit toujours `std::`.

## Lire l'entrée : `std::cin`

```cpp
int age = 0;
std::cin >> age;          // lit un entier (saute les espaces et retours à la ligne)

std::string word;
std::cin >> word;         // lit un MOT : s'arrête au premier blanc
```

Comme pour `cout`, pas de format : le type de la variable suffit. Une lecture
peut échouer (fin de l'entrée, texte au lieu d'un nombre). **Il faut le
vérifier**, comme le retour de `scanf` :

```cpp
if (!(std::cin >> age)) {
    // échec : entrée vide ou invalide
}

int x = 0;
while (std::cin >> x) {   // lit des nombres jusqu'à la fin de l'entrée
    // ...
}
```

Pour lire une **ligne entière**, espaces compris, on utilise `std::getline` :

```cpp
std::string line;
while (std::getline(std::cin, line)) {
    // une ligne, sans le '\n' final
}
```

## `std::string` : des chaînes qui gèrent leur mémoire

```cpp example
#include <iostream>
#include <string>

int main() {
    std::string first = "Grace";
    std::string last = "Hopper";
    std::string full = first + " " + last;    // concaténation : pas de strcat
    full += " (1906)";                        // ajout à la fin

    std::cout << full << '\n';
    std::cout << "Longueur : " << full.size() << '\n';
    std::cout << "Première lettre : " << full[0] << '\n';
    if (first == "Grace")                     // comparaison : pas de strcmp
        std::cout << "C'est bien elle\n";
}
```

```output
Grace Hopper (1906)
Longueur : 19
Première lettre : G
C'est bien elle
```

Une `std::string` s'agrandit toute seule et libère sa mémoire toute seule à
la fin de sa portée : pas de `malloc`, pas de `free`, pas de débordement de
tampon. On verra au chapitre RAII comment ce mécanisme fonctionne (et comment
l'utiliser pour tes propres types).

Attention : `size()` compte des **octets**, pas des lettres. « é » occupe 2
octets en UTF-8, donc `std::string("été").size()` vaut 5.

## Les types

Les types du C existent tous (`int`, `double`, `char`…), avec en plus :

- **`bool`**, avec `true` et `false`, intégré au langage : plus besoin de
  `<stdbool.h>`.
- **Des types à taille fixe** comme `std::int64_t` ou `std::uint8_t`, dans
  `<cstdint>`. Utilise-les quand la taille compte : `int` fait souvent 32 bits,
  mais rien ne le garantit.
- **Les en-têtes du C sont disponibles** sous la forme `<cmath>`, `<cstdio>`,
  `<cstring>`… (préfixe `c`, sans `.h`).

```cpp example
#include <cstdint>
#include <iostream>

int main() {
    std::int64_t big = 3'000'000'000;   // C++14 : ' sépare les chiffres
    bool even = big % 2 == 0;
    std::cout << big << ' ' << even << ' ' << std::boolalpha << even << '\n';
}
```

```output
3000000000 1 true
```

## Pièges

- **`std::endl` au lieu de `'\n'`.** `std::endl` passe à la ligne **et**
  vide le tampon de sortie à chaque fois, ce qui est inutile et lent dans une
  boucle. Réserve-le aux cas où l'affichage doit sortir immédiatement.
- **Mélanger `std::cin >>` et `std::getline`.** Après `std::cin >> n`, le
  `'\n'` tapé après le nombre reste dans l'entrée : le `getline` suivant lit
  une ligne vide ! Solution : `std::cin >> std::ws` (saute les blancs) avant
  `getline`, ou ne lire qu'avec `getline`.
- **Les dépassements d'entiers.** Comme en C, dépasser la capacité d'un `int`
  signé est un **comportement indéfini**. Choisis le type avant de calculer.
- **`toupper` / `isalpha` sur un `char` accentué.** Le `char` peut être
  négatif (octets UTF-8 > 127) : il faut convertir en `unsigned char`, comme
  en C : `std::toupper(static_cast<unsigned char>(c))`.

## En entreprise

- **Compile toujours avec `-Wall -Wextra`**, et souvent `-Werror` : un
  avertissement signale très souvent un bug.
- **Préfère `std::string` aux `char[]`**, `'\n'` à `std::endl`, et écris
  `std::` plutôt que `using namespace std;`.
- **`static_cast<T>(x)`** est la conversion explicite du C++. On la préfère au
  cast du C `(T)x` : elle est plus restrictive et facile à rechercher dans le
  code.

## Vérifie ta compréhension

1. Pourquoi `std::cout << x` ne peut-il pas avoir le bug d'un mauvais `%d` ?
2. Que lit `std::cin >> s` si l'entrée est `  Ada Lovelace` ?
3. Que vaut `std::string("été").size()` ?

### Réponses

1. Le compilateur choisit la façon d'afficher selon le **type** de `x` (on
   parle de **surcharge** de l'opérateur `<<`, vue au chapitre 3) : aucun
   format à écrire, donc aucun format faux possible.
2. `Ada` : les espaces initiaux sont sautés, et la lecture s'arrête au premier
   blanc. `Lovelace` reste dans l'entrée pour la lecture suivante.
3. `5` : `size()` compte les octets, et chaque « é » en occupe deux en UTF-8.
