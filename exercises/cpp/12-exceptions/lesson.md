id: cpp.lesson.exceptions
title: Exceptions : signaler et traiter les erreurs
lang: cpp
topic: exceptions
estimated_min: 25
---
## Ce que tu connais en C

```c
int load_config(const char *path, struct config *out) {
    FILE *f = fopen(path, "r");
    if (!f)
        return -1;                    /* l'erreur remonte par la valeur de retour */
    if (parse(f, out) != 0) {
        fclose(f);
        return -1;
    }
    fclose(f);
    return 0;
}
/* chaque appelant doit tester : if (load_config(...) != 0) { ... } */
```

C'est la convention de trainer lui-même (0 / -1 et `err_msg()`). Elle a deux
faiblesses : **rien n'oblige à tester** le code de retour (une erreur ignorée
passe en silence), et chaque fonction intermédiaire doit **retransmettre**
l'erreur à la main, ce qui alourdit tout le code.

Le C++ ajoute un deuxième mécanisme : les **exceptions**.

## `throw`, `try`, `catch`

```cpp example
#include <iostream>
#include <stdexcept>

double divide(double a, double b) {
    if (b == 0)
        throw std::invalid_argument("division par zéro");   // lance une exception
    return a / b;
}

double average_ratio(double a, double b, double c) {
    return (divide(a, b) + divide(a, c)) / 2;   // ne traite rien : l'exception traverse
}

int main() {
    try {
        std::cout << average_ratio(10, 2, 5) << '\n';
        std::cout << average_ratio(10, 2, 0) << '\n';   // lance
        std::cout << "jamais affiché\n";
    } catch (const std::invalid_argument &e) {           // attrapée ici
        std::cout << "erreur : " << e.what() << '\n';
    }
    std::cout << "le programme continue\n";
}
```

```output
3.5
erreur : division par zéro
le programme continue
```

- **`throw`** interrompt la fonction et **remonte la pile d'appels** jusqu'au
  premier `catch` dont le type correspond. Les fonctions traversées
  (`average_ratio`) n'ont rien à écrire.
- **`catch (const Type &e)`** : on attrape **toujours par référence
  constante**. Par valeur, on copierait l'exception et on risquerait de la
  « tronquer » (voir le chapitre 13).
- **`e.what()`** renvoie le message.
- **Une exception que personne n'attrape** termine le programme
  (`std::terminate`).

## La hiérarchie standard

Les exceptions de la bibliothèque standard héritent toutes de
`std::exception` (en-tête `<stdexcept>`). Un `catch` sur une classe de base
attrape aussi ses dérivées.

```
std::exception
├── std::logic_error         erreurs de programmation, évitables
│   ├── std::invalid_argument      argument invalide
│   └── std::out_of_range          indice hors limites (vector::at, stoi...)
├── std::runtime_error       erreurs détectables seulement à l'exécution
└── std::bad_alloc           mémoire épuisée (lancée par new)
```

```cpp example
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

int main() {
    std::vector<int> v = {1, 2, 3};
    const std::string inputs[] = {"42", "abc", "99999999999999999999"};

    for (const auto &text : inputs) {
        try {
            int n = std::stoi(text);       // texte -> int, lance en cas d'échec
            std::cout << text << " -> " << n << '\n';
        } catch (const std::invalid_argument &) {
            std::cout << text << " : pas un nombre\n";
        } catch (const std::out_of_range &) {
            std::cout << text << " : trop grand pour un int\n";
        }
    }

    try {
        std::cout << v.at(10) << '\n';     // at vérifie l'indice, [] ne le ferait pas
    } catch (const std::exception &e) {    // la base attrape toutes les dérivées
        std::cout << "exception standard attrapée\n";
    }
}
```

```output
42 -> 42
abc : pas un nombre
99999999999999999999 : trop grand pour un int
exception standard attrapée
```

Les `catch` sont essayés **dans l'ordre** : place les types les plus précis
d'abord, et `std::exception` en dernier.

### Tes propres exceptions

On dérive d'une exception standard, souvent `std::runtime_error`, dont le
constructeur prend le message :

```cpp
class InsufficientFunds : public std::runtime_error {
public:
    explicit InsufficientFunds(long missing)
        : std::runtime_error("fonds insuffisants"), missing_(missing) {}
    long missing() const { return missing_; }
private:
    long missing_;
};
```

L'héritage (`: public std::runtime_error`) est expliqué au chapitre 13.

## Exceptions et RAII : le nettoyage garanti

Quand une exception remonte la pile, **les destructeurs de tous les objets
locaux traversés sont appelés** (on parle de *stack unwinding*). C'est ce qui
rend les exceptions sûres en C++ : avec RAII, rien ne fuit.

```cpp example
#include <iostream>
#include <stdexcept>
#include <string>

struct Guard {
    std::string name;
    ~Guard() { std::cout << "libère " << name << '\n'; }
};

void step2() {
    Guard g{"connexion"};
    throw std::runtime_error("réseau coupé");
}

void step1() {
    Guard g{"fichier"};
    step2();
    std::cout << "jamais affiché\n";
}

int main() {
    try {
        step1();
    } catch (const std::exception &e) {
        std::cout << "erreur : " << e.what() << '\n';
    }
}
```

```output
libère connexion
libère fichier
erreur : réseau coupé
```

Comparé au C : aucun `fclose`, `free` ni `goto` à écrire. Même si l'erreur
survient au fond de trois appels, tout est libéré dans le bon ordre.

## Exceptions ou valeur de retour ?

Les exceptions servent aux **situations exceptionnelles** : celles où la
fonction ne peut pas faire son travail et où l'appelant direct ne saurait
souvent pas quoi faire (fichier de configuration illisible, mémoire épuisée,
invariant violé).

Pour un **échec attendu et fréquent** (un utilisateur qui tape un mauvais
nombre, une recherche sans résultat), une valeur de retour est plus simple et
plus rapide. C++17 propose `std::optional<T>` (en-tête `<optional>`) :
« un `T`, ou rien ».

```cpp
std::optional<int> parse_age(const std::string &text);   // vide si invalide
if (auto age = parse_age(input))
    std::cout << *age;
```

## `noexcept`

`noexcept` promet qu'une fonction ne lance jamais d'exception (on l'a vu pour
le déplacement au chapitre 8). Si elle en lance quand même, le programme
s'arrête immédiatement. Les destructeurs sont `noexcept` par défaut : **un
destructeur ne doit jamais laisser échapper d'exception.**

## Pièges

- **Attraper par valeur** (`catch (std::exception e)`) : copie et troncature.
  Toujours `catch (const std::exception &e)`.
- **`catch (...)` qui avale tout** sans rien faire : l'erreur disparaît.
  Attrape seulement ce que tu sais traiter.
- **`std::stoi("12abc")` renvoie 12 sans erreur** : il s'arrête au premier
  caractère invalide. Pour exiger un nombre complet, utilise son deuxième
  paramètre (le nombre de caractères lus) et vérifie qu'il vaut la longueur
  du texte.
- **Les exceptions pour le contrôle de flux normal** (sortir d'une boucle,
  signaler « pas trouvé ») : c'est lent et illisible.
- **Lancer depuis un destructeur** : `std::terminate`.

## En entreprise

- **RAII partout** : c'est la condition pour que les exceptions soient sûres.
- **Des exceptions qui dérivent de `std::exception`**, avec un message utile.
- **On attrape au bon niveau** : là où l'on sait réagir (afficher un message,
  réessayer, annuler une opération), souvent assez haut dans le programme.
- **Certaines équipes interdisent les exceptions** (jeux vidéo, embarqué,
  code de Google) pour des raisons de performance ou de prévisibilité. Elles
  utilisent alors des codes d'erreur ou des types comme `std::optional` et
  `std::expected` (C++23).

## Vérifie ta compréhension

1. Pourquoi attraper par `const &` ?
2. Que se passe-t-il pour les objets locaux d'une fonction traversée par une
   exception ?
3. Que renvoie `std::stoi("12abc")` ?

### Réponses

1. Pour ne pas copier l'exception, et pour conserver son vrai type dynamique
   (attrapée par valeur dans un `catch (std::exception e)`, une
   `std::out_of_range` serait tronquée en simple `std::exception`).
2. Leurs destructeurs sont appelés, dans l'ordre inverse de construction :
   c'est le *stack unwinding*, qui garantit le nettoyage avec RAII.
3. 12, sans erreur : la conversion s'arrête au premier caractère non chiffre.
   Il faut vérifier le nombre de caractères lus pour refuser ce texte.
