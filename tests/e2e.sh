#!/bin/sh
# tests/e2e.sh — tests de bout en bout : lance le vrai binaire sur de vrais
# exercices et vérifie codes de retour et messages.
#
# Usage : TRAINER=build/debug/trainer sh tests/e2e.sh
# (TRAINER peut contenir un préfixe, ex. "valgrind --error-exitcode=99 ./trainer")

set -u
TRAINER=${TRAINER:-./trainer}
ROOT=$(cd "$(dirname "$0")/.." && pwd)
FIXTURES="$ROOT/tests/fixtures/exercises"
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT INT TERM
total=0
fails=0

# Chaque section travaille dans un workspace et une progression neufs.
# XDG_DATA_HOME isolé : ces tests ne touchent JAMAIS à la vraie progression.
section() {
    echo "== $1"
    WS="$TMP/$2/ws"
    export XDG_DATA_HOME="$TMP/$2/data"
    PROGRESS="$XDG_DATA_HOME/trainer/progress.txt"
}

fail() {
    echo "ÉCHEC : $1"
    fails=$((fails + 1))
}

# expect <code attendu> <motif attendu dans la sortie, ou ""> <arguments de trainer...>
expect() {
    want=$1
    pattern=$2
    shift 2
    total=$((total + 1))
    # $TRAINER sans guillemets : volontaire, pour découper "valgrind ... trainer".
    out=$($TRAINER --no-color --workspace "$WS" "$@" 2>&1)
    got=$?
    if [ "$got" -ne "$want" ]; then
        fail "trainer $* -> code $got, attendu $want"
        printf '%s\n' "$out" | head -40 | sed 's/^/    /'
    elif [ -n "$pattern" ] && ! printf '%s\n' "$out" | grep -qF -- "$pattern"; then
        fail "trainer $* : « $pattern » absent de la sortie"
        printf '%s\n' "$out" | head -40 | sed 's/^/    /'
    fi
}

# solve <id> <dossier de l'exercice> : copie la solution dans le workspace
solve() {
    cp -R "$2/solution/." "$WS/cpp/$1/"
}

HELLO="$ROOT/exercises/cpp/01-basics/hello"
SUM="$ROOT/exercises/cpp/01-basics/sum"

section "commandes générales" general
expect 0 "trainer 0" --version
expect 2 "usage"
expect 2 "commande inconnue" frobnicate
expect 0 "cpp.basics.hello" list
expect 0 "Bonjour" show cpp.basics.hello
expect 2 "exercice inconnu" show cpp.nope
expect 2 "pas encore commencé" check cpp.basics.hello
expect 2 "introuvable" --exercises /dossier/inexistant list
expect 0 "g++" doctor

section "exercices du catalogue : le starter échoue, la solution réussit" catalog
for f in $(find "$ROOT/exercises" -name exercise.txt | sort); do
    dir=$(dirname "$f")
    id=$(sed -n 's/^id:[[:space:]]*//p' "$f")
    expect 0 "" start "$id"
    expect 1 "" check "$id"
    solve "$id" "$dir"
    expect 0 "Tous les tests passent" check "$id"
done
expect 0 "déjà commencé" start cpp.basics.hello
expect 0 "recopié" start cpp.basics.hello --force
expect 1 "" check cpp.basics.hello

section "progression, indices, solution, recommandations" learning
expect 0 "○ cpp.basics.hello" list
expect 0 "– cpp.basics.sum" list               # prérequis non réussi
expect 0 "Prochain exercice" next
expect 0 "Exercice créé" start cpp.basics.hello
expect 0 "● cpp.basics.hello" list
expect 1 "" check cpp.basics.hello
expect 0 "✗ cpp.basics.hello" list
expect 0 "finis-le d'abord" next
solve cpp.basics.hello "$HELLO"
expect 0 "" check cpp.basics.hello
grep -q "^cpp.basics.hello status=passed attempts=2 build_failures=0 hints=0 solution_seen=0 first=" "$PROGRESS" ||
    { fail "contenu de progress.txt"; cat "$PROGRESS"; }
expect 0 '"id":"cpp.basics.hello","title":"Bonjour, prénom","lang":"cpp","topic":"basics","level":1,"status":"passed","unlocked":true' list --json
expect 0 "cpp.basics.sum" next
expect 0 "Indice 1/2 (nouveau)" hint cpp.basics.sum
expect 0 "Indice 2/2 (nouveau)" hint cpp.basics.sum
expect 0 "Tous les indices sont affichés" hint cpp.basics.sum
expect 1 "confirmer avec --yes" solution cpp.basics.sum </dev/null
expect 0 "Explication" solution cpp.basics.sum --yes
expect 0 "1/2 exercices réussis" stats
expect 0 "Points faibles : cpp/basics" stats    # 1 échec + (2 indices, solution) : moyenne 2
expect 0 "" start cpp.basics.sum
solve cpp.basics.sum "$SUM"
expect 0 '"status":"passed","build_log":"","passed":5,"total":5' check cpp.basics.sum --json
expect 0 "Tous les exercices sont réussis" next
expect 0 "2/2 exercices réussis" stats
grep -q "^cpp.basics.sum status=passed attempts=1 build_failures=0 hints=2 solution_seen=1" "$PROGRESS" ||
    { fail "progression de cpp.basics.sum"; cat "$PROGRESS"; }

# Progression corrompue : consultation possible, écriture refusée (sans rien écraser)
printf 'cassé status=???\n' >> "$PROGRESS"
expect 0 "progression illisible" list
expect 0 "progression non enregistrée" check cpp.basics.hello
grep -q "^cassé" "$PROGRESS" || fail "le fichier corrompu a été écrasé"

section "validate" validate
expect 0 "0 rejeté" validate
expect 0 "ok" validate cpp.basics.sum
expect 2 "introuvable" validate cpp.nope
expect 1 "réussit déjà tous les tests" --exercises "$FIXTURES" validate cpp.fixture.ok
expect 1 "la solution échoue au test" --exercises "$FIXTURES" validate cpp.fixture.wrong-output
expect 1 "cycle de prérequis" --exercises "$ROOT/tests/fixtures/broken" validate

section "verdicts du moteur (fixtures)" verdicts
# nom : code attendu : motif attendu
while IFS=: read -r name code pattern; do
    id="cpp.fixture.$name"
    expect 0 "" --exercises "$FIXTURES" start "$id"
    solve "$id" "$FIXTURES/cpp/$name"
    expect "$code" "$pattern" --exercises "$FIXTURES" check "$id"
done <<'EOF'
ok:0:Tous les tests passent
wrong-output:1:obtenu  : «41»
compile-error:1:Compilation (C++)... échec
timeout:1:temps dépassé
crash:1:erreur de segmentation
exit-code:1:Code de retour 3, attendu 0
infinite-output:1:sortie trop longue
warning:0:avec avertissements
leak:1:erreur mémoire ou comportement indéfini
ubsan:1:runtime error
EOF

echo "$total vérifications, $fails échec(s)"
[ "$fails" -eq 0 ]
