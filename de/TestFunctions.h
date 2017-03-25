/**
 * \file TestFunctions.h
 * \author Milos Stojanovic Stojke (milsto)
 *
 * Collection of several popular test functions for optimization algorithms
 */

#pragma once

#include <vector>
#include <cassert>

#include "DifferentialEvolution.h"

namespace de
{
    class VSS : public IOptimizable
    {
    public:
        VSS(unsigned int dims = 2) :
            m_dim(dims)
        {

        }
        double EvaluteCost(std::vector<double> inputs) const override
        {
            assert(inputs.size() == m_dim);

            double val = 0.0;
            for (int i = 0; i < m_dim; i++)
            {
                val += inputs[i] * inputs[i]
                       - 100 * cos(inputs[i]) * cos(inputs[i])
                       - 100 * cos(inputs[i] * inputs[i] / 30);
            }

            return val + 1400.0;
        }

        unsigned int NumberOfParameters() const override
        {
            return m_dim;
        }

        std::vector<Constraints> GetConstraints() const override
        {
            std::vector<Constraints> constr(NumberOfParameters());
            for (auto& c : constr)
            {
                c = Constraints(-100.0, 100.0, true);
            }
            return constr;
        }

    private:
        unsigned int m_dim;
    };

    class CosineMixture : public IOptimizable
    {
    public:
        CosineMixture(unsigned int dims = 5) :
            m_dim(dims)
        {

        }
        double EvaluteCost(std::vector<double> inputs) const override
        {
            assert(inputs.size() == m_dim);

            double x = inputs[0];
            double y = inputs[1];

            double val1 = 0.0;
            double val2 = 0.0;

            for (int i = 0; i < m_dim; i++)
            {
                if (inputs[i] < -1.0 || inputs[i] > 1.0)
                {
                    return 1e7;
                }

                val1 += cos(5 * M_PI * inputs[i]);
                val2 += inputs[i] * inputs[i];
            }

            return -0.1 * val1 - val2;
        }

        unsigned int NumberOfParameters() const override
        {
            return m_dim;
        }

        std::vector<Constraints> GetConstraints() const override
        {
            std::vector<Constraints> constr(NumberOfParameters());
            for (auto& c : constr)
            {
                c = Constraints(-1.0, 1.0, true);
            }
            return constr;
        }

    private:
        unsigned int m_dim;
    };

    class Rastrigin : public IOptimizable
    {
    public:
        Rastrigin(unsigned int dims = 5) :
            m_dim(dims)
        {

        }

        double EvaluteCost(std::vector<double> inputs) const override
        {
            assert(inputs.size() == m_dim);

            double A = 10;

            double val = 0.0;

            for (int i = 0; i < m_dim; i++)
            {
                if (inputs[i] < -5.12 || inputs[i] > 5.12)
                {
                    return 1e7;
                }

                val += inputs[i] * inputs[i] - A * cos(2 * M_PI * inputs[i]);
            }

            return A * m_dim + val;
        }

        unsigned int NumberOfParameters() const override
        {
            return m_dim;
        }

        std::vector<Constraints> GetConstraints() const override
        {
            std::vector<Constraints> constr(NumberOfParameters());
            for (auto& c : constr)
            {
                c = Constraints(-5.12, 5.12, true);
            }
            return constr;
        }

    private:
        unsigned int m_dim;
    };
}
