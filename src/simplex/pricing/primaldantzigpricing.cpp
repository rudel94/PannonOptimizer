/**
 * @file   primaldantzigpricing.cpp
 */

#include <simplex/pricing/primaldantzigpricing.h>
#include <simplex/simplexparameterhandler.h>
#include <simplex/simplex.h>

static const Numerical::Double feasibilityTolerance =
        SimplexParameterHandler::getInstance().getDoubleParameterValue("e_feasibility");

PrimalDantzigPricing::PrimalDantzigPricing(const PrimalDantzigPricing& orig):
    PrimalPricing(orig),
    m_updater(orig.m_updater),
    m_basisHead(orig.m_basisHead)
{
    copy(orig);
}

PrimalDantzigPricing::PrimalDantzigPricing(const SimplexModel & model,
                                           PrimalPricingUpdater * updater,
                                           const std::vector<int> & basisHead):
    PrimalPricing(model),
    m_updater(dynamic_cast<PrimalDantzigPricingUpdater *>(updater)),
    m_basisHead(basisHead)
{
    m_negativeSums.resize( model.getColumnCount() + model.getRowCount(), 0.0 );
    m_positiveSums.resize( model.getColumnCount() + model.getRowCount(), 0.0 );
    m_phase1ReducedCosts.reserve( model.getColumnCount() + model.getRowCount() );
    m_reducedCost = 0.0;
}
PrimalDantzigPricing::~PrimalDantzigPricing()
{
    release();
}

void PrimalDantzigPricing::copy(const PrimalDantzigPricing &orig)
{
    __UNUSED(orig);
}

void PrimalDantzigPricing::release()
{

}

void PrimalDantzigPricing::initPhase1() {
    unsigned int rowCount = m_basisHead.size();
    // get the h vector
    Vector auxVector( rowCount, Vector::DENSE_VECTOR );
    const IndexList<> & basicVariableFeasibilities = m_updater->m_basicVariableFeasibilities;
    IndexList<>::Iterator iter, iterEnd;
    basicVariableFeasibilities.getIterators(&iter, &iterEnd, Simplex::MINUS);
    for (; iter != iterEnd; iter++) {
        auxVector.setNewNonzero(iter.getData(), 1.0);
    }

    basicVariableFeasibilities.getIterators(&iter, &iterEnd, Simplex::PLUS);
    for (; iter != iterEnd; iter++) {
        auxVector.setNewNonzero(iter.getData(), -1.0);
    }
    // compute the simplex multiplier
    m_updater->m_basis.Btran(auxVector);

    // compute the reduced costs
    std::vector<Numerical::Double>::iterator sumIter = m_negativeSums.begin();
    std::vector<Numerical::Double>::iterator sumIterEnd = m_negativeSums.end();
    for (; sumIter != sumIterEnd; sumIter++) {
        *sumIter = 0.0;
    }
    sumIter = m_positiveSums.begin();
    sumIterEnd = m_positiveSums.end();
    for (; sumIter != sumIterEnd; sumIter++) {
        *sumIter = 0.0;
    }
    const Matrix & matrix = m_updater->m_simplexModel.getMatrix();
    Vector::NonzeroIterator auxIter = auxVector.beginNonzero();
    Vector::NonzeroIterator auxIterEnd = auxVector.endNonzero();
    for (; auxIter != auxIterEnd; auxIter++) {
        const unsigned int rowIndex = auxIter.getIndex();
        const Numerical::Double lambda = *auxIter;
        Vector::NonzeroIterator nonzIter = matrix.row(rowIndex).beginNonzero();
        Vector::NonzeroIterator nonzIterEnd = matrix.row(rowIndex).endNonzero();
        for (; nonzIter != nonzIterEnd; nonzIter++) {
            Numerical::Double product = lambda * *nonzIter;
            if (product > 0.0) {
                m_positiveSums[nonzIter.getIndex()] += product;
            } else {
                m_negativeSums[nonzIter.getIndex()] += product;
            }
        }
    }
    unsigned int index;
    for (index = 0; index < m_phase1ReducedCosts.size(); index++) {
        m_phase1ReducedCosts[index] = Numerical::stableAdd( m_positiveSums[index], m_negativeSums[index] );
    }
    //TODO: az elso fazisu redukalt koltseget itt taroljuk
    //        a masodik fazisu redukalt koltseghez behuzzuk
    //        az m_reducedCosts vektort
}

int PrimalDantzigPricing::performPricingPhase1()
{
    initPhase1();
    int maxIndex = -1;
    int minIndex = -1;
    m_reducedCost = 0.0;
    m_incomingIndex = -1;
    Numerical::Double maxReducedCost = feasibilityTolerance;
    Numerical::Double minReducedCost = -feasibilityTolerance;
    IndexList<const Numerical::Double*>::Iterator iter, iterEnd;
    m_updater->m_variableStates.getIterators(&iter, &iterEnd, Simplex::NONBASIC_AT_LB, 1);

    for (; iter != iterEnd; iter++) {
        unsigned int variableIndex = iter.getData();
        if (m_updater->m_used[variableIndex] == true) {
            continue;
        }
        if (m_phase1ReducedCosts[variableIndex] < minReducedCost) {
            minIndex = variableIndex;
            minReducedCost = m_phase1ReducedCosts[variableIndex];
        }
    }

    m_updater->m_variableStates.getIterators(&iter, &iterEnd, Simplex::NONBASIC_AT_UB, 1);

    for (; iter != iterEnd; iter++) {
        unsigned int variableIndex = iter.getData();
        if (m_updater->m_used[variableIndex] == true) {
            continue;
        }
        if (m_phase1ReducedCosts[variableIndex] > maxReducedCost) {
            maxIndex = variableIndex;
            maxReducedCost = m_phase1ReducedCosts[variableIndex];
        }
    }

    m_updater->m_variableStates.getIterators(&iter, &iterEnd, Simplex::NONBASIC_FREE, 1);
    for (; iter != iterEnd; iter++) {
        unsigned int variableIndex = iter.getData();
        if (m_updater->m_used[variableIndex] == true) {
            continue;
        }
        if (m_phase1ReducedCosts[variableIndex] < minReducedCost) {
            minIndex = variableIndex;
            minReducedCost = m_phase1ReducedCosts[variableIndex];
        } else if (m_phase1ReducedCosts[variableIndex] > maxReducedCost) {
            maxIndex = variableIndex;
            maxReducedCost = m_phase1ReducedCosts[variableIndex];
        }
    }
    if (Numerical::fabs( minReducedCost ) > maxReducedCost) {
        m_reducedCost = minReducedCost;
        m_incomingIndex = minIndex;
        return minIndex;
    }
    m_reducedCost = maxReducedCost;
    m_incomingIndex = maxIndex;
    return maxIndex;
}

void PrimalDantzigPricing::initPhase2() {

}

int PrimalDantzigPricing::performPricingPhase2()
{
    initPhase2();

}

void PrimalDantzigPricing::releaseUsed() {
    unsigned int size = m_updater->m_used.size();
    m_updater->m_used.clear();
    m_updater->m_used.resize( size, false );
}

void PrimalDantzigPricing::lockLastIndex() {
    if (m_incomingIndex != -1) {
        m_updater->m_used[m_incomingIndex] = true;
    } else {
        // TODO: kell ez ide egyaltalan?
        throw NumericalException("Invalid column lock index!");
    }
}

