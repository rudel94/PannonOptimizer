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
 * @file manualmodelbuilder.cpp
 */

#include <lp/manualmodelbuilder.h>
#include <cstdarg>

ManualModelBuilder::ManualModelBuilder()
{
    m_objectiveConstant = 0.0;
    m_objectiveType = MINIMIZE;
}

/************************************************
 * 
 * Interface for the manual model construction
 * 
 * *********************************************/

void ManualModelBuilder::setName(const std::string & name)
{
    m_name = name;
}

void ManualModelBuilder::setObjectiveFunctionConstant(double value)
{
    m_objectiveConstant = value;
}

void ManualModelBuilder::addVariable(const Variable & variable,
    double costCoefficient,
    unsigned int nonzeros,
    const double * values,
    const unsigned int * indices)
{
    m_variables.push_back(variable);
    m_costVector.push_back(costCoefficient);

    // creating the new column
    std::list<IndexValuePair> newList;
    m_columns.push_back(newList);
    const unsigned int lastIndex = m_columns.size() - 1;
    unsigned int index;
    unsigned int maxIndex = 0;
    for (index = 0; index < nonzeros; index++) {
        IndexValuePair pair = {values[index], indices[index]};
        m_columns[lastIndex].push_back(pair);
        if (maxIndex < indices[index]) {
            maxIndex = indices[index];
        }
    }
    m_nonZerosInColumns.push_back(nonzeros);

    // creating the necessary constraints
    for (index = m_constraints.size(); index <= maxIndex; index++) {
        m_constraints.push_back(Constraint::createGreaterTypeConstraint(0, 0.0));
        m_rows.push_back(newList);
        m_nonZerosInRows.push_back(0);
    }

    // insert the nonzeros into the constraints
    unsigned int variableIndex = m_variables.size() - 1;
    for (index = 0; index < nonzeros; index++) {
        IndexValuePair pair = {values[index], variableIndex};
        unsigned int rowIndex = indices[index];
        m_nonZerosInRows[ rowIndex ]++;
        m_rows[ rowIndex ].push_back(pair);
    }
}

void ManualModelBuilder::addVariable(const Variable & variable,
    double costCoefficient,
    const SparseVector &vector)
{
    m_variables.push_back(variable);
    m_costVector.push_back(costCoefficient);

    // creating the new column
    std::list<IndexValuePair> newList;
    m_columns.push_back(newList);
    const unsigned int lastIndex = m_columns.size() - 1;

    unsigned int maxIndex = 0;
    SparseVector::NonzeroIterator iter = vector.beginNonzero();
    SparseVector::NonzeroIterator iterEnd = vector.endNonzero();
    for (; iter != iterEnd; ++iter) {
        IndexValuePair pair = {Numerical::DoubleToIEEEDouble(*iter), iter.getIndex()};
        m_columns[lastIndex].push_back(pair);
        if (maxIndex < iter.getIndex()) {
            maxIndex = iter.getIndex();
        }
    }
    m_nonZerosInColumns.push_back(vector.nonZeros());

    // creating the necessary constraints
    unsigned int index;
    for (index = m_constraints.size(); index <= maxIndex; index++) {
        m_constraints.push_back(Constraint::createGreaterTypeConstraint(0, 0.0));
        m_rows.push_back(newList);
        m_nonZerosInRows.push_back(0);
    }

    // insert the nonzeros into the constraints
    unsigned int variableIndex = m_variables.size() - 1;
    iter = vector.beginNonzero();
    for (; iter != iterEnd; ++iter) {
        IndexValuePair pair = {Numerical::DoubleToIEEEDouble(*iter), variableIndex};
        unsigned int rowIndex = iter.getIndex();
        m_nonZerosInRows[ rowIndex ]++;
        m_rows[ rowIndex ].push_back(pair);
    }

}

void ManualModelBuilder::addVariable(const Variable & variable,
    double costCoefficient,
    unsigned int nonzeros,
    ...)
{
    va_list arguments;
    va_start(arguments, nonzeros);

    m_variables.push_back(variable);
    m_costVector.push_back(costCoefficient);

    // creating the new column
    std::list<IndexValuePair> newList;
    m_columns.push_back(newList);
    const unsigned int lastIndex = m_columns.size() - 1;
    unsigned int index;
    unsigned int rowIndex;
    double value;
    unsigned int maxIndex = 0;
    for (index = 0; index < nonzeros; index++) {
        double dvalue;
        dvalue = va_arg(arguments, double);
        value = dvalue;
        rowIndex = va_arg(arguments, unsigned int);
        IndexValuePair pair = {value, rowIndex};
        m_columns[lastIndex].push_back(pair);
        if (maxIndex < rowIndex) {
            maxIndex = rowIndex;
        }
    }
    m_nonZerosInColumns.push_back(nonzeros);

    // creating the necessary constraints
    for (index = m_constraints.size(); index <= maxIndex; index++) {
        m_constraints.push_back(Constraint::createGreaterTypeConstraint(0, 0.0));
        m_rows.push_back(newList);
        m_nonZerosInRows.push_back(0);
    }

    // insert the nonzeros into the constraints
    va_start(arguments, nonzeros);
    unsigned int variableIndex = m_variables.size() - 1;
    for (index = 0; index < nonzeros; index++) {
        double dvalue;
        dvalue = va_arg(arguments, double);
        value = dvalue;
        rowIndex = va_arg(arguments, unsigned int);
        IndexValuePair pair = {value, variableIndex};
        m_nonZerosInRows[ rowIndex ]++;
        m_rows[ rowIndex ].push_back(pair);
    }
}

void ManualModelBuilder::setConstraint(unsigned int index, const Constraint & constraint)
{
    m_constraints[index] = constraint;
}

void ManualModelBuilder::addConstraint(const Constraint & constraint,
                                       const double * values,
                                       const unsigned int * indices,
                                       unsigned int nonzeros,
                                       Variable *defaultVariablePtr)
{
    m_constraints.push_back(constraint);

    // creating the new row
    std::list<IndexValuePair> newList;
    m_rows.push_back(newList);
    const unsigned int lastIndex = m_rows.size() - 1;
    unsigned int index;
    unsigned int maxIndex = 0;

    for (index = 0; index < nonzeros; index++) {
        IndexValuePair pair = {values[index], indices[index]};
        m_rows[lastIndex].push_back(pair);
        if (maxIndex < indices[index]) {
            maxIndex = indices[index];
        }
    }
    m_nonZerosInRows.push_back(nonzeros);

    // creating the necessary variables
    for (index = m_variables.size(); index <= maxIndex; index++) {
        if (defaultVariablePtr == nullptr) {
            m_variables.push_back(Variable::createPlusTypeVariable(0, 0.0));
        } else {
            m_variables.push_back( *defaultVariablePtr );
        }
        m_columns.push_back(newList);
        m_nonZerosInColumns.push_back(0);
        m_costVector.push_back(0.0);
    }

    // insert the nonzeros into the variables
    unsigned int constraintIndex = m_constraints.size() - 1;
    for (index = 0; index < nonzeros; index++) {
        IndexValuePair pair = {values[index], constraintIndex};
        unsigned int columnIndex = indices[index];
        m_nonZerosInColumns[ columnIndex ]++;
        m_columns[ columnIndex ].push_back(pair);
    }

}

void ManualModelBuilder::addConstraint(const Constraint & constraint,
    const SparseVector &vector)
{
    m_constraints.push_back(constraint);

    // creating the new row
    std::list<IndexValuePair> newList;
    m_rows.push_back(newList);
    const unsigned int lastIndex = m_rows.size() - 1;
    unsigned int index;
    unsigned int maxIndex = 0;
    unsigned int nonzeros = vector.nonZeros();
    SparseVector::NonzeroIterator iter = vector.beginNonzero();
    SparseVector::NonzeroIterator iterEnd = vector.endNonzero();
    for (; iter != iterEnd; ++iter) {
        IndexValuePair pair = {Numerical::DoubleToIEEEDouble(*iter), iter.getIndex()};
        m_rows[lastIndex].push_back(pair);
        if (maxIndex < iter.getIndex()) {
            maxIndex = iter.getIndex();
        }
    }
    m_nonZerosInRows.push_back(nonzeros);

    // creating the necessary variables
    for (index = m_variables.size(); index <= maxIndex; index++) {
        m_variables.push_back(Variable::createPlusTypeVariable(0, 0.0));
        m_columns.push_back(newList);
        m_nonZerosInColumns.push_back(0);
        m_costVector.push_back(0.0);
    }

    // insert the nonzeros into the variables
    unsigned int constraintIndex = m_constraints.size() - 1;
    iter = vector.beginNonzero();
    for (; iter != iterEnd; ++iter) {
        IndexValuePair pair = {Numerical::DoubleToIEEEDouble(*iter), constraintIndex};
        unsigned int columnIndex = iter.getIndex();
        m_nonZerosInColumns[ columnIndex ]++;
        m_columns[ columnIndex ].push_back(pair);
    }
}

void ManualModelBuilder::addConstraint(const Constraint & constraint,
    unsigned int nonzeros,
    ...)
{
    va_list arguments;
    va_start(arguments, nonzeros);
    m_constraints.push_back(constraint);

    // creating the new row
    std::list<IndexValuePair> newList;
    m_rows.push_back(newList);
    const unsigned int lastIndex = m_rows.size() - 1;
    unsigned int index;
    unsigned int maxIndex = 0;
    unsigned int columnIndex;
    double value;
    for (index = 0; index < nonzeros; index++) {
        double dvalue;
        dvalue = va_arg(arguments, double);
        value = dvalue;
        columnIndex = va_arg(arguments, unsigned int);
        IndexValuePair pair = {value, columnIndex};
        m_rows[lastIndex].push_back(pair);
        if (maxIndex < columnIndex) {
            maxIndex = columnIndex;
        }
    }
    m_nonZerosInRows.push_back(nonzeros);

    // creating the necessary variables
    for (index = m_variables.size(); index <= maxIndex; index++) {
        m_variables.push_back(Variable::createPlusTypeVariable(0, 0.0));
        m_columns.push_back(newList);
        m_nonZerosInColumns.push_back(0);
        m_costVector.push_back(0.0);
    }

    // insert the nonzeros into the variables
    va_start(arguments, nonzeros);
    unsigned int constraintIndex = m_constraints.size() - 1;
    for (index = 0; index < nonzeros; index++) {
        double dvalue;
        dvalue = va_arg(arguments, double);
        value = dvalue;
        columnIndex = va_arg(arguments, unsigned int);
        IndexValuePair pair = {value, constraintIndex};
        m_nonZerosInColumns[ columnIndex ]++;
        m_columns[ columnIndex ].push_back(pair);
    }
}

void ManualModelBuilder::setVariable(unsigned int index, const Variable & variable)
{
    m_variables[index] = variable;
}

void ManualModelBuilder::setCostCoefficient(unsigned int index, double cost)
{
    m_costVector[index] = cost;
}

void ManualModelBuilder::setObjectiveType(OBJECTIVE_TYPE type) {
    m_objectiveType = type;
}

/************************************************
 * 
 * Interface for the Model
 * 
 * *********************************************/

unsigned int ManualModelBuilder::getColumnCount() const
{
    return m_variables.size();
}

unsigned int ManualModelBuilder::getRowCount() const
{
    return m_constraints.size();
}

const Variable & ManualModelBuilder::getVariable(unsigned int index) const
{
    return m_variables[index];
}

const Constraint & ManualModelBuilder::getConstraint(unsigned int index) const
{
    return m_constraints[index];
}

void ManualModelBuilder::buildRow(unsigned int index, SparseVector *rowVector,
    std::vector<unsigned int> *) const
{
    unsigned int dimension = m_variables.size();
    rowVector->prepareForData(m_rows[index].size(), dimension);
    std::list< IndexValuePair >::const_iterator iter = m_rows[index].begin();
    std::list< IndexValuePair >::const_iterator iterEnd = m_rows[index].end();
    for (; iter != iterEnd; ++iter) {
        rowVector->newNonZero(iter->m_value, iter->m_index);
    }
}

void ManualModelBuilder::buildColumn(unsigned int index, SparseVector *columnVector,
    std::vector<unsigned int> *) const
{
    unsigned int dimension = m_constraints.size();
    columnVector->prepareForData(m_columns[index].size(), dimension);
    std::list< IndexValuePair >::const_iterator iter = m_columns[index].begin();
    std::list< IndexValuePair >::const_iterator iterEnd = m_columns[index].end();
    for (; iter != iterEnd; ++iter) {
        columnVector->newNonZero(iter->m_value, iter->m_index);
    }
}

void ManualModelBuilder::buildCostVector(DenseVector *costVector) const
{
    auto iter = m_costVector.begin();
    auto iterEnd = m_costVector.end();
    costVector->reInit(m_costVector.size());
    unsigned int index;
    if(m_objectiveType == MINIMIZE){
        for (index = 0; iter != iterEnd; ++iter, index++) {
            costVector->set( index, *iter);
        }
    } else {
        for (index = 0; iter != iterEnd; ++iter, index++) {
            costVector->set( index, -(*iter));
        }
    }
}

double ManualModelBuilder::getObjectiveConstant() const
{
    if(m_objectiveType == MINIMIZE){
        return m_objectiveConstant;
    } else {
        return -m_objectiveConstant;
    }
}

std::string ManualModelBuilder::getName() const
{
    return m_name;
}

bool ManualModelBuilder::hasRowwiseRepresentation() const {
    return true;
}
    
bool ManualModelBuilder::hasColumnwiseRepresentation() const {
    return true;
}

OBJECTIVE_TYPE ManualModelBuilder::getObjectiveType() const {
    return m_objectiveType;
}
