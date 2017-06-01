/**
 * \file DifferentialEvolution.h
 * \author Milos Stojanovic Stojke (milsto)
 *
 * Implementation of Differential evolution algorithm.
 */

#pragma once

#include <iostream>
#include <vector>
#include <cassert>
#include <random>
#include <iomanip>
#include <utility>
#include <memory>
#include <limits>

namespace de
{
    class IOptimizable
    {
    public:
        struct Constraints
        {
            Constraints(double lower = 0.0, double upper = 1.0, bool isConstrained = false) :
                lower(lower),
                upper(upper),
                isConstrained(isConstrained)
            {

            }

            bool Check(double candidate)
            {
                if (isConstrained)
                {
                    if (candidate <= upper && candidate >= lower)
                    {
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }
                else
                {
                    return true;
                }
            }

            double lower;
            double upper;
            bool isConstrained;
        };

        virtual double EvaluteCost(std::vector<double> inputs) const = 0;
        virtual unsigned int NumberOfParameters() const = 0;
        virtual std::vector<Constraints> GetConstraints() const = 0;
        virtual ~IOptimizable() {}
    };

    class DifferentialEvolution
    {
    public:
        /**
         * Construct Differential Evolution optimizer
         *
         * \param costFunction Cost function to minimize
         * \param populationSize Number of agents in each optimization step
         * \param randomSeed Set random seed to a fix value to have repeatable (non stochastic) experiments
         * \param shouldCheckConstraints Should constraints bee checked on for each new candidate.
         * This check check may be turned off to increase performance if the cost function is defined
         * and has no local minimum outside of the constraints.
         * \param callback Optional callback to be called after each optimization iteration has finished.
         * Optimization iteration is defined as processing of single population with SelectionAndCorssing method.
         */
        DifferentialEvolution(  const IOptimizable& costFunction,
                                unsigned int populationSize,
                                int randomSeed = 123,
                                bool shouldCheckConstraints = true,
                                std::function<void(const DifferentialEvolution&)> callback = nullptr,
                                std::function<bool(const DifferentialEvolution&)> terminationCondition = nullptr) :
            m_cost(costFunction),
            m_populationSize(populationSize),
            m_F(0.8),
            m_CR(0.9),
            m_bestAgentIndex(0),
            m_minCost(-std::numeric_limits<double>::infinity()),
            m_shouldCheckConstraints(shouldCheckConstraints),
            m_callback(callback),
            m_terminationCondition(terminationCondition)
        {
            m_generator.seed(randomSeed);
            assert(m_populationSize >= 4);

            m_numberOfParameters = m_cost.NumberOfParameters();

            m_population.resize(populationSize);
            for (auto& agent : m_population)
            {
                agent.resize(m_numberOfParameters);
            }

            m_minCostPerAgent.resize(m_populationSize);

            m_constraints = costFunction.GetConstraints();
        }

        void InitPopulation()
        {
            // Init population based on random sampling of the cost function
            std::shared_ptr<std::uniform_real_distribution<double>> distribution;

            for (auto& agent : m_population)
            {
                for (int i = 0; i < m_numberOfParameters; i++)
                {
                    if (m_constraints[i].isConstrained)
                    {
                        distribution = std::make_shared<std::uniform_real_distribution<double>>(std::uniform_real_distribution<double>(m_constraints[i].lower, m_constraints[i].upper));
                    }
                    else
                    {
                        distribution = std::make_shared<std::uniform_real_distribution<double>>(std::uniform_real_distribution<double>(g_defaultLowerConstraint, g_defaultUpperConstarint));
                    }

                    agent[i] = (*distribution)(m_generator);
                }
            }

            // Initialize minimum cost, best agent and best agent index
            for (int i = 0; i < m_populationSize; i++)
            {
                m_minCostPerAgent[i] = m_cost.EvaluteCost(m_population[i]);

                if (m_minCostPerAgent[i] < m_minCost)
                {
                    m_minCost = m_minCostPerAgent[i];
                    m_bestAgentIndex = i;
                }
            }
        }

        void SelectionAndCorssing()
        {
            std::uniform_real_distribution<double> distribution(0, m_populationSize);

            double minCost = m_minCostPerAgent[0];
            int bestAgentIndex = 0;

            for (int x = 0; x < m_populationSize; x++)
            {
                // For x in population select 3 random agents (a, b, c) different from x
                int a = x;
                int b = x;
                int c = x;

                // Agents must be different from each other and from x
                while (a == x || b == x || c == x || a == b || a == c || b == c)
                {
                    a = distribution(m_generator);
                    b = distribution(m_generator);
                    c = distribution(m_generator);
                }

                // Form intermediate solution z
                std::vector<double> z(m_numberOfParameters);
                for (int i = 0; i < m_numberOfParameters; i ++)
                {
                    z[i] = m_population[a][i] + m_F * (m_population[b][i] - m_population[c][i]);
                }

                // Chose random R
                std::uniform_real_distribution<double> distributionParam(0, m_numberOfParameters);
                int R = distributionParam(m_generator);

                // Chose random r for each dimension
                std::vector<double> r(m_numberOfParameters);
                std::uniform_real_distribution<double> distributionPerX(0, 1);
                for (auto& var : r)
                {
                    var = distributionPerX(m_generator);
                }

                std::vector<double> newX(m_numberOfParameters);

                // Execute crossing
                for (int i = 0; i < m_numberOfParameters; i++)
                {
                    if (r[i] < m_CR || i == R)
                    {
                        newX[i] = z[i];
                    }
                    else
                    {
                        newX[i] = m_population[x][i];
                    }
                }

                // Check if newX candidate satisfies constraints and skip it if not.
                // If agent is skipped loop iteration x is decreased so that it is ensured
                // that the population has constant size (equal to m_populationSize).
                if (m_shouldCheckConstraints && !CheckConstraints(newX))
                {
                    x--;
                    continue;
                }

                // Calculate new cost and decide should the newX be kept.
                double newCost = m_cost.EvaluteCost(newX);
                if (newCost < m_minCostPerAgent[x])
                {
                    m_population[x] = newX;
                    m_minCostPerAgent[x] = newCost;
                }

                // Track the global best agent.
                if (m_minCostPerAgent[x] < minCost)
                {
                    minCost = m_minCostPerAgent[x];
                    bestAgentIndex = x;
                }
            }

            m_minCost = minCost;
            m_bestAgentIndex = bestAgentIndex;
        }

        std::vector<double> GetBestAgent() const
        {
            return m_population[m_bestAgentIndex];
        }

        double GetBestCost() const
        {
            return m_minCostPerAgent[m_bestAgentIndex];
        }

        std::vector<std::pair<std::vector<double>, double>> GetPopulationWithCosts() const
        {
            std::vector<std::pair<std::vector<double>, double>> toRet;
            for (int i = 0; i < m_populationSize; i++)
            {
                toRet.push_back(std::make_pair(m_population[i], m_minCostPerAgent[i]));
            }

            return toRet;
        }

        void PrintPopulation() const
        {
            for (auto agent : m_population)
            {
                for (auto& var : agent)
                {
                    std::cout << var << " ";
                }
                std::cout << std::endl;
            }
        }

        void Optimize(int iterations, bool verbose = true)
        {
            InitPopulation();

            // Optimization loop
            for (int i = 0; i < iterations; i++)
            {
                // Optimization step
                SelectionAndCorssing();

                if (verbose)
                {
                    std::cout << std::fixed << std::setprecision(5);
                    std::cout << "Current minimal cost: " << m_minCost << "\t\t";
                    std::cout << "Best agent: ";
                    for (int i = 0; i < m_numberOfParameters; i++)
                    {
                        std::cout<< m_population[m_bestAgentIndex][i] << " ";
                    }
                    std::cout << std::endl;
                }

                if (m_callback)
                {
                    m_callback(*this);
                }

                if (m_terminationCondition)
                {
                    if (m_terminationCondition(*this))
                    {
                        if (verbose)
                        {
                            std::cout << "Terminated due to positive evaluation of the termination condition." << std::endl;
                        }
                        return;
                    }
                }
            }

            if (verbose)
            {
                std::cout << "Terminated due to exceeding total number of generations." << std::endl;
            }
        }

    private:
        bool CheckConstraints(std::vector<double> agent)
        {
            for (int i = 0; i < agent.size(); i++)
            {
                if (!m_constraints[i].Check(agent[i]))
                {
                    return false;
                }
            }

            return true;
        }

        const IOptimizable& m_cost;
        unsigned int m_populationSize;
        double m_F;
        double m_CR;

        unsigned int m_numberOfParameters;

        bool m_shouldCheckConstraints;

        std::function<void(const DifferentialEvolution&)> m_callback;
        std::function<bool(const DifferentialEvolution&)> m_terminationCondition;

        std::default_random_engine m_generator;
        std::vector<std::vector<double>> m_population;

        std::vector<double> m_minCostPerAgent;

        std::vector<IOptimizable::Constraints> m_constraints;

        int m_bestAgentIndex;
        double m_minCost;

        static constexpr double g_defaultLowerConstraint = -std::numeric_limits<double>::infinity();
        static constexpr double g_defaultUpperConstarint = std::numeric_limits<double>::infinity();
    };
}
