#ifndef DUAL_RATIOTEST_H
#define DUAL_RATIOTEST_H

#include <globals.h>

#include <iostream>
#include <vector>
#include <linalg/vector.h>
#include <linalg/matrix.h>
#include <utils/indexlist.h>
#include <simplex/dualfeasibilitychecker.h>
#include <simplex/dualratiotestupdater.h>

class Model;

class DualRatiotest{

    struct BreakPoint{
        unsigned int index;
        Numerical::Double value;
        Numerical::Double functionValue;
    };

public:
    DualRatiotest(const SimplexModel & model,
                  const Vector& reducedCosts,
                  const IndexList<>& reducedCostFeasibilities,
                  const IndexList<const Numerical::Double*>& variableStates,
                  DualRatiotestUpdater& dualRatiotestUpdater);
    virtual ~DualRatiotest();

    inline unsigned int getIncomingVariableIndex()const{return m_incomingVariableIndex;}

    inline unsigned int getOugoingVariableIndex()const{return m_outgoingVariableIndex;}

    inline Numerical::Double getPrimalSteplength()const{return m_primalSteplength;}

    inline Numerical::Double getDualSteplength()const{return m_dualSteplength;}

    inline Numerical::Double getObjectiveFunctionPhase1()const{return m_objectiveFunctionPhase1;}

    inline Numerical::Double getObjectiveFunctionPhase2()const{return m_objectiveFunctionPhase2;}

    inline const std::vector <unsigned int>& getBoundflips()const{return m_boundflips;}

    void performRatiotestPhase1(unsigned int outgoing,
                                const Vector& alpha,
                                Numerical::Double phaseIReducedCost,
                                Numerical::Double phaseIObjectiveValue
                                );

    void performRatiotestPhase2(unsigned int outgoing,
                                const Vector& alpha,
                                Numerical::Double objectiveFunction
                                );

private:
    const SimplexModel& m_model;
    const Vector& m_reducedCosts;
    const IndexList<>& m_reducedCostFeasibilities;
    const IndexList<const Numerical::Double*>& m_variableStates;

    DualRatiotestUpdater& m_dualRatiotestUpdater;
    unsigned int m_incomingVariableIndex;
    unsigned int m_outgoingVariableIndex;
    Numerical::Double m_dualSteplength;
    Numerical::Double m_primalSteplength;
    Numerical::Double m_objectiveFunctionPhase1;
    Numerical::Double m_objectiveFunctionPhase2;
    std::vector <unsigned int> m_boundflips;
    std::vector <BreakPoint> breakpoints;

    void shift(std::vector<BreakPoint>* breakpoints, unsigned int startid, unsigned int stopid);
    void getNextElement(std::vector<BreakPoint>* breakpoints, unsigned int length);

};
#endif // DUAL_RATIOTEST_H
