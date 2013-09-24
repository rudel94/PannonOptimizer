/*
 * sbf_logical.cpp
 *
 *  Created on: Apr 1, 2012
 *      Author: Virag Varga <varga@dcs.uni-pannon.hu>
 */

#include <simplex/startingbasisfinder/sbf_logical.h>

#include <simplex/simplex.h>

SbfLogical::SbfLogical(const SimplexModel& model,
                       std::vector<int>* basisHead,
                       IndexList<Numerical::Double>* variableStates,
                       Vector* basicVaraibleValues,
                       LOGICAL_BASIS_STRATEGY strategy):
    SbfSuper(model, basisHead, variableStates, basicVaraibleValues),
    m_strategy(strategy)
{

}

SbfLogical::~SbfLogical()
{

}

void SbfLogical::run()
{
    register unsigned int i;
    register unsigned int j;

    unsigned int basisSize = m_model.getRowCount();

    m_basisHead->clear();

    /* Basic variables: set state to BASIC */
    for (i=0, j=basisSize; i < basisSize; i++, j++) {
        m_basisHead->push_back(j);
        //TODO: X_B pointere
        Numerical::Double RHSValue = m_model.getRhs().at(j);
        m_variableStates->insert(Simplex::BASIC, j , RHSValue);
        m_basicVariableValues->set(j, RHSValue);
    }

    /* Nonbasic variables: set state to NONBASIC_AT_UB/LB depending on the strategy used */
    switch (m_strategy) {

    case LOWER_LOGICAL: {
        for (i=0; i<basisSize; i++) {
            adjustVariableByType(i, Simplex::NONBASIC_AT_LB);
        }
        break;
    }

    case UPPER_LOGICAL: {
        for (i=0; i<basisSize; i++) {
            adjustVariableByType(i, Simplex::NONBASIC_AT_UB);
        }
        break;
    }

    case MIXED_LOGICAL: {
        const Vector & costs = m_model.getCostVector();
        switch (m_model.getObjectiveType()) {
        case MINIMIZE:
            for (i=0; i<basisSize; i++) {
                if (costs.at(i) < 0) {
                    adjustVariableByType(i, Simplex::NONBASIC_AT_LB);
                } else {
                    adjustVariableByType(i, Simplex::NONBASIC_AT_UB);
                }
            }
            break;

        case MAXIMIZE:
            for (i=0; i<basisSize; i++) {
                if (costs.at(i) > 0) {
                    adjustVariableByType(i, Simplex::NONBASIC_AT_UB);
                } else {
                    adjustVariableByType(i, Simplex::NONBASIC_AT_LB);
                }
            }
            break;
        default:
            LPERROR("Unknown objective type");
        }
    }

    default: {
        /* */
        DEVWARNING(D::SBF_LOGICAL, "Unhandled logical basis finder algorithm selected.");
        break;
    }

    }
}
