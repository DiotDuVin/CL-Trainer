id: cpp.lesson.containers
title: std::vector et std::array
lang: cpp
topic: containers
estimated_min: 20
---
## Ce que tu connais en C

```c
int notes[100];                 /* taille fixée à la compilation */
int n = 0;                      /* à suivre à la main, à côté du tableau */

int *v = malloc(n * sizeof *v); /* taille connue à l'exécution */
v = realloc(v, 2 * n * sizeof *v);  /* agrandir : à la main, et ne pas oublier de vérifier */
free(v);                        /* ne pas oublier, et une seule fois */
```

Un tableau C ne connaît pas sa taille. Passé à une fonction, il devient un
simple pointeur : il faut transmettre la taille à part. Rien ne vérifie les
indices. Et un tableau dynamique demande `malloc`, `realloc` et `free`, avec
tous les bugs qui vont avec.

Le C++ propose deux remplaçants :

- **`std::vector<T>`**, pour un tableau dont la taille varie à l'exécution :
  l'équivalent sûr de `malloc`/`realloc`/`free` ;
- **`std::array<T, N>`**, pour un tableau de taille fixe : l'équivalent sûr
  de `int t[N]`.

## `std::vector` : le tableau dynamique

```cpp example
#include <iostream>
#include <vector>

int main() {
    std::vector<int> notes = {12, 15, 9};   // T entre chevrons : le type des éléments
    notes.push_back(18);                     // ajout à la fin : s'agrandit tout seul
    notes.push_back(7);

    std::cout << "taille : " << notes.size() << '\n';
    std::cout << "premier : " << notes[0] << ", dernier : " << notes.back() << '\n';

    notes[1] = 16;                           // accès comme un tableau C
    int total = 0;
    for (int n : notes)                      // boucle for sur intervalle (chapitre 3)
        total += n;
    std::cout << "total : " << total << '\n';
}                                            // mémoire libérée automatiquement ici
```

```output
taille : 5
premier : 12, dernier : 7
total : 62
```

`std::vector<int>` se lit « vecteur d'entiers ». Les chevrons indiquent le
type des éléments : c'est un *template* (chapitre 11), une même classe qui
fonctionne pour n'importe quel type (`std::vector<std::string>`,
`std::vector<double>`…).

Les opérations essentielles :

| Opération | Effet | Coût |
|---|---|---|
| `v.push_back(x)` | ajoute `x` à la fin | rapide (O(1) en moyenne) |
| `v.pop_back()` | retire le dernier élément | O(1) |
| `v.size()` | nombre d'éléments (`std::size_t`) | O(1) |
| `v.empty()` | `true` si vide | O(1) |
| `v[i]` | élément `i`, **sans vérification** | O(1) |
| `v.at(i)` | élément `i`, **avec vérification** (exception si hors limites) | O(1) |
| `v.front()`, `v.back()` | premier, dernier (**v ne doit pas être vide**) | O(1) |
| `v.clear()` | vide le vecteur | O(n) |

### Comment `push_back` peut être rapide

Un vecteur réserve plus de place que nécessaire : sa **capacité**
(`v.capacity()`) dépasse sa taille. Quand elle est pleine, il alloue un bloc
plus grand (souvent deux fois plus grand), y recopie les éléments, puis libère
l'ancien. C'est exactement le `strbuf` du moteur de trainer, écrit en C :
la croissance géométrique rend l'ajout rapide en moyenne.

Si tu connais la taille finale à l'avance, `v.reserve(n)` alloue d'un coup et
évite les réallocations successives.

### Construire un vecteur

```cpp
std::vector<int> a;               // vide
std::vector<int> b(5);            // 5 éléments valant 0
std::vector<int> c(5, -1);        // 5 éléments valant -1
std::vector<int> d = {5, -1};     // 2 éléments : 5 et -1  (attention à la différence !)
```

### Passer un vecteur à une fonction

Exactement comme une `std::string` (chapitre 2) : par `const &` pour lire,
par `&` pour modifier. **Jamais par valeur sans raison** : ce serait une copie
de tous les éléments.

```cpp example
#include <iostream>
#include <vector>

double average(const std::vector<double> &values) {   // lecture : const &
    if (values.empty())
        return 0.0;
    double sum = 0.0;
    for (double v : values)
        sum += v;
    return sum / static_cast<double>(values.size());
}

void add_bonus(std::vector<double> &values, double bonus) {   // modification : &
    for (double &v : values)   // & : on modifie les éléments eux-mêmes
        v += bonus;
}

std::vector<double> squares(int n) {   // renvoyer un vecteur : pas de copie (chapitre 8)
    std::vector<double> result;
    for (int i = 1; i <= n; ++i)
        result.push_back(i * i);
    return result;
}

int main() {
    std::vector<double> v = squares(3);   // 1 4 9
    add_bonus(v, 1.0);                    // 2 5 10
    std::cout << average(v) << '\n';
}
```

```output
5.66667
```

Renvoyer un vecteur par valeur est **normal et efficace** en C++ moderne : le
compilateur *déplace* le résultat au lieu de le copier. On verra pourquoi au
chapitre 8.

## `std::array` : le tableau de taille fixe

```cpp example
#include <array>
#include <iostream>

int main() {
    std::array<int, 7> days = {31, 28, 31, 30, 31, 30, 31};   // taille dans le type
    std::array<int, 10> counts{};                             // {} : tous à zéro

    counts[3] += 2;
    std::cout << days.size() << " mois, juillet : " << days[6] << '\n';
    std::cout << "counts[3] = " << counts[3] << ", counts[4] = " << counts[4] << '\n';
}
```

```output
7 mois, juillet : 31
counts[3] = 2, counts[4] = 0
```

Un `std::array` est aussi rapide qu'un tableau C : pas d'allocation, les
éléments sont directement dans l'objet. Mais il connaît sa taille, ne se
transforme pas en pointeur, se copie avec `=` et se compare avec `==`.

**Quand utiliser quoi ?** `std::vector` par défaut ; `std::array` quand la
taille est connue à la compilation et ne change jamais (jours de la semaine,
coordonnées 3D, compteurs par chiffre…).

## Tableaux à deux dimensions

Un vecteur de vecteurs :

```cpp example
#include <iostream>
#include <vector>

int main() {
    const std::size_t rows = 2, cols = 3;
    std::vector<std::vector<int>> grid(rows, std::vector<int>(cols, 0));
    grid[1][2] = 5;
    for (const auto &row : grid) {        // chaque ligne est un std::vector<int>
        for (int cell : row)
            std::cout << cell << ' ';
        std::cout << '\n';
    }
}
```

```output
0 0 0 
0 0 5 
```

## Pièges

- **Accès hors limites avec `[]`.** Comme en C, `v[v.size()]` est un
  comportement indéfini, sans aucun message. Pendant la mise au point,
  `v.at(i)` vérifie l'indice (et lance une exception, chapitre 12). Les
  sanitizers et l'option `-D_GLIBCXX_ASSERTIONS` de g++ détectent aussi ces
  erreurs.
- **`v.size() - 1` sur un vecteur vide.** `size()` est non signé : `0 - 1`
  donne un nombre énorme, pas -1. Une boucle `for (std::size_t i = 0; i < v.size() - 1; ++i)`
  sur un vecteur vide part donc dans les choux. Teste `empty()` d'abord.
- **Garder une référence pendant un `push_back`.** Si le vecteur doit
  s'agrandir, ses éléments sont déplacés vers un nouveau bloc : les références,
  pointeurs et itérateurs vers les anciens éléments deviennent invalides.

  ```cpp
  std::vector<int> v = {1, 2, 3};
  int &first = v[0];
  v.push_back(4);      // réallocation possible...
  first = 10;          // ... comportement indéfini : first peut désigner l'ancien bloc
  ```

- **`std::vector<int> v(5)` contre `std::vector<int> v{5}`.** Le premier
  crée 5 zéros, le second un seul élément valant 5.

## En entreprise

- **`std::vector` est le conteneur par défaut.** On ne choisit autre chose
  (liste, map…) que pour une raison précise, généralement mesurée.
- **`malloc` / `new[]` pour un tableau, on ne l'écrit plus.**
- **`reserve` quand la taille est connue**, pour éviter les réallocations.
- **Paramètres en `const std::vector<T> &`**, résultats renvoyés par valeur.

## Vérifie ta compréhension

1. Que contient `std::vector<int> v(3, 7);` ? Et `std::vector<int> w = {3, 7};` ?
2. Pourquoi `void f(std::vector<int> v)` est-il en général une mauvaise idée ?
3. Pourquoi un `int &x = v[0];` peut-il devenir invalide après `v.push_back(42)` ?

### Réponses

1. `v` contient trois 7 (`7 7 7`) ; `w` contient deux éléments, 3 et 7.
2. Le vecteur est copié en entier à chaque appel. On écrit `const std::vector<int> &`
   pour lire, `std::vector<int> &` pour modifier.
3. Si la capacité est pleine, `push_back` alloue un nouveau bloc, y déplace les
   éléments et libère l'ancien : `x` désigne alors de la mémoire libérée.
