#ifndef PRIMALRATIOTEST_H
#define PRIMALRATIOTEST_H

#include <vector>
#include <linalg/vector.h>
#include <linalg/matrix.h>
#include <utils/indexlist.h>
#include <simplex/primalfeasibilitychecker.h>
#include <simplex/primalratiotestupdater.h>

class Model;

class PrimalRatiotest{

    struct BreakPoint{
        int index;
        Numerical::Double value;
        Numerical::Double functionValue;

        friend ostream & operator<<(ostream & os, const BreakPoint & breakpoint)
        {
            os << "(" << breakpoint.index << "; " << breakpoint.value << "; " << breakpoint.functionValue << ")";
            return os;
        }
    };

public:
    PrimalRatiotest(const SimplexModel& model,
                    const Vector& basicVariableValues,
                    const std::vector<int>& basishead,
                    const IndexList<>& basicVariableFeasibilities,
                    const IndexList<const Numerical::Double*>& variableStates,
                    const Vector& reducedcosts
                    //PrimalRatiotestUpdater& primalRatiotestUpdater
                    );

    inline unsigned int getOutgoingVariableIndex()const{return m_outgoingVariableIndex;}

    inline Numerical::Double getPrimalSteplength()const{return m_primalSteplength;}

    inline Numerical::Double getDualSteplength()const{return m_dualSteplength;}

    inline Numerical::Double getPhaseIObjectiveValue()const{return m_phaseIObjectiveValue;}

    inline Numerical::Double getPhaseIIObjectiveValue()const{return m_phaseIIObjectiveValue;}

    inline const std::vector <unsigned int>& getBoundflips()const{return m_boundflips;}

    void performRatiotestPhase1(int incomingVariableIndex,
                                const Vector& alpha,
                                Numerical::Double phaseIReducedCost,
                                Numerical::Double phaseIObjectiveValue);
    void performRatiotestPhase2(int incomingVariableIndex,
                                const Vector& alpha);

private:
    const SimplexModel& m_model;
    const Vector& m_basicVariableValues;
    const std::vector<int>& m_basishead;
    const IndexList<>& m_basicVariableFeasibilities;
    const IndexList<const Numerical::Double*>& m_variableStates;
    const Vector& m_reducedcosts;

    //PrimalRatiotestUpdater& m_primalRatiotestUpdater;
    unsigned int m_outgoingVariableIndex;
    Numerical::Double m_dualSteplength; //TODO: hogy számolod?
    Numerical::Double m_primalSteplength;
    Numerical::Double m_phaseIObjectiveValue;
    Numerical::Double m_phaseIIObjectiveValue;
    std::vector <unsigned int> m_boundflips;

    void shift(std::vector<BreakPoint>* breakpoints, unsigned int startId, unsigned int stopId);
    void getNextElement(std::vector<BreakPoint>* breakpoints, unsigned int length);
};

#endif // PRIMALRATIOTEST_H
