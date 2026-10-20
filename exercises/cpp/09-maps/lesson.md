id: cpp.lesson.maps
title: Conteneurs associatifs : map, set et leurs variantes
lang: cpp
topic: maps
estimated_min: 20
---
## Ce que tu connais en C

Pour associer une clé à une valeur en C (compter des mots, un annuaire…), il
faut tout écrire soi-même : un tableau de paires parcouru linéairement (lent),
un tableau trié avec `bsearch` (insertions pénibles), ou une table de hachage
complète (long et délicat). Le C++ fournit ces structures, prêtes et testées.

| Conteneur | Contenu | Ordre | Recherche / insertion |
|---|---|---|---|
| `std::map<K, V>` | paires clé -> valeur, clés uniques | **trié** par clé | O(log n) (arbre équilibré) |
| `std::set<K>` | clés uniques | **trié** | O(log n) |
| `std::unordered_map<K, V>` | paires clé -> valeur | **aucun** | O(1) en moyenne (table de hachage) |
| `std::unordered_set<K>` | clés uniques | **aucun** | O(1) en moyenne |

En-têtes : `<map>`, `<set>`, `<unordered_map>`, `<unordered_set>`.

## `std::map` : un dictionnaire trié

```cpp example
#include <iostream>
#include <map>
#include <string>

int main() {
    std::map<std::string, int> ages;
    ages["Grace"] = 85;                  // insère ou remplace
    ages["Ada"] = 36;
    ages.insert({"Linus", 54});          // insère (sans remplacer si la clé existe)

    std::cout << "Ada : " << ages["Ada"] << '\n';
    std::cout << "taille : " << ages.size() << '\n';

    for (const auto &[name, age] : ages)  // C++17 : déballage de chaque paire
        std::cout << name << " -> " << age << '\n';   // parcours TRIÉ par clé
}
```

```output
Ada : 36
taille : 3
Ada -> 36
Grace -> 85
Linus -> 54
```

Chaque élément est une **paire** (`std::pair<const K, V>`) : `.first` est la
clé, `.second` la valeur. Avec `auto &[name, age]`, on leur donne directement
un nom. La clé est `const` : on ne peut pas la modifier en place, sinon l'arbre
ne serait plus trié.

### Le piège de `operator[]`

`m[k]` **insère** la clé `k` avec une valeur par défaut (0, chaîne vide…) si
elle n'existe pas. C'est parfait pour compter, mais désastreux pour une simple
recherche :

```cpp example
#include <iostream>
#include <map>
#include <string>

int main() {
    std::map<std::string, int> stock = {{"pommes", 3}};

    if (stock["poires"] == 0)                 // recherche... qui INSÈRE "poires" !
        std::cout << "pas de poires\n";
    std::cout << "taille : " << stock.size() << '\n';   // 2, et non 1

    auto it = stock.find("kiwis");            // vraie recherche : n'insère rien
    if (it == stock.end())
        std::cout << "pas de kiwis\n";
    else
        std::cout << "kiwis : " << it->second << '\n';
    std::cout << "taille : " << stock.size() << '\n';   // toujours 2
    std::cout << "pommes ? " << stock.count("pommes") << '\n';   // 1 ou 0
}
```

```output
pas de poires
taille : 2
pas de kiwis
taille : 2
pommes ? 1
```

- **`find(k)`** renvoie un *itérateur* vers l'élément, ou `end()` s'il est
  absent. Un itérateur, c'est un « curseur » dans le conteneur : `it->first`
  donne la clé, `it->second` la valeur.
- **`count(k)`** renvoie 1 ou 0 pour un `map` (les clés sont uniques). En
  C++20, `contains(k)` est plus lisible.
- **`operator[]` n'existe pas sur un `const std::map`**, puisqu'il pourrait
  insérer. Dans une fonction qui reçoit `const std::map<K, V> &`, on utilise
  `find` ou `at(k)`, qui lance une exception si la clé est absente.

### Compter : le cas idéal pour `operator[]`

```cpp example
#include <iostream>
#include <map>
#include <sstream>
#include <string>

int main() {
    std::istringstream text("le chat et le chien et le rat");
    std::map<std::string, int> counts;
    std::string word;
    while (text >> word)
        ++counts[word];        // absent : créé à 0 puis incrémenté
    for (const auto &[w, n] : counts)
        std::cout << w << ' ' << n << '\n';
}
```

```output
chat 1
chien 1
et 2
le 3
rat 1
```

(`std::istringstream` lit dans une chaîne comme `std::cin` lit l'entrée : idéal
pour un exemple sans clavier.)

## `std::set` : un ensemble sans doublons

```cpp example
#include <iostream>
#include <set>

int main() {
    std::set<int> seen;
    for (int x : {5, 3, 5, 1, 3})
        seen.insert(x);                      // les doublons sont ignorés
    for (int x : seen)
        std::cout << x << ' ';               // trié
    std::cout << "| " << seen.size() << " distincts, contient 4 ? "
              << seen.count(4) << '\n';
}
```

```output
1 3 5 | 3 distincts, contient 4 ? 0
```

## `unordered_map` et `unordered_set` : la table de hachage

Même interface (`[]`, `find`, `count`, `insert`, `erase`), mais une **table de
hachage** à l'intérieur : la recherche est en temps constant en moyenne, au
lieu de logarithmique. En contrepartie, **l'ordre de parcours est
imprévisible** : il dépend de l'implémentation et peut changer d'une exécution
à l'autre ou d'un compilateur à l'autre.

```cpp
std::unordered_map<std::string, std::string> phone;
phone["Ada"] = "01 23 45 67 89";
```

**Lequel choisir ?**

- **`unordered_map`** pour des recherches nombreuses quand l'ordre n'importe
  pas : c'est souvent le plus rapide.
- **`map`** quand on a besoin de parcourir les clés **dans l'ordre**, ou d'une
  sortie reproductible (affichage, tests, fichiers).

## Combiner les conteneurs

Les valeurs peuvent être n'importe quel type, y compris d'autres conteneurs :

```cpp
std::map<std::string, std::vector<int>> grades;   // élève -> ses notes
grades["Ada"].push_back(18);                      // [] crée le vecteur vide au besoin

std::map<std::string, std::set<std::string>> followers;   // personne -> abonnés distincts
```

## Pièges

- **`m[k]` pour vérifier une présence** : insère `k`. Utilise `find` ou
  `count`.
- **Compter sur l'ordre d'un `unordered_map`** : un programme qui affiche
  « dans l'ordre du parcours » peut donner un résultat différent sur une autre
  machine. Les tests automatiques le détectent vite.
- **Supprimer pendant un parcours** : `m.erase(it)` invalide `it`. Il faut
  écrire `it = m.erase(it);` (erase renvoie l'élément suivant) au lieu de
  `++it`.
- **Utiliser une `std::map` pour 5 éléments fixes** : un simple `std::vector`
  de paires (ou un `std::array`) est souvent plus rapide pour de très petites
  tailles.

## En entreprise

- **`std::unordered_map` par défaut** pour un dictionnaire, `std::map` quand
  l'ordre compte ou pour une sortie stable.
- **`find` pour chercher, `[]` pour insérer ou mettre à jour**, en connaissance
  de cause.
- **`const auto &[cle, valeur]`** pour parcourir, sans copier les paires.
- **Choisir la structure d'abord.** Remplacer un parcours linéaire par un
  `unordered_set` change souvent un programme de O(n²) à O(n).

## Vérifie ta compréhension

1. Que contient `m` après `std::map<std::string, int> m; if (m["x"] > 0) {}` ?
2. Tu dois afficher des mots et leur fréquence par ordre alphabétique. `map`
   ou `unordered_map` ?
3. Comment savoir si une clé est présente dans un `const std::map &` ?

### Réponses

1. Une entrée `"x" -> 0` : `operator[]` l'a insérée.
2. `std::map` : son parcours est trié par clé, et il n'y a rien d'autre à faire.
3. `m.find(cle) != m.end()` ou `m.count(cle) != 0` (`operator[]` n'est pas
   disponible sur un map constant).
