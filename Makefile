# Makefile de trainer
#
#   make              compile ./trainer (debug : -O0 -g)
#   make test         tests unitaires + tests de bout en bout
#   make asan         mêmes tests, compilés avec AddressSanitizer/UBSan
#   make valgrind     tests unitaires + un check complet sous valgrind
#   make release      binaire optimisé dans build/release/
#   make install      installe dans $(PREFIX) (défaut : /usr/local)
#                     ex. : make install PREFIX=$HOME/.local
#                     ($HOME plutôt que ~ : zsh ne développe pas ~ après '=')
#   make clean
#
# Chaque variante (debug, asan, release) a son propre dossier build/<variante>
# pour ne jamais mélanger des .o compilés avec des options différentes.

VERSION  := 0.2.0
PREFIX   ?= /usr/local
DATADIR  ?= $(PREFIX)/share/trainer
BUILD    ?= debug

CC       ?= cc
CSTD     := -std=c11
WARNINGS := -Wall -Wextra -Wpedantic -Wshadow -Wstrict-prototypes \
            -Wmissing-prototypes -Wformat=2 -Wvla -Wundef
# _XOPEN_SOURCE=700 : active POSIX 2008 + XSI (fork, poll, setrlimit,
# realpath...) tout en restant en C11 strict (pas d'extensions GNU).
# -MMD -MP : génère les dépendances .d (recompile si un .h change).
CPPFLAGS += -Iinclude -D_XOPEN_SOURCE=700 -MMD -MP \
            -DTRAINER_VERSION='"$(VERSION)"' -DTRAINER_DATADIR='"$(DATADIR)"'

ifeq ($(BUILD),release)
  OPTFLAGS := -O2
else ifeq ($(BUILD),asan)
  # Les sanitizers ajoutent des branches internes qui provoquent de faux
  # avertissements de GCC (« null format string ») : on les coupe ici seulement.
  OPTFLAGS := -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
              -Wno-format-truncation
  SANLDFLAGS := -fsanitize=address,undefined
else
  OPTFLAGS := -O0 -g
endif
# CFLAGS / LDFLAGS restent libres pour l'utilisateur (make CFLAGS=-Werror) :
# un « make VAR=... » en ligne de commande écrase toute affectation du
# Makefile, y compris +=. On construit donc nos propres variables ALL_*.
ALL_CFLAGS  = $(CSTD) $(WARNINGS) $(OPTFLAGS) $(CFLAGS)
ALL_LDFLAGS = $(SANLDFLAGS) $(LDFLAGS)

OUT      := build/$(BUILD)
SRC      := $(wildcard src/*.c src/util/*.c src/runners/*.c)
LIB_SRC  := $(filter-out src/main.c,$(SRC))
TEST_SRC := $(wildcard tests/unit/*.c)
OBJ      := $(SRC:%.c=$(OUT)/%.o)
LIB_OBJ  := $(LIB_SRC:%.c=$(OUT)/%.o)
TEST_OBJ := $(TEST_SRC:%.c=$(OUT)/%.o)

BIN      := $(OUT)/trainer
TEST_BIN := $(OUT)/unit_tests

VALGRIND := valgrind --quiet --leak-check=full --error-exitcode=99 \
            --errors-for-leak-kinds=definite,indirect,possible \
            --child-silent-after-fork=yes

.PHONY: all test unit e2e asan valgrind release install uninstall clean

all: trainer

# ./trainer à la racine : pratique pendant le développement.
trainer: $(BIN)
	cp $(BIN) $@

$(BIN): $(OBJ)
	$(CC) $(ALL_LDFLAGS) -o $@ $^

$(TEST_BIN): $(LIB_OBJ) $(TEST_OBJ)
	$(CC) $(ALL_LDFLAGS) -o $@ $^

# Règle générique : build/<variante>/src/x.o <- src/x.c
$(OUT)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(ALL_CFLAGS) -c -o $@ $<

test: unit e2e

unit: $(TEST_BIN)
	./$(TEST_BIN)

e2e: $(BIN)
	TRAINER=$(BIN) sh tests/e2e.sh

asan:
	$(MAKE) BUILD=asan unit e2e

valgrind: $(TEST_BIN) $(BIN)
	$(VALGRIND) ./$(TEST_BIN)
	TRAINER="$(VALGRIND) $(BIN)" sh tests/e2e.sh

release:
	$(MAKE) BUILD=release build/release/trainer

install: release
	install -Dm755 build/release/trainer $(DESTDIR)$(PREFIX)/bin/trainer
	rm -rf $(DESTDIR)$(DATADIR)/exercises
	mkdir -p $(DESTDIR)$(DATADIR)
	cp -R exercises $(DESTDIR)$(DATADIR)/

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/trainer
	rm -rf $(DESTDIR)$(DATADIR)

clean:
	rm -rf build trainer

-include $(OBJ:.o=.d) $(TEST_OBJ:.o=.d)
