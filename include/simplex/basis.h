/**
 * @file basis.h This file contains the Basis class.
 */

#ifndef BASIS_H
#define BASIS_H

class SimplexModel;

#include <globals.h>
#include <vector>
#include <list>

#include <linalg/vector.h>

#include <simplex/simplex.h>

#include <utils/exceptions.h>
#include <utils/indexlist.h>

/**
 * A struct describing an ETM %Matrix.
 * This structure represents an Elementary Transformation %Matrix (ETM).
 * Az ETM is such a matrix that differs from the unit matrix in only one column.
 * This struct is used throughout the inversion process and the FTRAN and BTRAN
 * operations. Az ETM is a $m \times m$ matrix.
 *
 *@struct
 */
struct ETM
{
    /**
     * Default constructor.
     */
    ETM(): eta(NULL), index(0) {}

    /**
     * Initializing constructor.
     * @param eta is the nontrivial column of the ETM.
     * @param index of the nontrivial column
     */
    ETM(Vector* eta, unsigned int index): eta(eta), index(index){}
    /**
     * Eta vector.
     * The nontrivial column of the ETM is the eta column. This is a pointer of
     * that vector, it is allocated dynamically .
     */
    Vector* eta;

    /**
     * Index of the nontrivial column.
     */
    unsigned int index;

};

/**
 * This class describes a general basis.
 * A basis is a nonsingular matrix.
 * @class Basis
 */
class Basis {
public:

    /**
     * This indicates the possible FTRAN modes for further development.
     * Describtion of the FTRAN operation can be found at Basis::Ftran().
     */
    enum FTRAN_MODE
    {
        DEFAULT_FTRAN
    };

    /**
     * This indicates the possible BTRAN modes for further development.
     ** Describtion of the FTRAN operation can be found at Basis::Btran().
     */
    enum BTRAN_MODE
    {
        DEFAULT_BTRAN
    };

    /**
     * Initializing constructor of the class.
     * @param model holds the LP problem.
     * @param basisHead contains the basicvariable indices
     * @param variableStates indexlist containing variable states (being at lower upper bound)
     * @param basicVariableValues the X_B vector
     *
     * @constructor
     */
    Basis(const SimplexModel& model,
          std::vector<int>* basisHead,
          IndexList<const Numerical::Double*>* variableStates,
          const Vector& basicVariableValues);

    /**
     * Default destructor of the class.
     *
     * @destructor
     */
    virtual ~Basis();

    /**
     * Gives information about the state of the inverse.
     * An inverse is considered fresh, if it is in the state after factorization.
     * (There was no modification since then)
     *
     * @return true if the basis is fresh, otherwise false.
     */
    inline bool isFresh() const {return m_isFresh;}

    /**
     * Pure virtual function for performing a basis inversion.
     * Should be implemented in the derived classes. The different basis types mean
     * different inversion techniques.
     */
    virtual void invert() = 0;

    /**
     * Pure virtual function performing the basis change.
     * @param vector the alpha vector of the incoming variable.
     * @param pivotRow shows the basis leaving variable
     * @param incoming shows the incoming variable
     * @param outgoingState shows on which bound the outgoing variable will be set
     *
     * @throws NumericalException if the outgoing variable trys to leave the basis
     * with a value different from LB or UB.
     */
    virtual void append(const Vector & vector, int pivotRow, int incoming, Simplex::VARIABLE_STATE outgoingState) = 0;

    /**
     * The basic FTRAN operation.
     * The operation B^-1 * v is called FTRAN.
     * @param vector on which the operation shall be done
     * @param mode optional parameter for the different FTRAN modes, see Basis::FTRAN_MODE
     */
    virtual void Ftran(Vector & vector, FTRAN_MODE mode = DEFAULT_FTRAN) const = 0;

    /**
     * The basic BTRAN operation.
     * The operation v * B^-1 is called BTRAN.
     * @param vector on which the operation shall be done
     * @param mode optional parameter for the different BTRAN modes, see Basis::BTRAN_MODE
     */
    virtual void Btran(Vector & vector, BTRAN_MODE mode = DEFAULT_BTRAN) const = 0;

    /**
     * Pure virtual function providing general information about the basis transformation.
     * It prints the used techniques, and other statistics.
     */
    virtual void printTransformationStatistics() const = 0;

    /**
     * Returns the measure of singularity in case of the basis becomes numerically singular.
     * @return Basis::m_singularityCounter
     */
    int getSingularityCount() {return m_singularityCounter;}
protected:
    /**
     * Model holds the LP problem.
     */
    const SimplexModel& m_model;

    /**
     * Vector containing the basicvariable indices.
     */
    std::vector<int>* m_basisHead;

    /**
     * Index list containing the variable states (being at lower upper bound).
     */
    IndexList<const Numerical::Double*>* m_variableStates;

    /**
     * The X_B vector.
     */
    const Vector& m_basicVariableValues;

    /**
     * The new basis head is needed in the Basis::setNewHead() function.
     */
    std::vector<int> m_basisNewHead;

    /**
     * Stores the active submatrix of used for the inversion (columnwise).
     * The dimension of the submatrix is m*k , where k is the number of struxtural variables
     * in the basis head. The rows of logical are also contained here. The logical rows
     * are omitted by setting the appropriate row counts to -1.
     */
    std::vector<const Vector*> m_basicColumns;

    /**
     * Contains copies of the basic columns. It's needed in the child classes for inverting.
     */
    std::vector<Vector*> m_basicColumnCopies;

    /**
     * The vector of linked lists of column indices.
     * Each linked list represents a group of columns that have the same column count.
     * The first list has columns with a column count of 1, the second with 2, etc.
     */
    IndexList<> m_columnCountIndexList;

    /**
     * The vector of linked lists of row indices.
     * Each linked list represents a group of rows that have the same row count.
     * The first list has rows with a row count of 1, the second with 2, etc.
     */
    IndexList<> m_rowCountIndexList;

    /**
     * Describes whether the basis is fresh or has been changed.
     */
    bool m_isFresh;

    /**
     * Counters for the nonzero element in the basis, and basis inverse.
     */
    unsigned int m_basisNonzeros, m_inverseNonzeros;

    /**
     * The measure of singularity in case if the basis becomes numerically singular.
     */
    int m_singularityCounter;

    /**
     * Parameter renference for the run-time parameter "e_pivot",
     * for details see SimplexParameterHandler.
     */
    const double & m_inversion;

    /**
     * This function sets the new basis head, after inversion, it updates the variable states too.
     */
    void setNewHead();

    /**
     * This function creates an Eta vector using the pivot element. The vector is needed to determine the basis inverse.
     * @param vector is the alpha vector containing the pivot element.
     * @param pivotPosition points to the pivot element in the vector
     * @return the computed Eta vector.
     *
     * @throws NumericalException if the pivot element is zero.
     */
    Vector* createEta(const Vector& vector, int pivotPosition);

    /**
     * Checker for the numerical singularity.
     */
    virtual void checkSingularity() = 0;

    /**
     * Pure virtual function printing the used techniques and other statistics.
     */
    virtual void printStatistics() const = 0;

    /**
     * Printer of the active submatrix, for debug.
     */
    void printActiveSubmatrix() const;
};

#endif // BASIS_H
