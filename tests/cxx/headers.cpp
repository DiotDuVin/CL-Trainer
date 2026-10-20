// headers.cpp — vérifie que libtrainer est utilisable depuis le C++
// (compilé avec -fsyntax-only : on ne produit aucun binaire).
#include "trainer.h"

int main()
{
    trainer_config cfg{};
    catalog cat{};
    progress prog{};
    check_report rep{};
    recommendation reco{};
    (void)cfg; (void)cat; (void)prog; (void)rep; (void)reco;
    return 0;
}
