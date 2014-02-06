/**
 * @file primalpricingupdater.cpp
 */

#include <simplex/primalpricingupdater.h>

PrimalPricingUpdater::PrimalPricingUpdater(const Vector &basicVariableValues,
                                          const IndexList<> & basicVariableFeasibilities,
                                          IndexList<> * reducedCostFeasibilities,
                                          const std::vector<int> & basisHead,
                                          const SimplexModel &simplexModel,
                                          const Basis & basis):
    m_basicVariableValues(basicVariableValues),
    m_basicVariableFeasibilities(basicVariableFeasibilities),
    m_reducedCostFeasibilities(reducedCostFeasibilities),
    m_basisHead(basisHead),
    m_simplexModel(simplexModel),
    m_basis(basis)
{

}

PrimalPricingUpdater::~PrimalPricingUpdater() {

}