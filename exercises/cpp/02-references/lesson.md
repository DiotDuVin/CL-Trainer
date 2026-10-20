id: cpp.lesson.references
title: Références et const
lang: cpp
topic: references
estimated_min: 15
---
## Ce que tu connais en C

En C, pour qu'une fonction modifie une variable de l'appelant, on lui passe un
**pointeur** :

```c
void swap(int *a, int *b) {
    int tmp = *a;
    *a = *b;
    *b = tmp;
}
/* appel : swap(&x, &y); */
```

Ça fonctionne, mais rien n'empêche de passer `NULL`, d'oublier un `*`, ou de
déplacer le pointeur par erreur (`a++`). Pour éviter la copie d'une grosse
structure, on passe aussi un pointeur, en ajoutant `const` pour promettre
qu'on ne la modifie pas : `void print(const struct big *b)`.

## Les références

Une **référence** est un **autre nom** pour une variable qui existe déjà. Ce
n'est pas une nouvelle variable : c'est un alias.

```cpp example
#include <iostream>

int main() {
    int x = 10;
    int &r = x;      // r EST x (même case mémoire)
    r = 42;          // modifie x
    std::cout << x << ' ' << (&r == &x) << '\n';
}
```

```output
42 1
```

Trois règles la distinguent d'un pointeur :

1. **Elle doit être initialisée.** `int &r;` ne compile pas.
2. **Elle ne peut pas changer de cible.** `r = y;` copie la *valeur* de `y`
   dans `x`, sans faire de `r` un alias de `y`.
3. **Il n'existe pas de « référence nulle ».** Elle désigne toujours un objet
   (si on ne triche pas).

Et à l'usage, **on n'écrit ni `*` ni `&`** : on manipule `r` exactement comme
`x`.

## Passage par référence : modifier un argument

```cpp example
#include <iostream>

void swap_values(int &a, int &b) {   // a et b sont des alias des arguments
    int tmp = a;
    a = b;
    b = tmp;
}

int main() {
    int x = 3, y = 7;
    swap_values(x, y);               // pas de & à l'appel
    std::cout << x << ' ' << y << '\n';
}
```

```output
7 3
```

Comparé au C : plus de `*` partout dans la fonction, plus de `&` à l'appel,
et impossible de passer `NULL`.

L'inconvénient : **à l'appel, on ne voit plus que l'argument peut être
modifié.** `swap_values(x, y)` ressemble à un passage par valeur. C'est
pourquoi, pour une fonction qui doit *produire* un résultat, on préfère le
**renvoyer**. Les références modifiables se réservent aux cas où modifier
l'argument est le but de la fonction, comme `swap`.

## `const &` : lire sans copier

Passer une `std::string` **par valeur** la copie entièrement (allocation et
recopie des caractères). Pour une simple lecture, c'est du gaspillage. La
solution est la **référence constante** :

```cpp example
#include <iostream>
#include <string>

// const std::string & : pas de copie, et interdiction de modifier
std::size_t count_spaces(const std::string &text) {
    std::size_t n = 0;
    for (std::size_t i = 0; i < text.size(); ++i)
        if (text[i] == ' ')
            ++n;
    // text += "!";   // ERREUR de compilation : text est const
    return n;
}

int main() {
    std::string sentence = "le C++ en trois mots";
    std::cout << count_spaces(sentence) << '\n';
    std::cout << count_spaces("une chaine litterale aussi") << '\n';
}
```

```output
4
3
```

Remarque le second appel : une `const &` accepte aussi une valeur
temporaire, ici une `std::string` construite à partir de la chaîne littérale.
Une référence **non** constante (`std::string &`) la refuserait : modifier un
temporaire qui va disparaître n'aurait pas de sens.

### Que choisir ?

| Paramètre | Quand |
|---|---|
| `int x` (par valeur) | petits types : `int`, `double`, `char`, `bool`, pointeurs… |
| `const T &x` | gros objets lus seulement : `std::string`, conteneurs, structures |
| `T &x` | la fonction **doit** modifier l'argument (rare : préfère renvoyer) |

## `const` en général

`const` s'applique aussi aux variables, et c'est souvent mieux qu'un `#define` :

```cpp
const int max_users = 100;           // typée, visible du débogueur, respecte les portées
constexpr double pi = 3.14159265358979;  // calculée à la compilation
```

Le réflexe professionnel est le **const par défaut**. Tout ce qui ne doit pas
changer est `const`. Le compilateur vérifie la promesse, et le lecteur sait
d'un coup d'œil ce qui bouge.

Rappel pour les pointeurs (identique en C) : on lit de droite à gauche.

```cpp
const int *p;        // pointeur vers un int constant : *p = 1 interdit
int *const q = &x;   // pointeur constant vers un int : q = &y interdit
```

## Renvoyer plusieurs valeurs

Au lieu de deux paramètres de sortie `int &min, int &max`, le C++17 permet de
renvoyer une **paire** et de la « déballer » à l'arrivée :

```cpp example
#include <iostream>
#include <utility>

std::pair<int, int> min_max(int a, int b) {
    if (a < b)
        return {a, b};
    return {b, a};
}

int main() {
    auto [low, high] = min_max(8, 3);   // C++17 : « structured bindings »
    std::cout << low << ' ' << high << '\n';
}
```

```output
3 8
```

`auto` demande au compilateur de déduire le type (ici `std::pair<int, int>`) ;
on le reverra au chapitre suivant.

## Pièges

- **La référence pendante (*dangling reference*).** Ne renvoie jamais une
  référence vers une variable locale : elle est détruite à la fin de la
  fonction, et la référence désigne alors une zone morte. C'est le même bug
  qu'un pointeur vers une variable locale en C. g++ le signale
  (`-Wreturn-local-addr`), mais pas dans tous les cas.

  ```cpp
  const std::string &bad() {
      std::string s = "temporaire";
      return s;          // s est détruite ici : référence pendante !
  }
  ```

- **Oublier le `&`.** `void reset(int x) { x = 0; }` compile très bien… et
  ne modifie que sa copie locale. C'est l'erreur la plus fréquente au début.
- **`auto` copie.** `auto r = x;` est une copie, même si `x` est une
  référence. Pour une référence, il faut écrire `auto &r = x;`.

## En entreprise

- **Les paramètres de type objet** (`std::string`, conteneurs, classes) se
  passent par `const &`, sauf raison précise. C'est l'une des premières
  choses vérifiées en revue de code.
- **On renvoie les résultats** au lieu d'utiliser des paramètres de sortie.
- **Une référence ne peut pas être nulle** : on l'utilise quand l'argument
  est obligatoire. Un pointeur signifie « peut être absent » (`nullptr`).

## Vérifie ta compréhension

1. Après `int a = 1, b = 2; int &r = a; r = b; r = 5;`, que valent `a` et `b` ?
2. Pourquoi `void f(std::string &s)` refuse-t-il l'appel `f("bonjour")`, alors
   que `void g(const std::string &s)` l'accepte ?
3. Quel type de paramètre pour une fonction qui calcule la moyenne d'une
   `std::string` de notes sans la modifier ?

### Réponses

1. `a = 5`, `b = 2`. `r = b` copie la valeur 2 dans `a` (une référence ne
   change jamais de cible), puis `r = 5` met 5 dans `a`.
2. `"bonjour"` doit d'abord être converti en `std::string` **temporaire**. Une
   référence non constante ne peut pas désigner un temporaire, alors qu'une
   `const &` le peut (le temporaire vit jusqu'à la fin de l'appel).
3. `const std::string &` : un objet, lu seulement.
