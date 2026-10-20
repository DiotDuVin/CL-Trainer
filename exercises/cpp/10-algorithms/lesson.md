id: cpp.lesson.algorithms
title: Algorithmes et lambdas
lang: cpp
topic: algorithms
estimated_min: 25
---
## Ce que tu connais en C

```c
int compare_desc(const void *a, const void *b) {
    return *(const int *)b - *(const int *)a;   /* piège : peut déborder ! */
}
qsort(tab, n, sizeof tab[0], compare_desc);
```

`qsort` est générique, mais au prix de `void *` et de casts. La fonction de
comparaison est séparée de son usage, et le compilateur ne peut pas
l'intégrer au tri (appel indirect à chaque comparaison). Pour tout le reste
(chercher, compter, filtrer, transformer), on réécrit une boucle à chaque
fois.

La bibliothèque standard C++ fournit une centaine d'**algorithmes** prêts à
l'emploi dans `<algorithm>` et `<numeric>`, et les **lambdas** permettent de
leur passer une fonction écrite sur place.

## Itérateurs : les intervalles `[begin, end)`

Les algorithmes ne travaillent pas sur un conteneur, mais sur un
**intervalle** délimité par deux **itérateurs** : `v.begin()` désigne le
premier élément, `v.end()` la position **juste après** le dernier. Un
itérateur se manipule comme un pointeur (`*it`, `++it`) ; pour un
`std::vector`, c'en est presque un.

```cpp
std::sort(v.begin(), v.end());         // tout le vecteur
std::sort(v.begin(), v.begin() + 3);   // les 3 premiers seulement
```

Ce même modèle fonctionne pour tous les conteneurs : `std::vector`,
`std::array`, `std::string`, `std::set`… et même les tableaux C (avec des
pointeurs).

## Les lambdas : une fonction écrite sur place

```cpp
[capture](paramètres) { corps }
```

```cpp example
#include <algorithm>
#include <iostream>
#include <vector>

int main() {
    std::vector<int> v = {5, -2, 9, 0, 3};

    std::sort(v.begin(), v.end());                                   // ordre croissant
    std::sort(v.begin(), v.end(), [](int a, int b) { return a > b; });   // décroissant

    for (int x : v)
        std::cout << x << ' ';
    std::cout << '\n';

    int threshold = 2;
    auto big = std::count_if(v.begin(), v.end(),
                             [threshold](int x) { return x > threshold; });   // capture par copie
    std::cout << big << " valeurs > " << threshold << '\n';
}
```

```output
9 5 3 0 -2 
3 valeurs > 2
```

- **`[]`** : la lambda n'utilise aucune variable extérieure.
- **`[threshold]`** : elle **capture** `threshold` par copie, et peut le lire.
- **`[&total]`** : capture par **référence**, pour pouvoir modifier `total`.
- **`[=]` / `[&]`** : capture tout ce qui est utilisé, par copie ou par
  référence. C'est pratique, mais moins explicite.

Le comparateur de `std::sort` renvoie `true` si `a` doit être placé **avant**
`b`. Il doit définir un « ordre strict » : `a < b`, jamais `a <= b`.

## Les algorithmes à connaître

```cpp example
#include <algorithm>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

int main() {
    std::vector<int> v = {4, 8, 15, 16, 23, 42};

    // Chercher
    auto it = std::find(v.begin(), v.end(), 16);
    std::cout << "16 à l'indice " << (it - v.begin()) << '\n';
    auto odd = std::find_if(v.begin(), v.end(), [](int x) { return x % 2 != 0; });
    std::cout << "premier impair : " << *odd << '\n';

    // Réduire
    int sum = std::accumulate(v.begin(), v.end(), 0);        // <numeric>
    auto [mn, mx] = std::minmax_element(v.begin(), v.end());
    std::cout << "somme " << sum << ", min " << *mn << ", max " << *mx << '\n';

    // Tester
    bool all_pos = std::all_of(v.begin(), v.end(), [](int x) { return x > 0; });
    std::cout << std::boolalpha << "tous positifs : " << all_pos << '\n';

    // Transformer (dans un autre vecteur)
    std::vector<std::string> labels(v.size());
    std::transform(v.begin(), v.end(), labels.begin(),
                   [](int x) { return "#" + std::to_string(x); });
    std::cout << labels.front() << " ... " << labels.back() << '\n';

    // Inverser
    std::reverse(v.begin(), v.end());
    std::cout << v[0] << '\n';
}
```

```output
16 à l'indice 3
premier impair : 15
somme 108, min 4, max 42
tous positifs : true
#4 ... #42
42
```

Si un algorithme ne trouve rien, il renvoie l'itérateur de **fin** (`v.end()`).
On doit toujours le tester avant de déréférencer : `if (it != v.end())`.

## Supprimer des éléments : l'idiome *erase-remove*

`std::remove_if` ne peut pas changer la taille d'un vecteur : il ne connaît
que des itérateurs. Il **déplace** les éléments à garder au début, et renvoie
la nouvelle fin logique. C'est `erase` qui retire ensuite la fin :

```cpp example
#include <algorithm>
#include <iostream>
#include <vector>

int main() {
    std::vector<int> v = {3, -1, 4, -1, 5, -9};
    v.erase(std::remove_if(v.begin(), v.end(), [](int x) { return x < 0; }),
            v.end());
    for (int x : v)
        std::cout << x << ' ';
    std::cout << "(" << v.size() << " éléments)\n";
}
```

```output
3 4 5 (3 éléments)
```

(C++20 simplifie l'écriture : `std::erase_if(v, predicat);`.)

## Trier des objets

```cpp example
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

struct Player {
    std::string name;
    int score;
};

int main() {
    std::vector<Player> players = {{"Linus", 50}, {"Ada", 70}, {"Grace", 50}};
    // Score décroissant, puis nom croissant en cas d'égalité
    std::sort(players.begin(), players.end(), [](const Player &a, const Player &b) {
        if (a.score != b.score)
            return a.score > b.score;
        return a.name < b.name;
    });
    for (const auto &p : players)
        std::cout << p.name << ' ' << p.score << '\n';
}
```

```output
Ada 70
Grace 50
Linus 50
```

Les paramètres de la lambda sont des `const &` : on ne copie pas les objets à
chaque comparaison. Et `std::stable_sort` conserve l'ordre initial des
éléments égaux, quand on en a besoin.

## Pièges

- **Un comparateur avec `<=`** : ce n'est pas un ordre strict. `std::sort`
  peut alors lire hors du tableau (comportement indéfini).
- **Oublier `erase` après `remove_if`** : la taille ne change pas, et la fin du
  vecteur contient des valeurs résiduelles.
- **Déréférencer le résultat d'un `find` sans le comparer à `end()`.**
- **`std::accumulate(v.begin(), v.end(), 0)` sur des `double`** : la valeur
  initiale `0` est un `int`, donc chaque somme partielle est convertie en
  `int` ! Il faut écrire `0.0`.
- **Capturer par référence une variable locale** dans une lambda qui survit à
  la fonction (stockée, renvoyée) : référence pendante.

## En entreprise

- **Un algorithme nommé plutôt qu'une boucle écrite à la main** :
  `std::any_of` dit ce qu'il fait, une boucle avec un `break` doit être lue
  pour être comprise.
- **Des lambdas courtes**, des captures explicites, des paramètres `const &`.
- **`std::sort` est en O(n log n)** et très optimisé ; ne réécris pas de tri.

## Vérifie ta compréhension

1. Que renvoie `std::find` quand l'élément est absent ?
2. Pourquoi `std::remove_if` seul ne réduit-il pas la taille du vecteur ?
3. Que vaut `std::accumulate(v.begin(), v.end(), 0)` pour `v = {0.5, 0.5}` ?

### Réponses

1. L'itérateur de fin de l'intervalle (`v.end()`).
2. Il ne manipule que des itérateurs et ne connaît pas le conteneur : il
   regroupe au début les éléments gardés et renvoie la nouvelle fin. C'est
   `erase` qui supprime réellement.
3. 0 : la valeur initiale est un `int`, donc `0 + 0.5` est tronqué à 0, deux
   fois. Avec `0.0`, le résultat est 1.
