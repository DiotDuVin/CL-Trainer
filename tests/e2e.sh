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
WS=$(mktemp -d)
trap 'rm -rf "$WS"' EXIT INT TERM
total=0
fails=0

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
        echo "ÉCHEC : trainer $* -> code $got, attendu $want"
        printf '%s\n' "$out" | head -40 | sed 's/^/    /'
        fails=$((fails + 1))
    elif [ -n "$pattern" ] && ! printf '%s\n' "$out" | grep -q -- "$pattern"; then
        echo "ÉCHEC : trainer $* : « $pattern » absent de la sortie"
        printf '%s\n' "$out" | head -40 | sed 's/^/    /'
        fails=$((fails + 1))
    fi
}

# solve <dossier d'exercices> <id> <dossier de l'exercice> : copie la solution
solve() {
    cp -R "$3/solution/." "$WS/cpp/$2/"
}

echo "== commandes générales"
expect 0 "trainer 0" --version
expect 2 "usage"
expect 2 "commande inconnue" frobnicate
expect 0 "cpp.basics.hello" list
expect 0 "Bonjour" show cpp.basics.hello
expect 2 "exercice inconnu" show cpp.nope
expect 2 "pas encore commencé" check cpp.basics.hello
expect 2 "introuvable" --exercises /dossier/inexistant list

echo "== exercices du catalogue : le starter échoue, la solution réussit"
for f in $(find "$ROOT/exercises" -name exercise.txt | sort); do
    dir=$(dirname "$f")
    id=$(sed -n 's/^id:[[:space:]]*//p' "$f")
    expect 0 "" start "$id"
    expect 1 "" check "$id"
    solve "$ROOT/exercises" "$id" "$dir"
    expect 0 "Tous les tests passent" check "$id"
done

echo "== start : conservation et --force"
expect 0 "déjà commencé" start cpp.basics.hello
expect 0 "recopié" start cpp.basics.hello --force
expect 1 "" check cpp.basics.hello

echo "== verdicts du moteur (fixtures)"
# nom : code attendu : motif attendu
while IFS=: read -r name code pattern; do
    id="cpp.fixture.$name"
    expect 0 "" --exercises "$FIXTURES" start "$id"
    solve "$FIXTURES" "$id" "$FIXTURES/cpp/$name"
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
