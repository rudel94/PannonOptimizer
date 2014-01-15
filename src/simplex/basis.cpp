/**
 * @file basis.cpp
 */

#include <simplex/basis.h>

#include <simplex/simplex.h>
#include <simplex/simplexmodel.h>

#include <utils/thirdparty/prettyprint.h>

#include <fstream>

Basis::Basis(const SimplexModel& model,
             std::vector<int>* basisHead,
             IndexList<const Numerical::Double*>* variableStates,
             const Vector &basicVariableValues) :
    m_model(model),
    m_basisHead(basisHead),
    m_variableStates(variableStates),
    m_basicVariableValues(basicVariableValues)
{
    m_isFresh = false;
}

Basis::~Basis()
{

}

void Basis::setNewHead() {
    //This vector indicates the pattern of the basis columns
    std::vector<char> nonbasic(m_model.getColumnCount() + m_model.getRowCount(), false);
    //First mark the positions of the given basis head
    for (std::vector<int>::iterator it = m_basisHead->begin(); it < m_basisHead->end(); it++) {
        nonbasic.at(*it) = true;
    }
    //Update the basis head with the recomputed one
    m_basisHead->assign(m_basisNewHead.begin(), m_basisNewHead.end());
    //Set the basic variables state and remove their traces from the pattern vector
    for (std::vector<int>::iterator it = m_basisHead->begin(); it < m_basisHead->end(); it++) {
        nonbasic.at(*it) = false;
        if(m_variableStates->where(*it) != Simplex::BASIC) {
            m_variableStates->move(*it, Simplex::BASIC, &(m_basicVariableValues.at(it - m_basisHead->begin())));
        } else {
            m_variableStates->setAttachedData(*it, &(m_basicVariableValues.at(it - m_basisHead->begin())));
        }
    }
    //If the pattern vector still contains true values then the basis head is modified, thus some variables
    //are aout of the basis, these must be marked as nonbasic and their states must be updated too.
    for (std::vector<char>::iterator it = nonbasic.begin(); it < nonbasic.end(); it++) {
        if (*it == true) {
            const Variable& variable = m_model.getVariable(*it);
            if (variable.getType() == Variable::FREE) {
                m_variableStates->move(*it, Simplex::NONBASIC_FREE, &ZERO);
            } else if (variable.getType() == Variable::MINUS) {
                m_variableStates->move(*it, Simplex::NONBASIC_AT_UB, &(variable.getUpperBound()));
                return;
            } else if (variable.getType() == Variable::PLUS) {
                m_variableStates->move(*it, Simplex::NONBASIC_AT_LB, &(variable.getLowerBound()));
                return;
            } else {
                m_variableStates->move(*it, Simplex::NONBASIC_AT_LB, &(variable.getLowerBound()));
                return;
            }
        }
    }
}

void Basis::checkSingularity() {
    DEVINFO(D::PFIMAKER, "Checking singularity");
    int singularity = 0;
    for (std::vector<int>::iterator it = m_basisNewHead.begin(); it < m_basisNewHead.end(); it++) {
        if (*it == -1) {
            DEVINFO(D::PFIMAKER, "Given basis column " << it - m_basisNewHead.begin() << " is singular, replacing with unit vector");
            *it = it - m_basisNewHead.begin() + m_model.getColumnCount();
            singularity++;
        }
    }
    if (singularity != 0) {
        LPWARNING("The given basis is singular, the measure of singularity is " << singularity);
    }
}

Vector* Basis::createEta(const Vector& vector, int pivotPosition)
{
    Vector* eta = new Vector();
    eta->prepareForData(vector.nonZeros(), vector.length());
    //TODO Ezt vajon lehet gyorsabban?
    Numerical::Double atPivot = vector.at(pivotPosition);

    if (Numerical::equals(atPivot, 0)) {
        throw NumericalException("NUMERICAL problem: Pivot element is zero at row " + pivotPosition );
    } else {
        Vector::NonzeroIterator it = vector.beginNonzero();
        Vector::NonzeroIterator endit = vector.endNonzero();
        for (; it < endit; it++) {
            if (it.getIndex() == (unsigned int) pivotPosition) {
                eta->newNonZero(1 / atPivot, pivotPosition);
            } else {
                eta->newNonZero(-(*it) / atPivot, it.getIndex());
//                if(Numerical::fabs(-(*it) / atPivot) < Numerical::AbsoluteTolerance){
//                    LPERROR("INVERSION ERROR!: "<<"atPivot: "<<atPivot);
//                    LPERROR("INVERSION ERROR!: "<<"(*it): "<<(*it));
//                    LPERROR("INVERSION ERROR!: "<<"index: "<<it.getIndex()<< " value: "<<Numerical::fabs(-(*it) / atPivot));
//                }
            }
        }
        DEVINFO(D::PFIMAKER, "Eta vector created with pivot " << pivotPosition);
        return eta;
    }
}

void Basis::printActiveSubmatrix() const
{
#ifndef NDEBUG
    DEVINFO(D::PFIMAKER, "Active submatrix pattern by columns");
    for (int i = 0; i < (int) m_basicColumns.size(); i++) {
        std::string s;
        for (std::vector<const Vector*>::const_iterator it = m_basicColumns.begin(); it < m_basicColumns.end(); it++) {
            s += Numerical::equals((*it)->at(i), 0) ? "-" : "X";
        }
        DEVINFO(D::PFIMAKER, s);
    }

#endif //!NDEBUG
}

