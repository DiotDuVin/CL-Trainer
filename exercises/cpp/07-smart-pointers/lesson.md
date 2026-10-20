id: cpp.lesson.smart-pointers
title: Pointeurs intelligents
lang: cpp
topic: smart-pointers
estimated_min: 25
---
## Ce que tu connais en C

```c
struct node *make_list(void);      /* qui doit libérer la liste ? l'appelant ? */
const char *get_name(struct user *u);   /* et ici ? faut-il free le résultat ? */
```

En C, un pointeur ne dit pas **qui est responsable de libérer** ce qu'il
désigne. On le note dans la documentation… quand on y pense. Résultat : des
fuites (personne ne libère), des doubles libérations (deux responsables) et
des *use-after-free* (on utilise après que le responsable a libéré).

Le C++ exprime cette responsabilité, qu'on appelle la **possession**
(*ownership*), **dans le type** :

| Type | Signification |
|---|---|
| `std::unique_ptr<T>` | je suis **le seul propriétaire** : je libère à ma destruction |
| `std::shared_ptr<T>` | **propriété partagée** : le dernier propriétaire libère |
| `T *` ou `T &` | j'**observe** seulement : je ne libère jamais |

Tous sont dans l'en-tête `<memory>`. Ce sont des classes RAII (chapitre 6) :
leur destructeur fait le `delete` à ta place.

## `std::unique_ptr` : un seul propriétaire

```cpp example
#include <iostream>
#include <memory>
#include <string>

class Sensor {
public:
    explicit Sensor(const std::string &name) : name_(name) { std::cout << "+ " << name_ << '\n'; }
    ~Sensor() { std::cout << "- " << name_ << '\n'; }
    const std::string &name() const { return name_; }
private:
    std::string name_;
};

int main() {
    std::unique_ptr<Sensor> a = std::make_unique<Sensor>("température");
    std::cout << "capteur : " << a->name() << '\n';   // -> comme un pointeur

    {
        auto b = std::make_unique<Sensor>("pression");
    }                                   // b détruit ici : delete automatique

    a.reset();                          // libère maintenant
    std::cout << "a est " << (a ? "plein" : "vide") << '\n';
}
```

```output
+ température
capteur : température
+ pression
- pression
- température
a est vide
```

- **`std::make_unique<T>(arguments…)`** construit l'objet sur le tas et
  renvoie le `unique_ptr` (C++14). C'est le remplaçant de `new`.
- **On l'utilise comme un pointeur** : `*p`, `p->membre`, et `if (p)` pour
  tester s'il est vide.
- **`p.reset()`** libère l'objet tout de suite ; **`p.get()`** donne le
  pointeur brut, pour observer sans prendre la propriété.

### Pas de copie, mais un transfert

Un `unique_ptr` **ne peut pas être copié** (sinon il y aurait deux
propriétaires) : `auto b = a;` ne compile pas. On peut en revanche **transférer**
la propriété avec `std::move` :

```cpp
auto a = std::make_unique<Sensor>("x");
auto b = std::move(a);   // b possède l'objet, a est maintenant vide (nullptr)
```

`std::move` est expliqué en détail au chapitre 8. Pour l'instant, retiens :
« je donne cet objet, je ne m'en servirai plus ».

Un `unique_ptr` ne coûte **rien** de plus qu'un pointeur brut : même taille,
même vitesse. Il n'y a aucune raison de s'en priver.

### Quand allouer sur le tas ?

La plupart des objets vivent très bien **sur la pile** (variables locales) ou
**dans un conteneur** (`std::vector<Sensor>`). On a besoin du tas, donc d'un
pointeur intelligent, quand :

- **un objet doit survivre** à la fonction qui le crée, sans être copié ;
- **la structure est récursive** : liste chaînée, arbre (un nœud contient ses
  enfants) ;
- **pour le polymorphisme** (chapitre 13), avec un `std::vector<std::unique_ptr<Base>>`.

```cpp example
#include <iostream>
#include <memory>

struct Node {                          // liste chaînée : chaque nœud POSSÈDE le suivant
    int value;
    std::unique_ptr<Node> next;
};

int main() {
    std::unique_ptr<Node> head;
    for (int v = 1; v <= 3; ++v)       // insertion en tête
        head = std::make_unique<Node>(Node{v, std::move(head)});

    for (const Node *n = head.get(); n != nullptr; n = n->next.get())   // observer : pointeur brut
        std::cout << n->value << ' ';
    std::cout << '\n';
}   // head détruit -> détruit son next -> ... : toute la liste est libérée
```

```output
3 2 1 
```

Pas une seule ligne de libération, et AddressSanitizer ne signale aucune fuite.

## `std::shared_ptr` : la propriété partagée

Quand plusieurs objets doivent **partager** une ressource sans qu'aucun ne
soit « le » propriétaire, `std::shared_ptr` tient un **compteur de
références**. Chaque copie l'incrémente, chaque destruction le décrémente, et
l'objet est libéré quand il atteint 0.

```cpp example
#include <iostream>
#include <memory>
#include <string>

struct Texture {
    std::string file;
    ~Texture() { std::cout << "texture " << file << " libérée\n"; }
};

int main() {
    auto t = std::make_shared<Texture>(Texture{"herbe.png"});
    std::cout << "propriétaires : " << t.use_count() << '\n';
    {
        std::shared_ptr<Texture> copy = t;     // copie : le compteur passe à 2
        std::cout << "propriétaires : " << t.use_count() << '\n';
    }                                          // copy détruit : retour à 1
    std::cout << "propriétaires : " << t.use_count() << '\n';
    t.reset();                                 // 0 : libération
    std::cout << "fin\n";
}
```

```output
texture herbe.png libérée
propriétaires : 1
propriétaires : 2
propriétaires : 1
texture herbe.png libérée
fin
```

(La première ligne vient du `Texture{"herbe.png"}` **temporaire** passé à
`make_shared`. Il est copié dans l'objet partagé, puis détruit. Avec
`std::make_shared<Texture>()` et un constructeur, on l'éviterait.)

`shared_ptr` a un coût : le compteur est alloué à part (sauf avec
`make_shared`, qui le colle à l'objet) et il est modifié de façon *atomique*
pour fonctionner entre threads. **Préfère `unique_ptr` par défaut**, et ne
partage que si la propriété est vraiment partagée.

### Le piège des cycles, et `std::weak_ptr`

Si A possède un `shared_ptr` vers B et B un `shared_ptr` vers A, les deux
compteurs ne tombent jamais à 0 : **fuite**. On casse le cycle avec un
**`std::weak_ptr`**, qui observe un objet partagé sans en être propriétaire.
On en obtient un `shared_ptr` temporaire avec `lock()`, vide si l'objet a
déjà été libéré.

```cpp
struct Child;
struct Parent { std::shared_ptr<Child> child; };   // le parent possède l'enfant
struct Child  { std::weak_ptr<Parent> parent; };   // l'enfant observe le parent
```

## Passer des pointeurs aux fonctions

La règle : **une fonction qui n'a pas besoin de la propriété ne reçoit pas de
pointeur intelligent.**

```cpp
void show(const Sensor &s);                   // utilise l'objet : référence
void maybe_show(const Sensor *s);             // objet facultatif : pointeur brut (peut être nullptr)
void adopt(std::unique_ptr<Sensor> s);        // PREND la propriété : par valeur, appel avec std::move
std::unique_ptr<Sensor> make_sensor();        // DONNE la propriété : renvoyé par valeur
```

## Pièges

- **Créer deux pointeurs intelligents à partir du même pointeur brut**
  (`std::unique_ptr<T> a(p), b(p);`) : chacun le libérera, d'où une double
  libération. Utilise toujours `make_unique` / `make_shared`.
- **Garder un pointeur brut (`get()`) plus longtemps que son propriétaire** :
  *use-after-free*, comme en C.
- **Les cycles de `shared_ptr`** : fuite silencieuse. Un des liens doit être
  un `weak_ptr`.
- **Une liste chaînée de `unique_ptr` très longue** : sa destruction est
  récursive (chaque nœud détruit le suivant). Avec des centaines de milliers
  de nœuds, la pile d'appels peut déborder. Pour une vraie liste, on écrit un
  destructeur itératif.

## En entreprise

- **Pas de `new` ni de `delete` dans le code** : `std::make_unique`,
  `std::make_shared`, les conteneurs.
- **`unique_ptr` par défaut**, `shared_ptr` seulement pour une propriété
  vraiment partagée.
- **Pointeurs bruts et références uniquement pour observer**, jamais pour
  posséder.
- **Les signatures disent la propriété** : un `unique_ptr` en paramètre
  signifie « je prends » ; en retour, « je te donne ».

## Vérifie ta compréhension

1. Pourquoi `std::unique_ptr<int> b = a;` ne compile-t-il pas ?
2. Une fonction affiche un `Sensor` sans le garder. Quel type de paramètre ?
3. Que vaut `use_count()` juste après `auto a = std::make_shared<int>(1); auto b = a; auto c = b;` ?

### Réponses

1. Copier créerait deux propriétaires uniques, donc deux `delete`. La copie
   est supprimée (`= delete`) ; seul le transfert avec `std::move` est permis.
2. `const Sensor &` : elle n'a pas besoin de la propriété.
3. 3 : `a`, `b` et `c` partagent le même objet.
