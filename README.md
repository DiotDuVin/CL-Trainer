# trainer

Outil d'entraînement en ligne de commande pour apprendre **C++**, **Git**, **C#** et
**Rust** par la pratique. Il donne des exercices courts et des mini-projets, avec une
correction automatique et des explications.

Le moteur est écrit en C11 (POSIX). Il compile et exécute votre code avec les vrais
outils (`g++`, `git`, `dotnet`, `cargo`), puis compare la sortie aux tests.

> État : **étape 2**. Moteur fonctionnel pour C++ (`list`, `show`, `start`, `check`).
> À venir : suivi de progression, indices, validation, répétition espacée, Git, C#, Rust.

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
trainer list                       # catalogue des exercices
trainer show cpp.basics.hello      # énoncé
trainer start cpp.basics.hello     # copie le code de départ dans ~/trainer-workspace
trainer start cpp.basics.hello --open   # ... et l'ouvre dans VS Code
trainer check cpp.basics.hello     # compile, teste, explique
```

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

```sh
make test        # tests unitaires (tests/unit) + tests de bout en bout (tests/e2e.sh)
make asan        # mêmes tests, moteur compilé avec AddressSanitizer + UBSan
make valgrind    # tests sous valgrind (aucune fuite tolérée)
make CC=clang    # vérifier avec un second compilateur
```

La CI GitHub (`.github/workflows/ci.yml`) lance tout cela à chaque push et pull request,
avec gcc et clang.

- Architecture et signatures : [docs/architecture.md](docs/architecture.md)
- Écrire un exercice : [docs/exercise-format.md](docs/exercise-format.md)
