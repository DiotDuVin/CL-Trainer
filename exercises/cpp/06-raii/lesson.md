id: cpp.lesson.raii
title: Destructeurs et RAII
lang: cpp
topic: raii
estimated_min: 25
---
## Ce que tu connais en C

```c
int process(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f)
        return -1;
    char *buf = malloc(4096);
    if (!buf) {
        fclose(f);              /* ne pas oublier... */
        return -1;
    }
    if (read_header(f, buf) != 0) {
        free(buf);              /* ... ni ici ... */
        fclose(f);
        return -1;
    }
    /* ... */
    free(buf);                  /* ... ni là */
    fclose(f);
    return 0;
}
```

Chaque sortie de la fonction doit libérer ce qui a été acquis, dans le bon
ordre. Le code de trainer utilise le motif `goto out;` pour n'avoir qu'un seul
chemin de libération. Ça fonctionne, mais c'est à la discipline du
programmeur : un oubli, et c'est une fuite.

Le C++ automatise ce nettoyage avec le **destructeur**.

## Le destructeur

Le destructeur porte le nom de la classe précédé de `~`. Il n'a ni paramètre
ni type de retour, et il est appelé **automatiquement** quand l'objet est
détruit. Pour une variable locale, c'est à la fin de sa portée (l'accolade
fermante), **quelle que soit la façon d'en sortir** : fin normale, `return`,
`break`, exception.

```cpp example
#include <iostream>
#include <string>

class Tracer {
public:
    explicit Tracer(const std::string &name) : name_(name) {
        std::cout << "+ " << name_ << '\n';      // acquisition
    }
    ~Tracer() {                                  // destructeur
        std::cout << "- " << name_ << '\n';      // libération
    }
private:
    std::string name_;
};

void work(bool stop_early) {
    Tracer a("a");
    if (stop_early)
        return;                 // ~Tracer() de a est appelé ici aussi
    Tracer b("b");
    std::cout << "travail\n";
}                               // b puis a sont détruits ici

int main() {
    work(false);
    work(true);
    {
        Tracer c("bloc");
    }                           // fin du bloc : c est détruit
    std::cout << "fin\n";
}
```

```output
+ a
+ b
travail
- b
- a
+ a
- a
+ bloc
- bloc
fin
```

Deux règles à retenir :

1. **La destruction est automatique et garantie** pour les objets locaux, sur
   tous les chemins de sortie.
2. **Les objets sont détruits dans l'ordre inverse de leur construction**
   (`b` avant `a`), comme une pile. C'est logique : `b` peut dépendre de `a`.

## RAII : lier une ressource à la vie d'un objet

**RAII** (*Resource Acquisition Is Initialization*) est l'idée centrale du
C++ : **chaque ressource appartient à un objet**. Le constructeur l'acquiert,
le destructeur la libère. Une ressource, c'est de la mémoire, un fichier, une
connexion réseau, un verrou…

Tu l'utilises déjà :

- **`std::string` et `std::vector`** libèrent leur mémoire dans leur
  destructeur : c'est pour ça que tu n'as jamais écrit `free` ;
- **`std::ofstream`** (fichier en écriture, en-tête `<fstream>`) ferme son
  fichier dans son destructeur ;
- **`std::lock_guard`** libère un verrou de *mutex* à la fin du bloc.

La fonction C du début devient :

```cpp
int process(const std::string &path) {
    std::ifstream f(path);              // ouvert ici...
    if (!f)
        return -1;
    std::vector<char> buf(4096);        // ... alloué ici
    if (read_header(f, buf) != 0)
        return -1;                      // tout est libéré automatiquement
    // ...
    return 0;
}                                       // ... et libéré ici, dans l'ordre inverse
```

Plus aucun nettoyage à écrire, donc plus aucun oubli possible.

## `new` et `delete` (pour comprendre ce qu'il y a dessous)

`new` est l'équivalent C++ de `malloc` : il alloue **et** construit l'objet.
`delete` détruit l'objet (appelle son destructeur) **et** libère la mémoire.

```cpp
int *p = new int(42);        // un int valant 42
delete p;

int *t = new int[100]();     // tableau de 100 int, () : initialisés à 0
delete[] t;                  // tableau : delete[] et pas delete !
```

On ne les écrit **presque jamais directement** en C++ moderne : un
`std::vector` ou un pointeur intelligent (chapitre 7) le fait pour nous. Mais
pour écrire une classe RAII, il faut savoir ce qu'elle encapsule.

## Une classe RAII à la main… et le problème de la copie

```cpp
class IntBuffer {
public:
    explicit IntBuffer(std::size_t n) : size_(n), data_(new int[n]()) {}
    ~IntBuffer() { delete[] data_; }
    int &operator[](std::size_t i) { return data_[i]; }
private:
    std::size_t size_;
    int *data_;
};

IntBuffer a(10);
IntBuffer b = a;   // copie... de quoi ?
```

Par défaut, copier un objet copie **chaque membre**. On copie donc le
*pointeur* `data_`, pas le tableau pointé. `a` et `b` partagent alors le même
bloc. À la fin de la portée, les deux destructeurs appellent `delete[]` sur
la même adresse : c'est une **double libération**, un comportement indéfini
(AddressSanitizer la signale : *attempting double-free*).

### La règle de trois

> Si une classe a besoin d'un **destructeur** personnalisé, elle a aussi
> besoin d'un **constructeur de copie** et d'un **opérateur d'affectation par
> copie**, ou alors elle doit **interdire** la copie.

```cpp example
#include <algorithm>
#include <cstddef>
#include <iostream>
#include <utility>

class IntBuffer {
public:
    explicit IntBuffer(std::size_t n) : size_(n), data_(new int[n]()) {}

    ~IntBuffer() { delete[] data_; }                       // 1. destructeur

    IntBuffer(const IntBuffer &other)                      // 2. constructeur de copie
        : size_(other.size_), data_(new int[other.size_]) {
        std::copy(other.data_, other.data_ + size_, data_);   // copie PROFONDE
    }

    IntBuffer &operator=(IntBuffer other) {                // 3. affectation par copie
        std::swap(size_, other.size_);                     //    (idiome copy-and-swap)
        std::swap(data_, other.data_);
        return *this;
    }   // other (qui contient maintenant l'ancien tableau) est détruit ici

    int &operator[](std::size_t i) { return data_[i]; }
    std::size_t size() const { return size_; }

private:
    std::size_t size_;
    int *data_;
};

int main() {
    IntBuffer a(3);
    a[0] = 7;
    IntBuffer b = a;       // constructeur de copie : tableau à part
    b[0] = 99;
    IntBuffer c(1);
    c = a;                 // affectation : c reçoit sa propre copie
    std::cout << a[0] << ' ' << b[0] << ' ' << c[0] << ' ' << c.size() << '\n';
}                          // trois destructeurs, trois tableaux distincts : aucun problème
```

```output
7 99 7 3
```

L'idiome **copy-and-swap** est élégant : le paramètre `other` est reçu
**par valeur**, donc déjà copié par le constructeur de copie. On échange
ensuite notre contenu avec cette copie. Le destructeur de `other` libère
l'ancien tableau. L'auto-affectation (`a = a`) fonctionne, et si la copie
échoue, l'objet n'a pas été modifié.

### Interdire la copie

Certaines ressources ne doivent pas être copiées (un fichier ouvert, une
connexion). On supprime alors les opérations de copie avec `= delete` :

```cpp
class Connection {
public:
    Connection(const Connection &) = delete;             // pas de copie
    Connection &operator=(const Connection &) = delete;  // pas d'affectation
    // ...
};
```

Toute tentative de copie devient une **erreur de compilation**, bien plus
sûre qu'un bug à l'exécution. C'est ce que fait la classe `Engine` de
l'application graphique de trainer.

### La règle de zéro

La meilleure solution est souvent… de ne rien écrire du tout. Si les membres
gèrent eux-mêmes leurs ressources (`std::vector<int>` au lieu de `int *`), les
opérations générées par défaut sont **correctes** : copier la classe copie le
vecteur, qui sait se copier. **Pas de destructeur, pas de copie à écrire.**

```cpp
class IntBuffer {           // version « règle de zéro »
public:
    explicit IntBuffer(std::size_t n) : data_(n) {}
    int &operator[](std::size_t i) { return data_[i]; }
    std::size_t size() const { return data_.size(); }
private:
    std::vector<int> data_;  // le vecteur gère tout
};
```

On n'écrit une classe à la règle de trois que pour **encapsuler une ressource
bas niveau**, et une seule ressource par classe. Le reste du code compose ces
classes en suivant la règle de zéro.

(La « règle de cinq » ajoute deux opérations de *déplacement* : chapitre 8.)

## Pièges

- **`delete` au lieu de `delete[]`** (ou l'inverse) : comportement indéfini.
  Chaque `new[]` va avec un `delete[]`.
- **Écrire un destructeur sans gérer la copie** : double libération à la
  première copie, souvent cachée (passage par valeur, `push_back` dans un
  vecteur…).
- **Lancer une exception depuis un destructeur** : à proscrire. Un destructeur
  est appelé pendant la propagation d'une exception, et une deuxième exception
  à ce moment arrête le programme (`std::terminate`).
- **Croire qu'un pointeur nu « possède » ce qu'il pointe.** Un `int *` ne
  libère rien tout seul : seul un objet RAII le fait.

## En entreprise

- **Pas de `new`/`delete` visibles dans le code applicatif** : conteneurs et
  pointeurs intelligents (chapitre 7).
- **Règle de zéro par défaut.** La règle de trois (ou cinq) ne sert qu'aux
  rares classes qui encapsulent une ressource brute.
- **Toute ressource a un propriétaire RAII** : fichiers, sockets, verrous,
  handles d'API C (avec une petite classe d'enveloppe).
- **Classe non copiable** : `= delete` explicite, plutôt que de compter sur la
  discipline.

## Vérifie ta compréhension

1. Dans quel ordre sont détruits `x`, `y` et `z`, déclarés dans cet ordre dans
   une fonction ?
2. Pourquoi la copie par défaut de la première version d'`IntBuffer` est-elle
   dangereuse ?
3. Une classe dont les membres sont un `std::string` et un `std::vector<int>`
   a-t-elle besoin d'un destructeur ?

### Réponses

1. `z`, puis `y`, puis `x` : l'ordre inverse de la construction.
2. Elle copie le pointeur, pas le tableau : deux objets partagent le même bloc
   et le libèrent chacun, d'où une double libération (et une modification de
   l'un visible dans l'autre).
3. Non (règle de zéro) : `std::string` et `std::vector` libèrent eux-mêmes
   leur mémoire, et la copie par défaut de la classe les copie correctement.
