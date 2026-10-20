# Makefile de trainer
#
#   make              compile ./trainer (debug : -O0 -g)
#   make test         tests unitaires + tests de bout en bout
#   make asan         mêmes tests, compilés avec AddressSanitizer/UBSan
#   make valgrind     tests unitaires + un check complet sous valgrind
#   make release      binaire optimisé dans build/release/
#   make gui          application graphique Qt (./trainer-gui), via CMake
#   make gui-test     test de l'application (parcours d'élève simulé)
#   make install      installe dans $(PREFIX) (défaut : /usr/local)
#   make install-gui  installe aussi l'application graphique et son menu
#                     ex. : make install PREFIX=$HOME/.local
#                     ($HOME plutôt que ~ : zsh ne développe pas ~ après '=')
#   make clean
#
# Chaque variante (debug, asan, release) a son propre dossier build/<variante>
# pour ne jamais mélanger des .o compilés avec des options différentes.

VERSION  := 0.4.0
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

# Organisation :
#   src/core + src/util -> libtrainer.a  (le moteur, sans aucun affichage)
#   src/cli             -> trainer       (l'interface terminal)
#   tests/unit          -> unit_tests
# L'application graphique (étape 4) se liera elle aussi à libtrainer.a.
OUT      := build/$(BUILD)
LIB_SRC  := $(wildcard src/core/*.c src/core/runners/*.c src/util/*.c)
CLI_SRC  := $(wildcard src/cli/*.c)
CLI_LIB_SRC := $(filter-out src/cli/main.c,$(CLI_SRC))
TEST_SRC := $(wildcard tests/unit/*.c)
LIB_OBJ  := $(LIB_SRC:%.c=$(OUT)/%.o)
CLI_OBJ  := $(CLI_SRC:%.c=$(OUT)/%.o)
CLI_LIB_OBJ := $(CLI_LIB_SRC:%.c=$(OUT)/%.o)
TEST_OBJ := $(TEST_SRC:%.c=$(OUT)/%.o)
ALL_OBJ  := $(LIB_OBJ) $(CLI_OBJ) $(TEST_OBJ)

LIB      := $(OUT)/libtrainer.a
BIN      := $(OUT)/trainer
TEST_BIN := $(OUT)/unit_tests

VALGRIND := valgrind --quiet --leak-check=full --error-exitcode=99 \
            --errors-for-leak-kinds=definite,indirect,possible \
            --child-silent-after-fork=yes

.PHONY: all lib test unit e2e cxx-headers asan valgrind release install uninstall clean \
        gui gui-test install-gui

all: trainer

# ./trainer à la racine : pratique pendant le développement.
trainer: $(BIN)
	cp $(BIN) $@

lib: $(LIB)

# Bibliothèque statique : une simple archive de fichiers .o (ar = archiver).
# L'éditeur de liens n'en extrait que les .o réellement utilisés.
$(LIB): $(LIB_OBJ)
	@rm -f $@
	$(AR) rcs $@ $^

# La bibliothèque vient APRÈS les .o : l'éditeur de liens ne cherche dans
# une archive que les symboles encore non résolus à ce moment-là.
$(BIN): $(CLI_OBJ) $(LIB)
	$(CC) $(ALL_LDFLAGS) -o $@ $^

$(TEST_BIN): $(TEST_OBJ) $(CLI_LIB_OBJ) $(LIB)
	$(CC) $(ALL_LDFLAGS) -o $@ $^

# Les en-têtes de src/cli ne sont visibles que de la CLI et des tests : le
# moteur ne peut pas en dépendre par erreur (il ne compilerait plus).
$(OUT)/src/cli/%.o $(OUT)/tests/%.o: EXTRA_INC := -Isrc/cli

# Règle générique : build/<variante>/src/x.o <- src/x.c
$(OUT)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(EXTRA_INC) $(ALL_CFLAGS) -c -o $@ $<

test: unit e2e cxx-headers

# Vérifie que les en-têtes publics se compilent en C++20 (pour l'application
# Qt) : extern "C", pas de mot-clé C++ comme nom (requires, class, new...).
cxx-headers:
	@command -v $(CXX) >/dev/null || { echo "$(CXX) absent : test ignoré"; exit 0; }
	$(CXX) -std=c++20 -Wall -Wextra -Werror -fsyntax-only -Iinclude tests/cxx/headers.cpp

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

# --- Application graphique : CMake s'occupe de Qt (voir gui/CMakeLists.txt).
GUI_BUILD := build/gui

gui:
	cmake -S gui -B $(GUI_BUILD) -DCMAKE_BUILD_TYPE=Debug
	cmake --build $(GUI_BUILD) -j
	cp $(GUI_BUILD)/trainer-gui trainer-gui

gui-test: gui
	ctest --test-dir $(GUI_BUILD) --output-on-failure

install-gui:
	cmake -S gui -B build/gui-release -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$(PREFIX)
	cmake --build build/gui-release -j
	DESTDIR=$(DESTDIR) cmake --install build/gui-release

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/trainer $(DESTDIR)$(PREFIX)/bin/trainer-gui
	rm -f $(DESTDIR)$(PREFIX)/share/applications/cl-trainer.desktop
	rm -rf $(DESTDIR)$(DATADIR)

clean:
	rm -rf build trainer trainer-gui

-include $(ALL_OBJ:.o=.d)
