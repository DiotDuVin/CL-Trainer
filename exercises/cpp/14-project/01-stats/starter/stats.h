// stats.h — interface du module (ne pas modifier)
#pragma once

#include <vector>

namespace stats {

// Moyenne ; 0 si la liste est vide.
double mean(const std::vector<double> &values);

// Médiane ; 0 si la liste est vide. Reçue par valeur : la fonction trie sa copie.
double median(std::vector<double> values);

// Écart-type (population) ; 0 si la liste est vide.
double stddev(const std::vector<double> &values);

}  // namespace stats
