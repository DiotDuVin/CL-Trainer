# CMakeLists.txt — application graphique CL-Trainer (Qt 6, C++17).
#
# CMake ne compile rien lui-même : il GÉNÈRE un système de construction
# (Makefile ou Ninja) adapté à la machine, en trouvant Qt, le compilateur...
#
#   cmake -S gui -B build/gui -DCMAKE_BUILD_TYPE=Debug   # configurer (une fois)
#   cmake --build build/gui -j                           # compiler
#   ctest --test-dir build/gui --output-on-failure       # tests
#
# (ou simplement « make gui » / « make gui-test » depuis la racine)

cmake_minimum_required(VERSION 3.16)
project(cl-trainer-gui VERSION 0.4.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)          # -std=c++17 et pas -std=gnu++17
# compile_commands.json : options exactes de chaque fichier, lues par clangd
# (voir gui/.clangd) pour que VS Code trouve les en-têtes Qt.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# AUTOMOC : Qt ajoute au C++ des « signaux » et « slots ». L'outil moc lit les
# classes marquées Q_OBJECT et génère le code correspondant ; CMake le lance
# automatiquement.
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Widgets Concurrent Test)

set(ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)
include(GNUInstallDirs)

# --- libtrainer : le moteur C, compilé à partir des mêmes sources que le
# Makefile. CONFIGURE_DEPENDS : CMake revérifie la liste à chaque build.
file(GLOB TRAINER_SOURCES CONFIGURE_DEPENDS
     ${ROOT}/src/core/*.c ${ROOT}/src/core/runners/*.c ${ROOT}/src/util/*.c)
add_library(trainer STATIC ${TRAINER_SOURCES})
set_target_properties(trainer PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF)
# PUBLIC : tout ce qui se lie à « trainer » voit aussi include/.
target_include_directories(trainer PUBLIC ${ROOT}/include)
target_compile_definitions(trainer PRIVATE
    _XOPEN_SOURCE=700
    TRAINER_DATADIR="${CMAKE_INSTALL_FULL_DATADIR}/trainer")

# Mêmes exigences qu'avec le Makefile : aucun avertissement toléré en CI.
set(WARNINGS -Wall -Wextra -Wpedantic -Wshadow)
target_compile_options(trainer PRIVATE ${WARNINGS})

# --- Le code de l'interface, dans une bibliothèque pour être partagé entre
# l'exécutable et les tests.
add_library(trainer-ui STATIC
    src/engine.cpp src/engine.h
    src/catalogview.cpp src/catalogview.h
    src/exerciseview.cpp src/exerciseview.h
    src/lessonview.cpp src/lessonview.h
    src/mainwindow.cpp src/mainwindow.h
    src/format.cpp src/format.h)
target_include_directories(trainer-ui PUBLIC src)
target_link_libraries(trainer-ui PUBLIC trainer Qt6::Widgets Qt6::Concurrent)
target_compile_options(trainer-ui PRIVATE ${WARNINGS})
target_compile_definitions(trainer-ui PUBLIC TRAINER_VERSION="${PROJECT_VERSION}")

add_executable(trainer-gui src/main.cpp)
target_link_libraries(trainer-gui PRIVATE trainer-ui)
target_compile_options(trainer-gui PRIVATE ${WARNINGS})

# --- Tests : un vrai parcours d'élève piloté par QTest, sans écran
# (plateforme « offscreen »).
enable_testing()
add_executable(test-gui tests/test_gui.cpp)
target_link_libraries(test-gui PRIVATE trainer-ui Qt6::Test)
add_test(NAME gui COMMAND test-gui)
set_tests_properties(gui PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;TRAINER_ROOT=${ROOT}")

# --- Installation : binaire + entrée dans le menu des applications.
install(TARGETS trainer-gui RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES cl-trainer.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
