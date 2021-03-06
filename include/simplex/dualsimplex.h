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
 * @file dualsimplex.h This file contains the API of the DualSimplex class.
 */


#ifndef DUALSIMPLEX_H
#define	DUALSIMPLEX_H

#include <globals.h>
#include <simplex/simplex.h>
#include <simplex/dualpricing.h>
#include <simplex/dualratiotest.h>
#include <simplex/dualfeasibilitychecker.h>

/**
 * Describes a general dual simplex object, containing all members of the algorithm.
 * The members are the dual pricing, dual feasibility checker, dual ratiotest. In this class we
 * can find the dual implementations of the abstract members of class Simplex.
 *
 * @class DualSimplex
 */
class DualSimplex: public Simplex
{
    friend class SimplexController;
private:

    /**
     * Default constructor of the class.
     *
     * @param simplexController A reference to the controller to reach the controller API.
     *
     * @constructor
     */
    DualSimplex(Basis *basis);

    /**
     * Default destructor of the class.
     *
     * @destructor
     */
    ~DualSimplex();

    /**
     * The dual pricing determines an improving candidate (an outgoing variable, row).
     */
    DualPricing * m_pricing;

    /**
     * The dual feasibility checker can check the feasibility of the reduced costs.
     * It can compute the measure of dual infeasibility.
     */
    DualFeasibilityChecker * m_feasibilityChecker;

    /**
     * The dual ratiotest determines an incoming variable (column), and the dual steplength.
     * After the ratiotest is done the basis change can be performed.
     */
    DualRatiotest * m_ratiotest;

    /**
     * The primal reduced cost(d) is needed by the iteration reporter.
     * The measure of the change is the product of d and the primal theta.
     */
    Numerical::Double m_primalReducedCost;

    /**
     * The primal theta (steplength) is needed to update the basic variable values.
     * It is needed along with the primal reduced cost.
     */
    Numerical::Double m_primalTheta;

    /**
     * The dual theta (steplength) is used in updating the solution, and updating the reduced costs.
     */
    Numerical::Double m_dualTheta;

    /**
     * This vector is needed in the pricing updater.
     */
    DenseVector m_pivotRowOfBasisInverse;

    /**
     * A counter measuring row disabling
     */
    int m_askForAnotherRowCounter;

    /**
     * Computes the measure of dual infeasibility of the reduced costs.
     */
    virtual void computeFeasibility();

    /**
     * Performs the dual pricing.
     *
     * @throws DualInfeasibleException if there is no improving candidate in phase 1
     * @throws OptimalException if there is no improving candidate in phase 2
     */
    virtual void price();

    /**
     * Performs the dual ratiotest.
     */
    virtual void selectPivot();

    /**
     * Performs the basis change, computes the reduced costs and the basic solution.
     *
     * @throws PanOptException if there is a boundflipping variable in the basis.
     * @throws PanOptException if there is an invalid variable type.
     */
    virtual void update();

    /**
     * Function for updating the solution if Wolfe's 'ad hoc' method is used.
     */
    void wolfeSpecialUpdate();

    /**
     * Updates the reduced cost values.
     */
    virtual void updateReducedCosts();

    /**
     * Sets the objective function value, considering the current phase.
     *
     * @param secondPhase True if it is performed in the second phase.
     */
    virtual void setReferenceObjective(bool secondPhase);

    /**
     * Checks if a bad iteration was done.
     *
     * @param secondPhase True if it is performed in the second phase.
     */
    virtual void checkReferenceObjective(bool secondPhase);

    /**
     * Initializes the expand working tolerance.
     */
    virtual void initWorkingTolerance();

    /**
     * Computes the expand working tolerance in the next iteration.
     */
    virtual void computeWorkingTolerance();

    /**
     * Releases the locked variables in the pricing.
     */
    virtual void releaseLocks();

    /**
     * Computes the transformed alpha row.
     *
     * @throws NumericalException if the rowIndex is invalid.
     */
    void computeTransformedRow();

    // Interface of the iteration report provider:
    /**
     * Returns the iteration report fields of this class.
     *
     * @see IterationReportProvider.
     */
    std::vector<IterationReportField> getIterationReportFields(enum ITERATION_REPORT_FIELD_TYPE & type) const;

    /**
     * Getter of one iteration Entry.
     *
     * @see IterationReportProvider.
     */
    Entry getIterationEntry(const std::string & name,
                            enum ITERATION_REPORT_FIELD_TYPE & type) const;

    void resetTolerances();

    void detectExcessivelyInstability();
};

#endif /* DUALSIMPLEX_H */
