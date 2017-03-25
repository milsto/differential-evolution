/**
 * \file example_1.cpp
 * \author Milos Stojanovic Stojke (milsto)
 *
 * Simplest example for optimizing Rastring's test function.
 */

#include "de/DifferentialEvolution.h"
#include "de/TestFunctions.h"


int main()
{
    // Creat Rastring's function in 5 dimensions
    de::Rastrigin cost(5);

    // Create Differential Evolution optimizer with population size of 50
    de::DifferentialEvolution de(cost, 50);

    // Optimize for 200 iterations
    de.Optimize(1000, true);

    return 0;
}
