//=================================================================================================
/*!
//  This file is part of the Pannon Optimizer library. 
//  This library is free software; you can redistribute it and/or modify it under the 
//  terms of the GNU Lesser General Public License as published by the Free Software 
//  Foundation; either version 3.0, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
//  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//  See the GNU General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public License; see the file 
//  COPYING. If not, see http://www.gnu.org/licenses/.
*/
//=================================================================================================

/**
 * @file dualratiotest.h This file contains the API of the DualRatiotest class.
 */

#ifndef DUALRATIOTEST_H
#define DUALRATIOTEST_H

#include <vector>
#include <linalg/matrix.h>
#include <utils/indexlist.h>
#include <simplex/dualfeasibilitychecker.h>
#include <utils/breakpointhandler.h>

class Model;

/**
 * The dual ratiotest determines the incoming variable, and the dual steplength.
 *
 * @class DualRatioTest
 */
class DualRatiotest{
    friend class DualRatiotestTestSuite;

public:

    /**
     * This indicates the ratio choosing methods that are implemented in the class.
     */
    enum DUAL_RATIOTEST_METHOD{
        ONE_STEP = 0,
        PIECEWISE_LINEAR_FUNCTION,
        STABLE_PIVOT
    };

    /**
     * Initializing constructor of the class.
     *
     * @param model The simplex model of the LP problem.
     * @param reducedCosts Vector of the reduced costs so that the ratios can be defined.
     * @param reducedCostFeasibilities Reduced cost feasibilities defined by the DualFeasibilityChecker.
     * @param variableStates Variable stated determining whether a variable is on its lower or upper bound
     *
     * @constructor
     */
    DualRatiotest(const SimplexModel & model,
                  DenseVector &reducedCosts,
                  const IndexList<>& reducedCostFeasibilities,
                  const IndexList<const Numerical::Double*>& variableStates);

    /**
     * Default destructor of the class.
     *
     * @destructor
     */
    virtual ~DualRatiotest(){}

    /**
     * Getter of the updateFeasibilitySets member.
     *
     * @return DualRatiotest::m_updateFeasibilitySets.
     */
    inline const std::vector<std::pair<int,char> >& getUpdateFeasibilitySets(){return m_updateFeasibilitySets;}

    /**
     * Getter of the m_becomesFeasible member.
     *
     * @return DualRatiotest::m_becomesFeasible.
     */
    inline const std::vector<int>& getBecomesFeasible(){return m_becomesFeasible;}

    /**
     * Getter that returns the incoming variable's index.
     *
     * @return the index of the incoming variable.
     */
    inline int getIncomingVariableIndex()const{return m_incomingVariableIndex;}

    /**
     * Getter that returns the dual steplength.
     *
     * @return the dual steplength.
     */
    inline Numerical::Double getDualSteplength()const{return m_dualSteplength;}

    /**
     * Getter that returns the phase 1 objective value.
     *
     * @return the phase 1 obective value.
     */
    inline Numerical::Double getPhaseIObjectiveValue()const{return m_phaseIObjectiveValue;}

    /**
     * Getter that returns with a vector containing the variables on which a boundflip operation shall be performed.
     *
     * @return the vector containing the variables on which a boundflip operation shall be performed.
     */
    inline const std::vector <unsigned int>& getBoundflips()const{return m_boundflips;}

    /**
     * This function performs and controls the phase 1 dual ratiotest.
     *
     * @param alpha The alpha vector needed to define ratios.
     * @param phaseIReducedCost The phase 1 reduced cost needed to define t > 0 and t < 0 cases.
     * @param phaseIObjectiveValue The phase 1 objective value.
     */
    void performRatiotestPhase1(const DenseVector &alpha,
                                Numerical::Double phaseIReducedCost,
                                Numerical::Double phaseIObjectiveValue);

    /**
     * This function performs and controls the phase 2 dual ratiotest.
     *
     * @param outgoingVariableIndex The index of the variable chosen by the pricing to leave the basis.
     * @param alpha The aplha vector needed to define ratios.
     * @param workingTolerance The optimality tolerance.
     *
     * @throws FallbackException if there is any infeasible variable.
     * @throws DualUnboundedException if no breakpoint was found.
     */
    void performRatiotestPhase2(unsigned int outgoingVariableIndex,
                                const DenseVector &alpha,
                                Numerical::Double workingTolerance);

    //Wolfe
    /**
     * Performs ratiotest according to Wolfe's 'ad hoc' method.
     * @param alpha The alpha value needed to define ratios.
     * @return true if a pivot row p is found.
     */
    bool performWolfeRatiotest(const DenseVector &alpha);

    /**
     * This function performs Wolfe's 'ad hoc' method.
     * @param outgoingVariableIndex The index of the outgoing variable.
     * @param alpha The alpha value needed to define ratios.
     * @throw FallbackException if any variable is infeasible
     */
    void wolfeAdHocMethod(int outgoingVariableIndex, const DenseVector &alpha, Numerical::Double workingTolerance);

    /**
     * Getter of degenerate at lb member, this is required in PrimalSimplex Wolfe special update.
     * @return  reference of member m_degenerateAtLB
     */
    inline IndexList<>& getDegenerateAtLB(){return m_degenerateAtLB;}

    /**
     * Getter of degenerate at ub member, this is required in PrimalSimplex Wolfe special update.
     * @return  reference of member m_degenerateAtUB
     */
    inline IndexList<>& getDegenerateAtUB(){return m_degenerateAtUB;}

    /**
     * Getter of degeneracy depth member.
     * @return m_degenDepth
     */
    inline unsigned int getDegenDepth()const{return m_degenDepth;}

    /**
     * Returns whether Wolfe's 'ad hoc' method was used during the ratiotest.
     * @return true if Wolfe is on, otherwise false.
     */
    inline bool isWolfeActive() const {return m_wolfeActive;}

    //Ratiotest study
    /**
     * Getter for the ratiotest studies.
     *
     * @return DualRatiotest::m_stablePivotActivationPhase1
     */
    inline int getStablePivotActivationPhase1() const {return m_stablePivotActivationPhase1;}

    /**
     * Getter for the ratiotest studies.
     *
     * @return DualRatiotest::m_stablePivotBackwardStepsPhase1
     */
    inline int getStablePivotBackwardStepsPhase1() const {return m_stablePivotBackwardStepsPhase1;}

    /**
     * Getter for the ratiotest studies.
     *
     * @return DualRatiotest::m_stablePivotForwardStepsPhase1
     */
    inline int getStablePivotForwardStepsPhase1() const {return m_stablePivotForwardStepsPhase1;}

    /**
     * Getter for the ratiotest studies.
     *
     * @return DualRatiotest::m_fakeFeasibilityActivationPhase1
     */
    inline int getFakeFeasibilityActivationPhase1() const {return m_fakeFeasibilityActivationPhase1;}

    /**
     * Getter for the ratiotest studies.
     *
     * @return DualRatiotest::m_fakeFeasibilityCounterPhase1
     */
    inline int getFakeFeasibilityCounterPhase1() const {return m_fakeFeasibilityCounterPhase1;}

    /**
     * Getter for the ratiotest studies.
     *
     * @return DualRatiotest::m_stablePivotNotFoundPhase2
     */
    inline int getStablePivotNotFoundPhase2() const {return m_stablePivotNotFoundPhase2;}

    /**
     * Getter for the ratiotest studies.
     *
     * @return DualRatiotest::m_fakeFeasibilityActivationPhase2
     */
    inline int getFakeFeasibilityActivationPhase2() const {return m_fakeFeasibilityActivationPhase2;}

    /**
     * Getter for the ratiotest studies.
     *
     * @return DualRatiotest::m_fakeFeasibilityCounterPhase2
     */
    inline int getFakeFeasibilityCounterPhase2() const {return m_fakeFeasibilityCounterPhase2;}

    /**
     * Returns whether the last iteration was degenerate or not.
     *
     * @return 0 if the last iteration was not degenerate, otherwise not 0.
     */
    inline int isDegenerate() const {return m_degenerate;}

private:
    /**
     * The simplex model of the LP problem.
     */
    const SimplexModel& m_model;

    /**
     * Vector of the reduced costs so that the ratios can be defined.
     * Do not set to const reference, because in Wolfe's 'ad hoc' method the ratiotest should be able to
     * change the reduced cost values thus it is a virtual perturbation.
     */
    DenseVector& m_reducedCosts;

    /**
     * Index list storing the feasibilities of the reduced costs defined by the DualFeasibilityChecker.
     */
    const IndexList<>& m_reducedCostFeasibilities;

    /**
     * Index list storing variable states, to be able to check whether a variable is on its lower or upper bound.
     */
    const IndexList<const Numerical::Double*>& m_variableStates;

    /**
     * With this the t > 0, and t < 0 cases can easly handled in the ratiotest.
     * When defining the ratios we compute with sigma*alpha values.
     */
    int m_sigma;

    /**
     * This holds the indices of the basicvariables, whose value is equal (with tolerance) to their lower bounds.
     *
     * This is a degeneracy set required in Wolfe's 'ad hoc' method.
     */
    IndexList <> m_degenerateAtLB;

    /**
     * This holds the indices of the basicvariables, whose value is equal (with tolerance) to their upper bounds.
     *
     * This is a degeneracy set required in Wolfe's 'ad hoc' method.
     */
    IndexList <> m_degenerateAtUB;

    /**
     * This is is true if Wolfe's 'ad hoc' method is active (degeneracy is present).
     */
    bool m_wolfeActive;

    /**
     * Depth of degeneracy in Wolfe's 'ad hoc' method.
     */
    unsigned int m_degenDepth;

    /**
     * The variable index of the incoming variable.
     */
    int m_incomingVariableIndex;

    /**
      * Age vector to record transformation counts.
      */
    std::vector<double> m_variableAge;


    /**
     * The value of the dual steplength defined in the ratiotest.
     */
    Numerical::Double m_dualSteplength;

    /**
     * The dual phase 1 objective value, needed in the piecewise linear method.
     */
    Numerical::Double m_phaseIObjectiveValue;

    /**
     * Vector storing the variable indices on which a boundflip operation shall be performed.
     */
    std::vector <unsigned int> m_boundflips;

    /**
     * The phase 1 objective value in the current iteration before the ratiotest is performed.
     */
    Numerical::Double m_initialPhaseIObjectiveValue;

    /**
     * The phase 2 objective value in the current iteration before the ratiotest is performed.
     */
    Numerical::Double m_initialPhaseIIObjectiveValue;

    /**
     * This class stores and provides the ratios and performs the sorting if necessary.
     */
    BreakpointHandler m_breakpointHandler;

    //Ratiotest research measures
    /**
     * Counter for the ratiotest studies.
     * It counts the number of activating the stable pivot procedure in phase1.
     */
    int m_stablePivotActivationPhase1;

    /**
     * Counter for the ratiotest studies.
     * It counts the number of steps taken left from the top of the piecewise linear function.
     */
    int m_stablePivotBackwardStepsPhase1;

    /**
     * Counter for the ratiotest studies.
     * It counts the number of steps taken right from the top of the piecewise linear function.
     */
    int m_stablePivotForwardStepsPhase1;

    /**
     * Counter for the ratiotest studies.
     * It's incremented if fake feasible variables are found in phase 1.
     */
    int m_fakeFeasibilityActivationPhase1;

    /**
     * Counter for the ratiotest studies.
     * It computes the number of fake feasible variables.
     */
    int m_fakeFeasibilityCounterPhase1;

    /**
     * Counter for the ratiotest studies.
     * It's incremented if the stable pivot procedure hasn't find a good pivot in phase 2.
     */
    int m_stablePivotNotFoundPhase2;

    /**
     * Counter for the ratiotest studies.
     * It's incremented if fake feasible variables are found in phase 2.
     */
    int m_fakeFeasibilityActivationPhase2;

    /**
     * Counter for the ratiotest studies.
     * It computes the number of fake feasible variables.
     */
    int m_fakeFeasibilityCounterPhase2;

    //Parameter references
    /**
     * Value of the run-time parameter "e_pivot_generation",
     * see SimplexParameterHandler for details.
     */
    bool m_ePivotGeneration;

    /**
     * Reference of the run-time parameter "nonlinear_dual_phaseI_function",
     * see SimplexParameterHandler for details.
     */
    const DUAL_RATIOTEST_METHOD m_nonlinearDualPhaseIFunction;

    /**
     * Reference of the run-time parameter "nonlinear_dual_phaseII_function",
     * see SimplexParameterHandler for details.
     */
    const DUAL_RATIOTEST_METHOD m_nonlinearDualPhaseIIFunction;

    /**
     * Reference of the "e_optimality" tolerance run-time parameter,
     * see SimplexParameterHandler for details.
     */
    const double & m_optimalityTolerance;

    /**
     * Reference of the "e_pivot" tolerance run-time parameter,
     * see SimplexParameterHandler for details.
     */
    const double & m_pivotTolerance;

    /**
     * Reference of the "enable_fake_feasibility" run-time parameter,
     * see SimplexParameterHandler for details.
     */
    const bool & m_enableFakeFeasibility;

    /**
     * Parameter reference of run-time parameter "expand",
     * see SimplexParameterHandler for details.
     */
    const std::string & m_expand;

    /**
     * Parameter reference of run-time parameter "avoidthetamin",
     * see SimplexParameterHandler for details.
     */
     const bool & m_avoidThetaMin;

     /**
      * Parameter reference of run-time parameter "enable_wolfe_adhoc",
      * see SimplexParameterHandler for details.
      */
      const bool & m_wolfe;

    /**
     * This is the value of the tolerance increment in the expand procedure.
     */
    Numerical::Double m_toleranceStep;

    /**
     * This contains the directions of the phase 1 ratios.
     * If the variable moves up from set M or F it stores 1,
     * otherwise if it moves down from set P or F it stores 0.
     */
    std::vector<char> m_ratioDirections;

    /**
     * This vector stores the changes in the feasibilities.
     * If the variable moves from set F to M it stores the variable index and -1 as a pair.
     */
    std::vector<std::pair<int,char> > m_updateFeasibilitySets;

    /**
     * This vector contains the variable idices of explicit feasible variables.
     */
    std::vector<int> m_becomesFeasible;

    /**
     * Describes whether the last iteration was degenerate or not.
     */
    bool m_degenerate;

    /**
     * This function computes the ratios in dual phase 1.
     *
     * @param alpha The reduced cost values derived by the alpha values.
     */
    void generateSignedBreakpointsPhase1(const DenseVector &alpha);

    /**
     * This function computes the piecewise linear concave function in dual phase 1.
     * If it is called the BreakpointHandler performs the necessary sorting operations.
     *
     * @param alpha The alpha vector.
     * @param iterationCounter The count of the computed breakpoints.
     * @param functionSlope The value of the objective function's slope.
     */
    void computeFunctionPhase1(const DenseVector &alpha, unsigned int& iterationCounter, Numerical::Double &functionSlope);

    /**
     * This is the phase 1 stable pivot procedure that provides the solver a numerically stable pivot candidate.
     *
     * @param iterationCounter The maximum iteration count of the piecewise linear function.
     * @param alpha The alpha vector.
     * @param functionSlope The value of the objective function's slope.
     */
    void useNumericalThresholdPhase1(unsigned int iterationCounter, const DenseVector &alpha, Numerical::Double &functionSlope);

    /**
     * This function computes the ratios in dual phase 2.
     *
     * @param alpha the reduced cost values are devided by the alpha values
     */
    void generateSignedBreakpointsPhase2(const DenseVector &alpha);

    /**
     * With this function we can define ratios corresponding to the expanding tolerance.
     *
     * @param alpha The alpha vector.
     * @param workingTolerance The value of the expanding tolerance in the current iteration.
     */
    void generateExpandedBreakpointsPhase2(const DenseVector &alpha, Numerical::Double workingTolerance);

    /**
     * This function computes the piecewise linear concave function in dual phase 2.
     * If it is called the BreakpointHandler performs the necessary sorting operations.
     *
     * @param alpha The alpha vector.
     * @param iterationCounter The count of the computed breakpoints.
     * @param functionSlope The value of the objective function's slope.
     * @param workingTolerance The value of the expanding tolerance in the current iteration.
     */
    void computeFunctionPhase2(const DenseVector &alpha, unsigned int& iterationCounter, Numerical::Double &functionSlope, Numerical::Double workingTolerance);

    /**
     * This is the phase 2 stable pivot procedure that provides the solver a numerically stable pivot candidate.
     *
     * @param iterationCounter The maximum iteration count of the piecewise linear function.
     * @param alpha The alpha vector.
     * @param functionSlope The value of the objective function's slope.
     */
    void useNumericalThresholdPhase2(unsigned int iterationCounter, const DenseVector &alpha);

    /**
     * Converts a string describing a dual ratiotest method to the method describing enum.
     *
     * @param method The string to be converted.
     * @return The dual ratiotest method in DualRatiotest::DUAL_RATIOTEST_METHOD enum.
     */
    static DUAL_RATIOTEST_METHOD getDualRatiotestMethod(const std::string & method) {
        if (method == "TRADITIONAL") {
            return ONE_STEP;
        }
        if (method == "PIECEWISE") {
            return PIECEWISE_LINEAR_FUNCTION;
        }
        if (method == "PIECEWISE_THRESHOLD") {
            return STABLE_PIVOT;
        }
        // TODO: exception
        return ONE_STEP;
    }
};
#endif // DUALRATIOTEST_H
