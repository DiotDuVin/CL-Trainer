id: cpp.lesson.move
title: Copier ou déplacer : la sémantique de déplacement
lang: cpp
topic: move
estimated_min: 25
---
## Ce que tu connais en C

En C, pour « donner » un gros tableau à une autre partie du programme, on ne
le copie pas : on transmet le **pointeur**. Le tableau ne bouge pas, seule
l'adresse change de main.

```c
struct big *a = make_big();   /* le contenu est alloué sur le tas */
struct big *b = a;            /* « transfert » : on copie juste le pointeur */
a = NULL;                     /* et on oublie l'ancien */
```

En C++, une `std::string` ou un `std::vector` possèdent aussi un bloc sur le
tas. Les copier (chapitre 6) copie tout le bloc, ce qui est coûteux. Mais si
l'original ne sert plus, on pourrait se contenter de **voler son bloc**,
comme en C. C'est le **déplacement** (*move*), ajouté en C++11.

## Copier ou déplacer

```cpp example
#include <iostream>
#include <string>
#include <utility>
#include <vector>

int main() {
    std::vector<int> a(1000000, 7);          // un million d'entiers
    std::vector<int> b = a;                  // COPIE : un 2e bloc, un million de recopies
    std::vector<int> c = std::move(a);       // DÉPLACEMENT : c vole le bloc de a
    std::cout << b.size() << ' ' << c.size() << '\n';

    std::string s = "un texte assez long pour être alloué sur le tas";
    std::string t = std::move(s);            // t récupère les caractères de s
    std::cout << t.size() << '\n';
}
```

```output
1000000 1000000
49
```

(49 et pas 47 : « ê » et « é » occupent chacun 2 octets en UTF-8, chapitre 1.)

Un déplacement ne copie que quelques pointeurs et tailles : c'est quasi gratuit,
quelle que soit la taille des données.

### Que devient l'original ?

Après `std::move(a)`, `a` est dans un état **valide mais non spécifié** : on
peut le détruire ou lui affecter une nouvelle valeur, mais **pas compter sur
son contenu**. En pratique, un vecteur déplacé est vide, mais la norme ne le
garantit pas pour tous les types. La règle : **on ne lit pas un objet après
l'avoir déplacé**, on le réaffecte ou on le laisse mourir.

## Ce que fait vraiment `std::move`

**`std::move` ne déplace rien.** C'est une simple conversion (un *cast*) qui
dit au compilateur : « tu peux traiter cet objet comme un temporaire, je ne
m'en servirai plus ». Le déplacement lui-même est fait par le **constructeur
de déplacement** (ou l'**affectation par déplacement**) du type, que le
compilateur choisit alors à la place de la copie.

Le C++ distingue deux sortes d'expressions :

- les **lvalues**, qui ont un nom et une adresse (`a`, `v[0]`) : on ne peut pas
  les piller, on s'en sert encore ;
- les **rvalues**, qui sont des temporaires (`f()`, `x + 1`, `std::string("a")`)
  ou des objets marqués par `std::move` : on peut les piller sans risque.

Le paramètre d'un constructeur de déplacement est une **référence vers une
rvalue**, notée `T &&` :

```cpp
class Buffer {
public:
    Buffer(const Buffer &other);   // copie    : appelée pour une lvalue
    Buffer(Buffer &&other);        // déplacement : appelé pour une rvalue
};
```

## Écrire un constructeur de déplacement

Pour une classe qui possède une ressource brute (règle de trois, chapitre 6),
le déplacement consiste à **voler** le pointeur et à **vider** la source, pour
que son destructeur ne libère rien :

```cpp example
#include <cstddef>
#include <iostream>
#include <utility>

class Buffer {
public:
    explicit Buffer(std::size_t n) : size_(n), data_(new int[n]()) {}
    ~Buffer() { delete[] data_; }                          // delete[] nullptr : sans effet

    Buffer(const Buffer &) = delete;                        // (copie omise pour l'exemple)
    Buffer &operator=(const Buffer &) = delete;

    Buffer(Buffer &&other) noexcept                        // déplacement
        : size_(other.size_), data_(other.data_) {         // on vole...
        other.size_ = 0;                                   // ... et on vide la source
        other.data_ = nullptr;
    }

    Buffer &operator=(Buffer &&other) noexcept {           // affectation par déplacement
        std::swap(size_, other.size_);                     // échange : other emportera
        std::swap(data_, other.data_);                     // notre ancien tableau
        return *this;
    }

    std::size_t size() const { return size_; }

private:
    std::size_t size_;
    int *data_;
};

int main() {
    Buffer a(100);
    Buffer b = std::move(a);       // constructeur de déplacement
    Buffer c(5);
    c = std::move(b);              // affectation par déplacement
    std::cout << a.size() << ' ' << b.size() << ' ' << c.size() << '\n';
}
```

```output
0 5 100
```

Remarque `b.size()` : après l'affectation par échange, `b` a récupéré l'ancien
contenu de `c` (5 éléments). C'est permis, puisqu'un objet déplacé est
« valide mais non spécifié ».

### La règle de cinq

Une classe qui gère elle-même une ressource écrit les **cinq** opérations :
destructeur, constructeur de copie, affectation par copie, constructeur de
déplacement et affectation par déplacement (ou en supprime certaines avec
`= delete`). Et la **règle de zéro** reste la meilleure : avec des membres
`std::vector` ou `std::unique_ptr`, les cinq opérations générées
automatiquement sont correctes.

### Pourquoi `noexcept` compte

Quand un `std::vector` s'agrandit, il transfère ses éléments dans le nouveau
bloc. S'il les **déplaçait** et qu'un déplacement lançait une exception au
milieu, le vecteur serait à moitié vidé et impossible à remettre en état. Il
ne déplace donc que si le constructeur de déplacement est déclaré
**`noexcept`** (« ne lance jamais d'exception ») ; sinon, il **copie**.

```cpp example
#include <iostream>
#include <vector>

struct Safe {
    Safe() = default;
    Safe(const Safe &) { std::cout << "copie "; }
    Safe(Safe &&) noexcept { std::cout << "déplacement "; }
};

struct Unsafe {
    Unsafe() = default;
    Unsafe(const Unsafe &) { std::cout << "copie "; }
    Unsafe(Unsafe &&) { std::cout << "déplacement "; }   // pas de noexcept
};

int main() {
    std::vector<Safe> s(2);
    s.reserve(10);            // agrandissement : les 2 éléments sont transférés
    std::cout << '\n';
    std::vector<Unsafe> u(2);
    u.reserve(10);
    std::cout << '\n';
}
```

```output
déplacement déplacement 
copie copie 
```

**Marque toujours tes opérations de déplacement `noexcept`** (elles ne font que
copier des pointeurs, elles ne peuvent pas échouer).

## Le déplacement au quotidien

- **Renvoyer par valeur est gratuit.** `return result;` sur une variable
  locale ne la copie pas : le compilateur la construit directement chez
  l'appelant, ou à défaut la déplace. N'écris **pas**
  `return std::move(result);` : ça empêche cette optimisation (g++ le signale
  avec `-Wpessimizing-move`, activé par `-Wall`).
- **Les paramètres « puits ».** Une fonction qui va **garder** une chaîne
  (un constructeur, par exemple) la prend par valeur et la déplace :

  ```cpp
  class User {
  public:
      explicit User(std::string name) : name_(std::move(name)) {}
  private:
      std::string name_;
  };
  User u1("Ada");          // temporaire : déplacé, aucune copie
  User u2(some_name);      // lvalue : une seule copie (dans le paramètre)
  ```

- **Mettre un objet dans un conteneur sans le copier** :
  `v.push_back(std::move(s));`, ou `v.emplace_back(arguments…)` qui construit
  l'élément directement dans le vecteur.
- **`std::unique_ptr` ne se copie pas, il se déplace** : c'est ce que tu as
  écrit au chapitre 7 avec `std::move(head_)`.

## Pièges

- **Utiliser un objet après `std::move`.** Son contenu n'est plus fiable.
  Réaffecte-le d'abord (`s = "nouveau";`).
- **`std::move` sur un objet `const`** : le déplacement doit modifier la
  source, ce qui est impossible sur un `const`. Le compilateur choisit alors la
  **copie**, en silence.
- **Oublier `noexcept`** sur le constructeur de déplacement : les vecteurs
  copient au lieu de déplacer.
- **`return std::move(local);`** : inutile, et même plus lent.

## En entreprise

- **Règle de zéro** : la plupart des classes n'écrivent aucune des cinq
  opérations.
- **Renvoyer par valeur** sans s'inquiéter du coût, et sans `std::move`.
- **Paramètre puits** : passage par valeur, puis `std::move` dans le membre.
- **`noexcept`** sur toute opération de déplacement écrite à la main.

## Vérifie ta compréhension

1. Que fait `std::move(x)` à lui seul ?
2. Après `std::vector<int> b = std::move(a);`, que peut-on faire de `a` ?
3. Pourquoi `std::vector` copie-t-il les éléments de type `Unsafe` lors d'un
   agrandissement ?

### Réponses

1. Rien à l'exécution : c'est une conversion en rvalue, qui permet au
   compilateur de choisir le déplacement plutôt que la copie.
2. Le détruire ou lui affecter une nouvelle valeur. Pas compter sur son
   contenu (en pratique vide, mais ce n'est pas garanti pour tous les types).
3. Son constructeur de déplacement n'est pas `noexcept` : un déplacement qui
   échouerait au milieu laisserait le vecteur dans un état incohérent. La copie
   permet de tout annuler si elle échoue.
