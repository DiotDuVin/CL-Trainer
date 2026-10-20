# trainer

Outil d'entraînement en ligne de commande pour apprendre **C++**, **Git**, **C#** et
**Rust** par la pratique. Il donne des exercices courts et des mini-projets, avec une
correction automatique et des explications.

Le moteur est écrit en C11 (POSIX). Il compile et exécute votre code avec les vrais
outils (`g++`, `git`, `dotnet`, `cargo`), puis compare la sortie aux tests.

> État : **étape 3**. Moteur C++ complet avec suivi de progression, indices,
> recommandations et validation des exercices. À venir : application graphique (Qt),
> une trentaine d'exercices C++, puis Git, C# et Rust.

## Installation

Plateformes : Linux, macOS, Windows via WSL.

```sh
# Arch Linux
sudo pacman -S --needed base-devel gcc git valgrind gdb clang
# Debian / Ubuntu
sudo apt install build-essential g++ git valgrind gdb clangd

git clone <url-du-dépôt> trainer && cd trainer
make                 # produit ./trainer
make test            # vérifie que tout fonctionne sur votre machine
```

Installation globale (facultative) :

```sh
make install PREFIX=$HOME/.local    # ~/.local/bin doit être dans votre PATH
```

## Utilisation

```sh
trainer next                       # que faire maintenant ?
trainer list                       # catalogue et état de chaque exercice
trainer show cpp.basics.hello      # énoncé
trainer start cpp.basics.hello     # copie le code de départ dans ~/trainer-workspace
trainer start cpp.basics.hello --open   # ... et l'ouvre dans VS Code
trainer check cpp.basics.hello     # compile, teste, explique
trainer hint cpp.basics.hello      # indice suivant
trainer solution cpp.basics.hello  # solution commentée (notée dans la progression)
trainer stats                      # progression et points faibles par thème
trainer doctor                     # outils installés et dossiers utilisés
```

La progression est enregistrée dans `~/.local/share/trainer/progress.txt` (ou
`$XDG_DATA_HOME/trainer/`). C'est un fichier texte lisible : une ligne par exercice.

Votre code se trouve dans `~/trainer-workspace/<langage>/<id>/`. Vous pouvez changer
cet emplacement avec `--workspace DIR` ou la variable `TRAINER_WORKSPACE`. Astuce :
faites de ce dossier un dépôt Git et commitez vos solutions, c'est un bon entraînement.

Codes de sortie : `0` réussite, `1` échec de l'exercice, `2` erreur d'utilisation.

### Avec VS Code

`trainer start` génère dans le dossier de l'exercice :

- `.vscode/tasks.json` : **Ctrl+Maj+B** lance `trainer check`, et les erreurs du
  compilateur deviennent cliquables (onglet « Problèmes ») ;
- `compile_flags.txt` : l'extension **clangd** utilise alors les mêmes options que le
  correcteur.

Ouvrez le **dossier de l'exercice** (pas tout le workspace), par exemple avec
`trainer start <id> --open`.

## Développement

Le moteur est une bibliothèque (`libtrainer.a`, dossiers `src/core` et `src/util`, en-tête
`include/trainer.h`) sans aucun affichage. Le programme `trainer` (`src/cli`) n'en est qu'une
interface ; l'application graphique en sera une autre.

```sh
make test        # tests unitaires + bout en bout + en-têtes compilables en C++20
./trainer validate   # chaque solution de référence passe ses tests, chaque starter échoue
make asan        # mêmes tests, moteur compilé avec AddressSanitizer + UBSan
make valgrind    # tests sous valgrind (aucune fuite tolérée)
make CC=clang    # vérifier avec un second compilateur
```

La CI GitHub (`.github/workflows/ci.yml`) lance tout cela à chaque push et pull request,
avec gcc et clang.

- Architecture et signatures : [docs/architecture.md](docs/architecture.md)
- Écrire un exercice : [docs/exercise-format.md](docs/exercise-format.md)
