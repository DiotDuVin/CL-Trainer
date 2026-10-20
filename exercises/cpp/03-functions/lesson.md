id: cpp.lesson.functions
title: Fonctions en C++ moderne
lang: cpp
topic: functions
estimated_min: 20
---
## Ce que tu connais en C

En C, deux fonctions ne peuvent pas porter le même nom. On écrit donc
`abs`, `labs`, `fabs`… une version par type. Tous les paramètres doivent
être fournis à chaque appel. Pour parcourir un tableau, on gère un indice à
la main. Et `NULL` est une macro qui vaut souvent simplement `0`.

Le C++ améliore ces quatre points.

## La surcharge : un nom, plusieurs versions

Plusieurs fonctions peuvent porter **le même nom** si leurs paramètres
diffèrent. Le compilateur choisit la bonne version d'après les types des
arguments, **à la compilation** :

```cpp example
#include <iostream>
#include <string>

void show(int x)                { std::cout << "entier : " << x << '\n'; }
void show(double x)             { std::cout << "réel : " << x << '\n'; }
void show(const std::string &s) { std::cout << "texte : " << s << '\n'; }

int main() {
    show(42);
    show(2.5);
    show(std::string("bonjour"));
}
```

```output
entier : 42
réel : 2.5
texte : bonjour
```

Tu utilises déjà la surcharge sans le savoir : `std::cout << x` appelle une
version différente de `<<` selon le type de `x`. C'est pour ça qu'il n'y a pas
de `%d`.

Le type de retour seul ne suffit pas à distinguer deux surcharges : `int f()`
et `double f()` ne peuvent pas coexister.

### Le piège de la chaîne littérale

```cpp example
#include <iostream>
#include <string>

void log_value(bool b)                 { std::cout << "bool : " << b << '\n'; }
void log_value(const std::string &s)   { std::cout << "texte : " << s << '\n'; }

int main() {
    log_value("attention");   // surprise !
}
```

```output
bool : 1
```

`"attention"` est un `const char *`, pas une `std::string`. Convertir un
pointeur en `bool` est une conversion *standard* du langage, que le
compilateur préfère à la construction d'une `std::string`. Pour l'éviter,
écris `log_value(std::string("attention"))`, ajoute une surcharge
`log_value(const char *)`, ou utilise le suffixe `"attention"s` de C++14
(avec `using namespace std::string_literals;`).

## Les valeurs par défaut

Un paramètre peut avoir une **valeur par défaut**, utilisée quand l'appelant
ne le fournit pas :

```cpp example
#include <iostream>
#include <string>

std::string repeat(const std::string &text, int times = 2, const std::string &sep = " ") {
    std::string result;
    for (int i = 0; i < times; ++i) {
        if (i > 0)
            result += sep;
        result += text;
    }
    return result;
}

int main() {
    std::cout << repeat("ha") << '\n';          // times = 2, sep = " "
    std::cout << repeat("ha", 3) << '\n';       // sep = " "
    std::cout << repeat("ha", 3, "-") << '\n';
}
```

```output
ha ha
ha ha ha
ha-ha-ha
```

Règles :

- **Les valeurs par défaut sont les derniers paramètres.**
  `f(int a = 1, int b)` est interdit.
- **On ne peut pas sauter un paramètre** : pour changer `sep`, il faut aussi
  donner `times`.
- **Avec un en-tête séparé** (`.h` + `.cpp`), la valeur par défaut s'écrit
  **dans la déclaration** (le `.h`), pas dans la définition.

## `auto` : laisser le compilateur déduire le type

```cpp
auto n = 42;              // int
auto price = 9.99;        // double
std::string word = "abc";
auto len = word.size();   // std::size_t : on n'a pas à connaître le nom exact
```

Le type est fixé **à la compilation**, une fois pour toutes : ce n'est pas un
type dynamique comme en Python. `auto` est utile quand le type est long ou
évident, notamment avec la bibliothèque standard. Ne l'utilise pas quand le
type apporte une information au lecteur.

## La boucle `for` sur un intervalle (*range-based for*)

Pour parcourir **tous les éléments** d'une chaîne, d'un tableau ou d'un
conteneur, sans indice :

```cpp example
#include <cctype>
#include <iostream>
#include <string>

int main() {
    int scores[] = {12, 15, 9};
    int total = 0;
    for (int s : scores)            // s prend chaque valeur (copie)
        total += s;
    std::cout << "total : " << total << '\n';

    std::string word = "bonjour";
    for (char &c : word)            // &c : référence, on peut modifier
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    std::cout << word << '\n';

    for (const auto &letter : word) // lecture seule, sans copie : la forme la plus courante
        std::cout << letter << '.';
    std::cout << '\n';
}
```

```output
total : 36
BONJOUR
B.O.N.J.O.U.R.
```

Les trois formes :

| Écriture | Effet |
|---|---|
| `for (T x : c)` | une **copie** de chaque élément (bien pour les petits types) |
| `for (T &x : c)` | une **référence** : modifier `x` modifie l'élément |
| `for (const T &x : c)` | une référence en lecture seule : ni copie, ni modification |

On garde la boucle avec indice quand on a besoin de l'indice, par exemple
pour regarder l'élément précédent.

## `nullptr` au lieu de `NULL`

En C++, le pointeur nul s'écrit `nullptr`. `NULL` est une macro qui vaut un
**entier** (`0`), ce qui piège la surcharge :

```cpp
void f(int);
void f(char *);

f(NULL);     // erreur « appel ambigu » avec g++ et clang ; f(int) ailleurs !
f(nullptr);  // appelle f(char *) : nullptr a un vrai type de pointeur
```

En C++, on n'utilise plus `NULL` ni `0` pour les pointeurs.

## Pièges

- **Les surcharges ambiguës.** Avec `f(int)` et `f(double)`, l'appel
  `f(5L)` (un `long`) est ambigu : le compilateur refuse de choisir.
- **La chaîne littérale qui choisit `bool`** (voir plus haut).
- **`auto` et les références.** `auto x = expr;` fait une copie ; pour une
  référence, écris `auto &x` ou `const auto &x`.
- **Modifier une copie dans une boucle.** `for (char c : word) c = 'x';` ne
  modifie pas `word` : il manque le `&`.

## En entreprise

- **Les boucles `for` sur intervalle** sont la norme pour parcourir un
  conteneur, avec `const auto &` pour lire.
- **La surcharge** sert pour des opérations identiques sur des types
  différents ; on ne la détourne pas pour des actions différentes.
- **Les valeurs par défaut** sont pratiques, mais trop nombreuses elles
  rendent les appels illisibles : au-delà de deux ou trois, on préfère une
  structure d'options.
- **`nullptr`, toujours.**

## Vérifie ta compréhension

1. Peut-on avoir `int parse(const std::string &)` et
   `double parse(const std::string &)` ?
2. Que vaut `word` après `std::string word = "abc"; for (auto c : word) c = 'z';` ?
3. Dans `void draw(int x, int y = 0, int color = 1);`, l'appel `draw(5, , 3)`
   est-il valide ?

### Réponses

1. Non : deux surcharges ne peuvent pas différer seulement par le type de
   retour (à l'appel, rien ne permettrait de choisir).
2. Toujours `"abc"` : `auto c` est une copie de chaque caractère. Il faut
   `auto &c`.
3. Non : on ne peut pas sauter un paramètre. Il faut écrire `draw(5, 0, 3)`.
