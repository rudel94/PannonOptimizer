/**
 * @file simplexmodel.cpp
 */

#include <simplex/simplexmodel.h>

SimplexModel::SimplexModel(const Model & model):
    m_model(model)
{
    m_rhs.setSparsityRatio(DENSE);
    makeComputationalForm();
}

void SimplexModel::makeComputationalForm()
{

    unsigned int i;

    //Set the matrix
    const Matrix & matrix = getMatrix();
    //Set the constraints
    const std::vector<Constraint> & constraints = getConstraints();

    //Set the cost vector
    const unsigned int rowCount = matrix.rowCount();
    const unsigned int columnCount = matrix.columnCount();
    const unsigned int variablesSize = rowCount + columnCount;

    Vector::NonzeroIterator it = m_model.getCostVector().beginNonzero();
    Vector::NonzeroIterator itEnd = m_model.getCostVector().endNonzero();

    m_costVector.resize(variablesSize);
    m_costVector.setSparsityRatio(DENSE);
    for(;it != itEnd; it++){
        m_costVector.setNewNonzero(it.getIndex(),*it);
    }

    //Set the variables
    m_variables.resize(variablesSize);

    m_rhs.reInit(rowCount);
    m_rhs.setSparsityRatio(DENSE);


    //Set the structural variables
    for (i=0; i < columnCount; i++) {
        //TODO: Is this fast enough?
        m_variables[i] = m_model.getVariable(i);
    }

    //Set the logical variables
    for (i=0; i < rowCount; i++) {
        m_variables[columnCount + i].setName(constraints.at(i).getName());

        switch(constraints.at(i).getType()) {
        case Constraint::LESS_OR_EQUAL: {
            m_variables[columnCount + i].setLowerBound(0.);
            m_variables[columnCount + i].setUpperBound(Numerical::Infinity);
            m_rhs.set(i, constraints.at(i).getUpperBound());
            break;
        }

        case Constraint::GREATER_OR_EQUAL: {
            m_variables[columnCount + i].setLowerBound(-Numerical::Infinity);
            m_variables[columnCount + i].setUpperBound(0.);
            m_rhs.set(i, constraints.at(i).getLowerBound());
            break;
        }

        case Constraint::RANGE: {
            m_variables[columnCount + i].setLowerBound(0.);
            m_variables[columnCount + i].setUpperBound(constraints.at(i).getUpperBound() - constraints.at(i).getLowerBound());
            m_rhs.set(i, constraints.at(i).getUpperBound());
            break;
        }

        case Constraint::NON_BINDING: {
            m_variables[columnCount + i].setLowerBound(-Numerical::Infinity);
            m_variables[columnCount + i].setUpperBound(Numerical::Infinity);
            m_rhs.set(i, 0.);
            Numerical::Double temp = constraints.at(i).getUpperBound();
            Numerical::Double temp2 = constraints.at(i).getLowerBound();
            if ( temp != -Numerical::Infinity) {
                m_rhs.set(i, temp);
            } else if ( temp2 != Numerical::Infinity) {
                m_rhs.set(i, temp2);
            } else {
                m_rhs.set(i, 0.);
            }
            break;
        }

        case Constraint::EQUALITY: {
            m_variables[columnCount + i].setLowerBound(0.);
            m_variables[columnCount + i].setUpperBound(0.);
            m_rhs.set(i, constraints.at(i).getUpperBound());
            break;
        }

        default: {
            LPWARNING("Unhandled case");
            break;
        }
        }
    }
}

void SimplexModel::print(std::ostream& out) const
{
    out << (getObjectiveType()==MINIMIZE?"min":"max");
    Vector::Iterator it = getCostVector().begin();
    Vector::Iterator end = getCostVector().end();
    for(unsigned int i=0; it<end; i++, it++){
        out << (i==0?" ":"+ ") << *it << "*x" << i << " \n";
    }
    out << "\n s.t.: \n";
    for (unsigned int i = 0; i<getMatrix().rowCount();i++){
        const Vector & row=getMatrix().row(i);
        Vector::Iterator it = row.begin();
        Vector::Iterator end = row.end();
        for(unsigned int j=0; it<end; j++, it++){
            out << (j==0?" ":"+ ") << *it << "*x" << j << " ";
        }
        out << " + y" << i << " = " << m_rhs.at(i) <<"\n";
    }
    out << "\n";
    unsigned int columnCount = getColumnCount();
    for(unsigned int i = 0; i<getVariables().size();i++){
        const Variable & variable = getVariable(i);
        if(i<columnCount){
            out << variable.getLowerBound() << " x" << i << " " << variable.getUpperBound()<<"\n";
        } else {
            out << variable.getLowerBound() << " y" << i-columnCount << " " << variable.getUpperBound()<<"\n";
        }
    }
}
