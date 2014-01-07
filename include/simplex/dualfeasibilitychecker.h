/**
 * @file dualfeasibilitychecker.h
 */

#ifndef DUALFEASIBILITYCHECKER_H
#define DUALFEASIBILITYCHECKER_H

#include <globals.h>
#include <utils/indexlist.h>
#include <linalg/vector.h>
#include <simplex/simplexmodel.h>

class Basis;

class DualFeasibilityChecker{

public:
    DualFeasibilityChecker(const SimplexModel& model,
                           IndexList<const Numerical::Double *>* variableStates,
                           IndexList<>* reducedCostFeasibilities,
                           const Vector& reducedCosts,
                           const Basis& basis,
                           Numerical::Double* phaseIObjectiveValue);

    bool checkFeasibility();
    void computeFeasibility(Numerical::Double tolerance);
    void feasiblityCorrection(Vector* basicVariableValues);

private:
    const SimplexModel& m_model;
    IndexList<const Numerical::Double*>* m_variableStates;
    IndexList<>* m_reducedCostFeasibilities;
    const Vector& m_reducedCosts;
    const Basis& m_basis;
    Numerical::Double* m_phaseIObjectiveValue;
};

#endif // DUALFEASIBILITYCHECKER_H
