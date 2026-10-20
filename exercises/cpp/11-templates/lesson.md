id: cpp.lesson.templates
title: Templates : du code générique sans void *
lang: cpp
topic: templates
estimated_min: 25
---
## Ce que tu connais en C

Pour écrire du code « qui marche avec n'importe quel type », le C a deux
outils, tous deux risqués :

```c
#define MAX(a, b) ((a) > (b) ? (a) : (b))   /* MAX(i++, j) évalue i++ deux fois ! */

void swap_any(void *a, void *b, size_t size);   /* aucune vérification de type */
```

La macro n'est pas une fonction : pas de vérification de types, les arguments
sont évalués plusieurs fois, les erreurs sont incompréhensibles. Le `void *`
fonctionne, mais le compilateur ne sait plus rien des types : une erreur de
`size` ou de cast passe sans un mot.

Les **templates** du C++ donnent la généricité **et** la vérification des
types. Tu les utilises depuis le chapitre 4 : `std::vector<int>`, c'est un
template.

## Les fonctions template

```cpp example
#include <iostream>
#include <string>

template <typename T>              // T : un type, choisi à chaque utilisation
T largest(const T &a, const T &b) {
    return a < b ? b : a;
}

int main() {
    std::cout << largest(3, 7) << '\n';                       // T = int
    std::cout << largest(2.5, -1.0) << '\n';                  // T = double
    std::cout << largest(std::string("pomme"), std::string("poire")) << '\n';   // T = std::string (« m » > « i »)
    std::cout << largest<double>(3, 2.5) << '\n';             // T imposé : 3 converti en double
}
```

```output
7
2.5
pomme
3
```

- **`template <typename T>`** annonce un paramètre de type nommé `T` (on lit
  aussi parfois `class T`, c'est équivalent).
- **Le compilateur déduit `T`** à partir des arguments. Si la déduction est
  ambiguë (`largest(3, 2.5)` : `int` ou `double` ?), il refuse, et on précise
  le type entre chevrons : `largest<double>(3, 2.5)`.
- **Ce n'est pas une fonction, c'est un modèle.** Le compilateur génère (on dit
  qu'il *instancie*) une vraie fonction pour chaque type utilisé :
  `largest<int>`, `largest<double>`… Chacune est aussi rapide qu'une version
  écrite à la main pour ce type.

### Ce que le template exige du type

`largest` utilise `a < b` : elle fonctionne avec **tout type qui possède un
`<`**. Avec un type qui n'en a pas, l'erreur apparaît à la compilation, à
l'endroit de l'instanciation :

```cpp
struct Point { int x, y; };
largest(Point{1, 2}, Point{3, 4});   // erreur : no match for 'operator<'
```

Ces exigences sont implicites, et les messages d'erreur des templates sont
réputés longs : il faut lire la **première** ligne « required from here ».
C++20 permet d'écrire ces exigences explicitement avec les *concepts*
(`template <std::totally_ordered T>`), ce qui produit des messages bien plus
clairs.

## Les classes template

```cpp example
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

template <typename T>
class Stack {
public:
    void push(const T &value) { items_.push_back(value); }
    void pop() { items_.pop_back(); }
    const T &top() const { return items_.back(); }
    bool empty() const { return items_.empty(); }
    std::size_t size() const { return items_.size(); }
private:
    std::vector<T> items_;    // un template peut en utiliser un autre
};

int main() {
    Stack<int> numbers;               // pour une classe : type entre chevrons
    numbers.push(1);
    numbers.push(2);
    Stack<std::string> words;
    words.push("bonjour");
    std::cout << numbers.top() << ' ' << numbers.size() << ' ' << words.top() << '\n';
}
```

```output
2 2 bonjour
```

`Stack<int>` et `Stack<std::string>` sont deux **types différents**, générés
à partir du même modèle. C'est exactement ainsi que sont écrits
`std::vector`, `std::map` ou `std::unique_ptr`.

### Des paramètres qui ne sont pas des types

Un paramètre de template peut aussi être une **valeur constante**, connue à la
compilation. C'est le `N` de `std::array<int, N>` :

```cpp example
#include <iostream>

template <typename T, int N>
T sum(const T (&values)[N]) {     // référence vers un tableau C de N éléments
    T total{};
    for (int i = 0; i < N; ++i)
        total += values[i];
    return total;
}

int main() {
    int a[] = {1, 2, 3, 4};
    double b[] = {0.5, 0.25};
    std::cout << sum(a) << ' ' << sum(b) << '\n';   // N déduit : 4, puis 2
}
```

```output
10 0.75
```

`T total{}` initialise « à la valeur par défaut du type » : 0 pour les
nombres, une chaîne vide pour `std::string`. C'est la bonne façon d'écrire un
zéro générique.

## Où écrire un template ?

**Dans l'en-tête (`.h`), en entier.** Pour instancier `largest<int>`, le
compilateur doit voir le **corps** du template dans le fichier qui l'utilise.
Une déclaration seule ne suffit pas. C'est l'exception à la règle « déclaration
dans le `.h`, définition dans le `.cpp` » (chapitre 14) : oublier ce point
donne une erreur d'édition de liens (*undefined reference*).

## Pièges

- **Déduction ambiguë** : `largest(3, 2.5)` ne compile pas, car `T` ne peut
  pas valoir à la fois `int` et `double`. On précise : `largest<double>(3, 2.5)`.
- **Définir un template dans un `.cpp`** : erreur à l'édition de liens.
- **Les chaînes C dans un template** : avec `T = const char *`
  (`largest<const char *>("abc", "abd")`), `a < b` compare les **adresses**
  des chaînes, pas leur texte. Le résultat dépend de l'emplacement en mémoire.
  Utilise des `std::string`.
- **Des messages d'erreur de 100 lignes** : lis la première erreur et la ligne
  « required from », qui pointe vers TON code.
- **Tout rendre générique « au cas où »** : un template n'a de valeur que s'il
  est vraiment utilisé avec plusieurs types.

## En entreprise

- **On utilise les templates de la bibliothèque standard tous les jours ; on en
  écrit beaucoup moins.** On les écrit pour de vrais outils génériques
  (conteneurs, algorithmes, utilitaires).
- **Des noms parlants** pour les paramètres quand il y en a plusieurs :
  `template <typename Key, typename Value>`.
- **Les concepts (C++20)** pour documenter et vérifier les exigences, quand le
  projet le permet.

## Vérifie ta compréhension

1. Pourquoi `largest(3, 2.5)` ne compile-t-il pas ?
2. Que se passe-t-il si on appelle `largest` avec un type sans `operator<` ?
3. Pourquoi le code d'un template est-il placé dans l'en-tête ?

### Réponses

1. `T` serait `int` d'après le premier argument et `double` d'après le second :
   la déduction échoue. Solution : `largest<double>(3, 2.5)`.
2. Une erreur de compilation, au moment de l'instanciation : le corps utilise
   `a < b`, qui n'existe pas pour ce type.
3. Le compilateur génère le code pour chaque type utilisé : il doit voir le
   corps complet dans chaque fichier qui utilise le template.
