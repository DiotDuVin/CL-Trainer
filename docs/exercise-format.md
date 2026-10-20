# Format des exercices

Un exercice est un **dossier** placé n'importe où sous `exercises/`. Le préfixe
numérique des dossiers fixe l'ordre d'affichage (`exercises/cpp/01-basics/hello/`).

```
hello/
├── exercise.txt      métadonnées + énoncé + correction
├── tests.txt         cas de test
├── starter/          code de départ copié chez l'élève (doit compiler, doit échouer aux tests)
│   └── main.cpp
└── solution/         solution de référence (doit réussir tous les tests)
    └── main.cpp
```

## exercise.txt

Le fichier commence par un en-tête `clé: valeur`, suivi de sections `## nom`.

```
id: cpp.basics.hello
title: Bonjour, prénom
lang: cpp
level: 1
topic: basics
tags: io, string
requires: cpp.basics.intro
files: main.cpp
timeout_ms: 2000
estimated_min: 5

## statement
Énoncé...

## hints
Premier indice.
---
Second indice.

## explanation
Correction détaillée, affichée quand tous les tests passent.

## common_mistakes
- Erreur typique 1...
```

| Clé             | Oblig. | Valeurs                                                        |
|-----------------|:------:|----------------------------------------------------------------|
| `id`            | oui    | `<lang>.<thème>.<nom>` en `[a-z0-9._-]`. **Ne jamais modifier** une fois publié : la progression des utilisateurs en dépend. |
| `title`         | oui    | texte                                                          |
| `lang`          | oui    | `cpp` (puis `git`, `csharp`, `rust`)                           |
| `level`         | oui    | 1 (débutant) à 5 (professionnel)                               |
| `topic`         | oui    | thème, utilisé pour les statistiques de points faibles         |
| `tags`          | non    | liste séparée par des virgules                                 |
| `requires`      | non    | ids des prérequis, séparés par des virgules                    |
| `files`         | non    | fichiers modifiés par l'élève (défaut C++ : `main.cpp`). Chemins relatifs, sans `..` |
| `kind`          | non    | `exercise` (défaut) ou `project`                               |
| `timeout_ms`    | non    | délai par test, 2000 par défaut (max 120000)                   |
| `estimated_min` | non    | durée indicative                                               |
| `compare`       | non    | `normalized` (défaut) ou `exact`                               |
| `sanitize`      | non    | `yes` : compile avec AddressSanitizer + UBSan (fuites, débordements, comportements indéfinis) |

Sections : `statement` et `explanation` sont **obligatoires**. `hints` et
`common_mistakes` sont recommandées. Dans une section, `### titre` reste du texte
ordinaire. Seul `## ` en début de ligne ouvre une nouvelle section.

Les clés inconnues, les doublons et les valeurs invalides sont **refusés**, avec le
numéro de ligne. C'est volontaire : une faute de frappe (`tittle:`) ne doit pas passer
inaperçue.

## tests.txt

```
=== test: exemple de l'énoncé
--- stdin
3
1 2 3
--- stdout
6

=== test: grandes valeurs [hidden]
--- args
--verbose
--- stdin
...
--- stdout
...
--- exit
0
```

- `=== test: nom` ouvre un test. Avec `[hidden]`, l'entrée et la sortie attendue ne
  sont pas montrées en cas d'échec. On les utilise pour les cas limites, pour éviter un
  code écrit « pour passer les tests ».
- Blocs : `--- stdin`, `--- stdout` (**obligatoire**, peut être vide), `--- args` (un
  argument par ligne, espaces autorisés), `--- exit` (code attendu, 0 par défaut).
- Dans un bloc, chaque ligne se termine par `\n`. Les **lignes vides finales sont
  retirées** : elles servent juste à aérer le fichier.

### Comparaison des sorties

- `normalized` (défaut) : on ignore `\r\n` contre `\n`, les espaces et tabulations en
  fin de ligne, et les lignes vides finales.
- `exact` : comparaison octet par octet. La sortie attendue se termine toujours par
  `\n` (sauf si elle est vide).

## Limites connues du format

- Une ligne de sortie attendue qui commence par `=== ` ou `--- ` serait prise pour un
  marqueur. Si un exercice en a besoin, il faudra un mécanisme d'échappement.
- En mode `exact`, on ne peut pas exiger de lignes vides en fin de sortie.

## Règles de qualité (vérifiées par `trainer validate` et la CI)

1. La solution de référence réussit tous les tests.
2. Le starter compile mais **échoue** au moins à un test.
3. Chaque exercice a au moins un test caché pour un cas limite.
4. Le catalogue n'a ni id en double, ni prérequis inconnu, ni cycle de prérequis.
5. (Relecture humaine) L'explication montre la solution, explique le **pourquoi**, et
   cite les bonnes pratiques professionnelles.
