/**
 * @file pfibasis.cpp
 */

#include <string>

#include <simplex/pfibasis.h>
#include <simplex/simplex.h>
#include <simplex/simplexparameterhandler.h>

#include <utils/thirdparty/prettyprint.h>
#include <utils/architecture.h>

double expDiffSquareSum = 0;
double expDiffSum = 0;
int expCounter = 0;

double aprExpDiffSquareSum = 0;
double aprExpDiffSum = 0;
int aprExpCounter = 0;

thread_local int PfiBasis::m_inversionCount = 0;
thread_local std::vector<ETM>* PfiBasis::m_updates = nullptr;

//TODO This thread_local leads to memory error
thread_local std::vector<int>* PfiBasis::m_updateHelper = nullptr;

PfiBasis::PfiBasis() :
    Basis(),
    m_nontriangularMethod(getNontriangularMethod(SimplexParameterHandler::getInstance().getStringParameterValue("Factorization.PFI.nontriangular_method"))),
    //m_nontriangularMethod(static_cast<NONTRIANGULAR_METHOD>
    //                      (SimplexParameterHandler::getInstance().getStringParameterValue("Factorization.PFI.nontriangular_method"))),
    m_nontriangularPivotRule(getNontriangularPivotRule(
                                 SimplexParameterHandler::getInstance().getStringParameterValue("Factorization.PFI.nontriangular_pivot_rule"))),
    //m_nontriangularPivotRule(static_cast<NONTRIANGULAR_PIVOT_RULE>
    //                         (SimplexParameterHandler::getInstance().getIntegerParameterValue("Factorization.PFI.nontriangular_pivot_rule"))),
    m_threshold(SimplexParameterHandler::getInstance().getDoubleParameterValue("Factorization.pivot_threshold"))
{
    m_basis = new std::vector<ETM>();

    m_transformationCount = 0;
    m_transformationAverage = 0;
    m_mNumAverage = 0;

    m_cColumns = new std::vector<const SparseVector*>();
    m_cPivotIndexes = new std::vector<int>();

    m_mmRows = new std::vector<SparseVector>();
    m_mmRowIndices = new std::vector<int>();
    m_mmColumns = new std::vector<SparseVector>();
    m_mmColumnIndices = new std::vector<int>();
    m_mmGraphOut = new std::vector<std::vector<int> >();
    m_mmGraphUsed = new std::vector<char>();
    m_stack = new std::vector<PathNode>();
    m_mmBlocks = new std::vector<int>();
    m_rowSwapHash = new std::vector<int>();
    m_columnSwapHash = new std::vector<int>();
    m_columnSwapLog = new std::vector<int>();
}

PfiBasis::~PfiBasis() {
    //TODO set nullptr to deleted pointers (also initialization)
    delete m_mmRows;
    m_mmRows = nullptr;
    delete m_mmRowIndices;
    m_mmRowIndices = nullptr;
    delete m_mmColumns;
    m_mmColumns = nullptr;
    delete m_mmColumnIndices;
    m_mmColumnIndices = nullptr;
    delete m_mmGraphOut;
    m_mmGraphOut = nullptr;
    delete m_mmGraphUsed;
    m_mmGraphUsed = nullptr;
    delete m_stack;
    m_stack = nullptr;
    delete m_mmBlocks;
    m_mmBlocks = nullptr;
    delete m_rowSwapHash;
    m_rowSwapHash = nullptr;
    delete m_columnSwapHash;
    m_columnSwapHash = nullptr;
    delete m_columnSwapLog;
    m_columnSwapLog = nullptr;
    delete m_cColumns;
    m_cColumns = nullptr;
    delete m_cPivotIndexes;
    m_cPivotIndexes = nullptr;
    for (std::vector<ETM>::iterator iter = m_basis->begin(); iter < m_basis->end(); ++iter) {
        delete iter->eta;
    }
    delete m_basis;
    m_basis = nullptr;
}


void PfiBasis::registerThread() {
    if(m_updates == nullptr){
        m_updates = new std::vector<ETM>();
    }

    if(m_updateHelper == nullptr){
        m_updateHelper = new std::vector<int>();
    }
}

void PfiBasis::releaseThread() {
    for (std::vector<ETM>::iterator iter = m_updates->begin(); iter < m_updates->end(); ++iter) {
        delete iter->eta;
    }
    delete m_updates;
    m_updates = nullptr;

    delete m_updateHelper;
    m_updateHelper = nullptr;
}

void PfiBasis::clearUpdates() {
    if(m_updates == nullptr){
        throw PanOptException("Thread is not registered to manage basis updates!");
    }
    for (std::vector<ETM>::iterator iter = m_updates->begin(); iter < m_updates->end(); ++iter) {
        delete iter->eta;
    }
    m_updates->clear();
}

void PfiBasis::copyBasis(bool buildIndexLists) {
    DEVINFO(D::PFIMAKER, "Copy the basis");
    unsigned int columnCount = m_model->getColumnCount();
    unsigned int rowCount = m_model->getRowCount();

    //Reinit data structures
    m_columnCounts.clear();
    m_columnCountIndexList.init(0,0);
    m_rowCounts.clear();
    m_rowCountIndexList.init(0,0);

    //Containers to be resized directly
    //NEW//
    m_basicColumns.clear();
    m_basicColumns.reserve(rowCount);
    m_basicColumnCopies.resize(rowCount, NULL);
    m_rowNonzeroIndices.clear();
    m_rowNonzeroIndices.resize(rowCount);
    m_basicColumnIndices.clear();
    m_basicColumnIndices.reserve(rowCount);
    //NEW//

    m_columnCounts.reserve(rowCount);
    m_rowCounts.resize(rowCount, 0);
    m_basisNewHead.resize(rowCount, -1);

#ifndef NDEBUG
    std::vector<char> headChecker(rowCount + columnCount, 0);
    for (std::vector<int>::iterator it = m_basisHead->begin(); it < m_basisHead->end(); ++it) {
        if (headChecker[*it] == 0) {
            headChecker[*it] = 1;
        } else {
            LPWARNING("Duplicate index in basis head: " << *it);
            throw PanOptException("Duplicate index in the basis head!");
        }
    }
#endif

    //Copy the active submatrix
    for (std::vector<int>::iterator it = m_basisHead->begin(); it < m_basisHead->end(); ++it) {
        if (*it >= (int) columnCount) {
            //Ignore logical columns from the inverse
            m_rowCounts[*it-columnCount] = -1;
            m_basisNewHead[*it-columnCount] = *it;
        } else {
            //The submatrix is the active submatrix needed for inversion
            m_basicColumns.push_back(&(m_model->getMatrix().column(*it)));
            m_basicColumnIndices.push_back(*it);
            //IGNORE//
            m_columnCounts.push_back(m_basicColumns.back()->nonZeros());
            m_basisNonzeros += m_basicColumns.back()->nonZeros();
        }
    }
    //Set up row counts, column counts (r_i, c_i) and the corresponding row lists
    int maxRowCount = 0;
    int maxColumnCount = 0;
    for (std::vector<const SparseVector*>::iterator it = m_basicColumns.begin(); it < m_basicColumns.end(); ++it) {
        int columnIndex = it - m_basicColumns.begin();
        SparseVector::NonzeroIterator vectorIt = (*it)->beginNonzero();
        SparseVector::NonzeroIterator vectorItEnd = (*it)->endNonzero();
        for (; vectorIt < vectorItEnd; ++vectorIt) {
            int rowIndex = vectorIt.getIndex();
            if(m_rowCounts[rowIndex] != -1){
                m_rowCounts[rowIndex]++;
                m_rowNonzeroIndices[rowIndex].push_back(columnIndex);
            } else {
                m_columnCounts[columnIndex]--;
            }
        }
        if (maxColumnCount < m_columnCounts[columnIndex]) {
            maxColumnCount = m_columnCounts[columnIndex];
        }
    }

    if(buildIndexLists){
        for (std::vector<int>::iterator it = m_rowCounts.begin(); it < m_rowCounts.end(); ++it) {
            if (maxRowCount < (int) *it) {
                maxRowCount = *it;
            }
        }
        buildRowCountIndexLists(rowCount, maxRowCount);
        buildColumnCountIndexLists(m_basicColumns.size(), maxColumnCount);
    }
}


void PfiBasis::buildRowCountIndexLists(int size, int maxRowCount) {
    m_rowCountIndexList.init(size, maxRowCount+1);
    if(maxRowCount>0){
        for (std::vector<int>::iterator it = m_rowCounts.begin(); it < m_rowCounts.end(); ++it) {
            if (*it >= 0) {
                m_rowCountIndexList.insert(*it, it - m_rowCounts.begin());
            }
        }
    }
}

void PfiBasis::buildColumnCountIndexLists(int size, int maxColumnCount) {
    m_columnCountIndexList.init(size, maxColumnCount+1);
    if(maxColumnCount>0){
        for (std::vector<int>::iterator it = m_columnCounts.begin(); it < m_columnCounts.end(); ++it) {
            if (*it >= 0) {
                m_columnCountIndexList.insert(*it, it - m_columnCounts.begin());
            }
        }
    }
}

void PfiBasis::invert() {
    m_transformationCount = 0;
    m_inversionCount++;

    //Reset the data structures
    m_cColumns->clear();
    m_cPivotIndexes->clear();

    for (std::vector<ETM>::iterator iter = m_basis->begin(); iter < m_basis->end(); ++iter) {
        delete iter->eta;
    }
    m_basis->clear();
    clearUpdates();

    m_basisNewHead.clear();

    m_basisNonzeros = 0;
    m_inverseNonzeros = 0;

    //Copy the basis for computation
    copyBasis();
    DEVINFO(D::PFIMAKER, "Basis copied");

    //Invert the R, M, C parts separately
    invertR();
    findC();
    invertM();
    invertC();

    //Free the copied columns
    for(unsigned int i=0; i<m_basicColumnCopies.size(); i++){
        if(m_basicColumnCopies[i] != NULL){
            delete m_basicColumnCopies[i];
            m_basicColumnCopies[i] = NULL;
        }
    }

    //Verify the result of the inversion
    checkSingularity();

    //Update the basis head
    setNewHead();

    m_isFresh = true;
    //printStatistics();
    m_transformationAverage += (m_transformationCount - m_transformationAverage) / m_inversionCount;
}

void PfiBasis::append(const SparseVector &vector, int pivotRow, int incoming, Simplex::VARIABLE_STATE outgoingState) {
    //If the alpha vector comes in, then ftran is done already

    if(m_updates == nullptr){
        throw PanOptException("Thread is not registered to manage basis updates!");
    }

    int outgoing = (*m_basisHead)[pivotRow];
    const Variable & outgoingVariable = m_model->getVariable(outgoing);
    if (outgoingState == Simplex::NONBASIC_AT_LB) {
        if(!Numerical::equal(*(m_variableStates->getAttachedData(outgoing)), outgoingVariable.getLowerBound(),1.0e-4)){
            LPERROR("Outgoing variable is rounded to its lower bound! "<<outgoing);
            LPERROR("Current value: " << setw(19) << scientific << setprecision(16) << *(m_variableStates->getAttachedData(outgoing)));
            LPERROR("Lower bound: " << setw(19) << scientific << setprecision(16) << outgoingVariable.getLowerBound());
            LPERROR("Upper bound: " << setw(19) << scientific << setprecision(16) << outgoingVariable.getUpperBound());
            cerr.unsetf(ios_base::floatfield);
        }
        pivot(vector, pivotRow, m_updates);
        m_variableStates->move(outgoing,Simplex::NONBASIC_AT_LB, &(outgoingVariable.getLowerBound()));
    } else if (outgoingState == Simplex::NONBASIC_AT_UB) {
        if(!Numerical::equal(*(m_variableStates->getAttachedData(outgoing)), outgoingVariable.getUpperBound(),1.0e-4)){
            LPERROR("Outgoing variable is rounded to its upper bound! "<<outgoing);
            LPERROR("Current value: " << setw(19) << scientific << setprecision(16) << *(m_variableStates->getAttachedData(outgoing)));
            LPERROR("Lower bound: " << setw(19) << scientific << setprecision(16) << outgoingVariable.getLowerBound());
            LPERROR("Upper bound: " << setw(19) << scientific << setprecision(16) << outgoingVariable.getUpperBound());
            cerr.unsetf(ios_base::floatfield);
        }
        pivot(vector, pivotRow, m_updates);
        m_variableStates->move(outgoing,Simplex::NONBASIC_AT_UB, &(outgoingVariable.getUpperBound()));
    } else if ( outgoingState == Simplex::NONBASIC_FIXED) {
        if(!Numerical::equal(*(m_variableStates->getAttachedData(outgoing)), outgoingVariable.getLowerBound(),1.0e-4)){
            LPERROR("Outgoing fixed variable is rounded to its bound! "<<outgoing);
            LPERROR("Current value: " << setw(19) << scientific << setprecision(16) << *(m_variableStates->getAttachedData(outgoing)));
            LPERROR("Bound: " << setw(19) << scientific << setprecision(16) << outgoingVariable.getLowerBound());
            cerr.unsetf(ios_base::floatfield);
        }
        pivot(vector, pivotRow, m_updates);
        m_variableStates->move(outgoing,Simplex::NONBASIC_FIXED, &(outgoingVariable.getLowerBound()));
    } else {
        LPERROR("Invalid outgoing variable state!");
        cerr.unsetf(ios_base::floatfield);
        throw NumericalException(std::string("Invalid outgoing variable state"));
    }
    (*m_basisHead)[pivotRow] = incoming;
    m_isFresh = false;
}

std::string doubleToHex(double num) {
    union Num {
        double m_d;
        unsigned long long int m_i;
    } n;
    n.m_d = num;
    std::ostringstream str;
    str << std::hex << n.m_i << std::dec;
    return str.str();
}

void PfiBasis::Ftran(DenseVector &vector, FTRAN_MODE mode) const {
    __UNUSED(mode);
#ifndef NDEBUG
    //In debug mode the dimensions of the basis and the given vector v are compared.
    //If the dimension mismatches, then the operation cannot be performed.
    //This can't happen in the "normal" case, so in release mode this check is unnecessary.
    if (vector.length() != m_basisHead->size()) {
        LPERROR("FTRAN failed, vector dimension mismatch! ");
        LPERROR("Dimension of the vector to be transformed: " << vector.length());
        LPERROR("Dimension of the basis: " << m_basisHead->size());
    }
#endif //!NDEBUG
    //The ftran operation.
    Numerical::Double * denseVector;
    denseVector = vector.m_data;

    // 2. lepes: vegigmegyunk minden eta vektoron es elvegezzuk a hozzaadast
    std::vector<ETM>::const_iterator iter = m_basis->begin();
    std::vector<ETM>::const_iterator iterEnd = m_basis->end();

    for (; iter != iterEnd; ++iter) {
        const Numerical::Double pivotValue = denseVector[ iter->index ];
        if (pivotValue == 0.0) {
            continue;
        }
        Numerical::Double * ptrEta = iter->eta->m_data;
        unsigned int * ptrIndex = iter->eta->m_indices;
        const unsigned int * ptrIndexEnd = ptrIndex + iter->eta->m_nonZeros;
        const unsigned int pivotPosition = iter->index;
        while (ptrIndex < ptrIndexEnd) {
            Numerical::Double & originalValue = denseVector[*ptrIndex];
            if (*ptrEta != 0.0) {
                Numerical::Double val;
                if (*ptrIndex != pivotPosition) {
                    val = Numerical::stableAddAbs(originalValue, pivotValue * *ptrEta);
                } else {
                    val = pivotValue * *ptrEta;
                }
                originalValue = val;
            }
            ptrIndex++;
            ptrEta++;
        }
    }

    // 3. lepes: vegigmegyunk minden update eta vektoron es elvegezzuk a hozzaadast
    iter = m_updates->begin();
    iterEnd = m_updates->end();

    for (; iter != iterEnd; ++iter) {
        const Numerical::Double pivotValue = denseVector[ iter->index ];
        if (pivotValue == 0.0) {
            continue;
        }
        Numerical::Double * ptrEta = iter->eta->m_data;
        unsigned int * ptrIndex = iter->eta->m_indices;
        const unsigned int * ptrIndexEnd = ptrIndex + iter->eta->m_nonZeros;
        const unsigned int pivotPosition = iter->index;
        while (ptrIndex < ptrIndexEnd) {
            Numerical::Double & originalValue = denseVector[*ptrIndex];
            if (*ptrEta != 0.0) {
                Numerical::Double val;
                if (*ptrIndex != pivotPosition) {
                    val = Numerical::stableAddAbs(originalValue, pivotValue * *ptrEta);
                } else {
                    val = pivotValue * *ptrEta;
                }
                originalValue = val;
            }
            ptrIndex++;
            ptrEta++;
        }
    }
}

void PfiBasis::Ftran(SparseVector &vector, FTRAN_MODE mode) const {
    __UNUSED(mode);
#ifndef NDEBUG
    //In debug mode the dimensions of the basis and the given vector v are compared.
    //If the dimension mismatches, then the operation cannot be performed.
    //This can't happen in the "normal" case, so in release mode this check is unnecessary.
    if (vector.length() != m_basisHead->size()) {
        LPERROR("FTRAN failed, vector dimension mismatch! ");
        LPERROR("Dimension of the vector to be transformed: " << vector.length());
        LPERROR("Dimension of the basis: " << m_basisHead->size());
    }
#endif //!NDEBUG
    //The ftran operation.

    Numerical::Double * denseVector;

    // 1. lepes: ha kell akkor atvaltjuk dense-re
    vector.scatter();
    denseVector = SparseVector::sm_fullLengthVector;

    // 2. lepes: vegigmegyunk minden eta vektoron es elvegezzuk a hozzaadast
    std::vector<ETM>::const_iterator iter = m_basis->begin();
    std::vector<ETM>::const_iterator iterEnd = m_basis->end();

    for (; iter != iterEnd; ++iter) {
        const Numerical::Double pivotValue = denseVector[ iter->index ];
        if (pivotValue == 0.0) {
            continue;
        }

        Numerical::Double * ptrEta = iter->eta->m_data;
        unsigned int * ptrIndex = iter->eta->m_indices;
        const unsigned int * ptrIndexEnd = ptrIndex + iter->eta->m_length;
        const unsigned int pivotPosition = iter->index;
        while (ptrIndex < ptrIndexEnd) {
            Numerical::Double & originalValue = denseVector[*ptrIndex];
            if (*ptrEta != 0.0) {
                Numerical::Double val;
                if (*ptrIndex != pivotPosition) {
                    val = Numerical::stableAddAbs(originalValue, pivotValue * *ptrEta);
                    if (originalValue == 0.0 && val != 0.0) {
                        vector.m_nonZeros++;
                    } else if (originalValue != 0.0 && val == 0.0) {
                        vector.m_nonZeros--;
                    }
                } else {
                    val = pivotValue * *ptrEta;
                }
                originalValue = val;
            }
            ptrIndex++;
            ptrEta++;
        }
    }

    // 3. lepes: vegigmegyunk minden update eta vektoron es elvegezzuk a hozzaadast
    iter = m_updates->begin();
    iterEnd = m_updates->end();

    for (; iter != iterEnd; ++iter) {
        const Numerical::Double pivotValue = denseVector[ iter->index ];
        if (pivotValue == 0.0) {
            continue;
        }

        Numerical::Double * ptrEta = iter->eta->m_data;
        unsigned int * ptrIndex = iter->eta->m_indices;
        const unsigned int * ptrIndexEnd = ptrIndex + iter->eta->m_length;
        const unsigned int pivotPosition = iter->index;
        while (ptrIndex < ptrIndexEnd) {
            Numerical::Double & originalValue = denseVector[*ptrIndex];
            if (*ptrEta != 0.0) {
                Numerical::Double val;
                if (*ptrIndex != pivotPosition) {
                    val = Numerical::stableAddAbs(originalValue, pivotValue * *ptrEta);
                    if (originalValue == 0.0 && val != 0.0) {
                        vector.m_nonZeros++;
                    } else if (originalValue != 0.0 && val == 0.0) {
                        vector.m_nonZeros--;
                    }
                } else {
                    val = pivotValue * *ptrEta;
                }
                originalValue = val;
            }
            ptrIndex++;
            ptrEta++;
        }
    }

    // 4. lepes: ha kell akkor v-t atvaltani, adatokat elmenteni    Vector::VECTOR_TYPE newType;

    vector.prepareForData(vector.m_nonZeros, vector.m_length);
    Numerical::Double * ptrValue = denseVector;
    const Numerical::Double * ptrValueEnd = denseVector + vector.m_length;
    unsigned int index = 0;
    while (ptrValue < ptrValueEnd) {
        if (*ptrValue != 0.0) {
            vector.newNonZero(*ptrValue, index);
            *ptrValue = 0.0;
        }
        ptrValue++;
        index++;
    }
}

Numerical::Double lostValueAdd(Numerical::Double a, Numerical::Double b) {
    Numerical::Double sum = a + b;
    // levonjuk a nagyobb abszolut erteku elem eredeti erteket
    Numerical::Double lost = sum - (Numerical::fabs(a) > Numerical::fabs(b) ? a : b);
    // ezt kivonjuk a kisebb abszolut erteku elem eredeti ertekebol
    return (Numerical::fabs(a) > Numerical::fabs(b) ? b : a) - lost;
}

void PfiBasis::Btran(DenseVector &vector, BTRAN_MODE mode) const
{
    __UNUSED(mode);

#ifndef NDEBUG
    //In debug mode the dimensions of the basis and the given vector v are compared.
    //If the dimension mismatches, then the operation cannot be performed.
    //This can't happen in the "normal" case, so in release mode this check is unnecessary.
    if (vector.length() != m_basisHead->size()) {
        LPERROR("BTRAN failed, vector dimension mismatch! ");
        LPERROR("Dimension of the vector to be transformed: " << vector.length());
        LPERROR("Dimension of the basis: " << m_basisHead->size());
    }
#endif //!NDEBUG

    //The btran operation.
    Numerical::Double * denseVector;

    denseVector = vector.m_data;

    // 2. perform the dot products on the update vectors
    ETM * iterEnd = m_updates->data() - 1;
    ETM * iter = iterEnd + m_updates->size();

    unsigned int etaIndex = 0;
    for (; iter != iterEnd; iter--, etaIndex++) {

        Numerical::Summarizer summarizer;
        Numerical::Double dotProduct = 0;

        Numerical::Double * ptrValue = iter->eta->m_data;
        unsigned int * ptrIndex = iter->eta->m_indices;
        const unsigned int * ptrIndexEnd = ptrIndex + iter->eta->m_nonZeros;

        while (ptrIndex < ptrIndexEnd) {
            summarizer.add(denseVector[*ptrIndex] * *ptrValue);
            ptrIndex++;
            ptrValue++;
        }

        dotProduct = summarizer.getResult();

        // store the dot product, and update the nonzero counter
        const int pivot = iter->index;
        denseVector[pivot] = dotProduct;
    }

    // 3. perform the dot products on the basic vectors
    iterEnd = m_basis->data() - 1;
    iter = iterEnd + m_basis->size();

    etaIndex = 0;
    for (; iter != iterEnd; iter--, etaIndex++) {
        //unsigned int nonZeros = vector.nonZeros();

        Numerical::Summarizer summarizer;
        Numerical::Double dotProduct = 0;

        Numerical::Double * ptrValue = iter->eta->m_data;
        unsigned int * ptrIndex = iter->eta->m_indices;
        const unsigned int * ptrIndexEnd = ptrIndex + iter->eta->m_nonZeros;

        while (ptrIndex < ptrIndexEnd) {
            summarizer.add(denseVector[*ptrIndex] * *ptrValue);
            ptrIndex++;
            ptrValue++;
        }

        dotProduct = summarizer.getResult();

        // store the dot product, and update the nonzero counter
        const int pivot = iter->index;
        denseVector[pivot] = dotProduct;
    }
}

void PfiBasis::Btran(SparseVector &vector, BTRAN_MODE mode) const
{
    __UNUSED(mode);

#ifndef NDEBUG
    //In debug mode the dimensions of the basis and the given vector v are compared.
    //If the dimension mismatches, then the operation cannot be performed.
    //This can't happen in the "normal" case, so in release mode this check is unnecessary.
    if (vector.length() != m_basisHead->size()) {
        LPERROR("BTRAN failed, vector dimension mismatch! ");
        LPERROR("Dimension of the vector to be transformed: " << vector.length());
        LPERROR("Dimension of the basis: " << m_basisHead->size());
    }
#endif //!NDEBUG

    //The btran operation.
    Numerical::Double * denseVector;

    // 1. convert the input vector to dense form if necessary
    vector.scatter();
    denseVector = SparseVector::sm_fullLengthVector;

    // 2. perform the dot products on the update vectors
    ETM * iterEnd = m_updates->data() - 1;
    ETM * iter = iterEnd + m_updates->size();

    unsigned int etaIndex = 0;
    for (; iter != iterEnd; iter--, etaIndex++) {

        Numerical::Summarizer summarizer;
        Numerical::Double dotProduct = 0;

        //All eta vectors are sparse!
        Numerical::Double * ptrValue = iter->eta->m_data;
        unsigned int * ptrIndex = iter->eta->m_indices;
        const unsigned int * ptrIndexEnd = ptrIndex + iter->eta->m_length;

        while (ptrIndex < ptrIndexEnd) {
            summarizer.add(denseVector[*ptrIndex] * *ptrValue);
            ptrIndex++;
            ptrValue++;
        }

        dotProduct = summarizer.getResult();

        // store the dot product, and update the nonzero counter
        const int pivot = iter->index;
        if (denseVector[pivot] != 0.0 && dotProduct == 0.0) {
            vector.m_nonZeros--;
        }
        if (denseVector[pivot] == 0.0 && dotProduct != 0.0) {
            vector.m_nonZeros++;
        }
        denseVector[pivot] = dotProduct;
    }

    // 3. perform the dot products on the basic vectors
    iterEnd = m_basis->data() - 1;
    iter = iterEnd + m_basis->size();

    etaIndex = 0;
    for (; iter != iterEnd; iter--, etaIndex++) {

        Numerical::Summarizer summarizer;
        Numerical::Double dotProduct = 0;
        Numerical::Double * ptrValue = iter->eta->m_data;
        unsigned int * ptrIndex = iter->eta->m_indices;
        const unsigned int * ptrIndexEnd = ptrIndex + iter->eta->m_length;

        while (ptrIndex < ptrIndexEnd) {
            summarizer.add(denseVector[*ptrIndex] * *ptrValue);
            ptrIndex++;
            ptrValue++;
        }

        dotProduct = summarizer.getResult();

        // store the dot product, and update the nonzero counter
        const int pivot = iter->index;
        if (denseVector[pivot] != 0.0 && dotProduct == 0.0) {
            vector.m_nonZeros--;
        }
        if (denseVector[pivot] == 0.0 && dotProduct != 0.0) {
            vector.m_nonZeros++;
        }
        denseVector[pivot] = dotProduct;
    }

    // 4. store the result in the output vector
    // operating vector and output are the same

    // build the result vector, if the original vector was sparse
    vector.prepareForData(vector.m_nonZeros, vector.m_length);
    Numerical::Double * ptrValue = denseVector;
    const Numerical::Double * ptrValueEnd = denseVector + vector.m_length;
    unsigned int index = 0;
    while (ptrValue < ptrValueEnd) {
        if (*ptrValue != 0.0) {
            vector.newNonZero(*ptrValue, index);
            *ptrValue = 0.0;
        }
        ptrValue++;
        index++;
    }
}

void PfiBasis::updateColumns(unsigned int rowindex, unsigned int columnindex) {
    std::list<int>::iterator it = m_rowNonzeroIndices[rowindex].begin();
    std::list<int>::iterator itend = m_rowNonzeroIndices[rowindex].end();

    m_updateHelper->resize(m_model->getRowCount(), 0);
    for (; it != itend; ++it) {
        if (*it != (int) columnindex && m_columnCounts[*it] > -1) {

            if(m_basicColumnCopies[*it]==NULL){
                m_basicColumnCopies[*it] = new SparseVector(*(m_basicColumns[*it]));
                m_basicColumns[*it] = m_basicColumnCopies[*it];
            }
            m_transformationCount += m_basis->back().eta->nonZeros();

            //Remove everything
            SparseVector::NonzeroIterator columnIt = m_basicColumnCopies[*it]->beginNonzero();
            SparseVector::NonzeroIterator columnItend = m_basicColumnCopies[*it]->endNonzero();
            for (; columnIt < columnItend; ++columnIt) {
                if(columnIt.getIndex() != rowindex && m_rowCounts[columnIt.getIndex()] > -1){
                    (*m_updateHelper)[columnIt.getIndex()]--;
                }
            }

            //            LPINFO("UPDATING COLUMN: "<<*m_basicColumnCopies.at(*it));
            m_basicColumnCopies[*it]->elementaryFtran(*(m_basis->back().eta), m_basis->back().index);
            //            LPINFO("UPDATED COLUMN: "<<*m_basicColumnCopies.at(*it));

            //Add the changes back
            int newColumnCount = 0;
            columnIt = m_basicColumnCopies[*it]->beginNonzero();
            columnItend = m_basicColumnCopies[*it]->endNonzero();
            for (; columnIt < columnItend; ++columnIt) {
                if(columnIt.getIndex() != rowindex && m_rowCounts[columnIt.getIndex()] > -1){
                    (*m_updateHelper)[columnIt.getIndex()]++;
                    newColumnCount++;
                }
            }

            //Update the column count
            m_columnCounts[*it] = newColumnCount;

            std::vector<int>::iterator helperIt = m_updateHelper->begin();
            std::vector<int>::iterator helperItend = m_updateHelper->end();
            for(int columnIndex = 0; helperIt != helperItend; helperIt++, columnIndex++){
                if(*helperIt == -1){
                    *helperIt = 0;
                    m_rowNonzeroIndices[columnIndex].remove(*it);
                    m_rowCounts[columnIndex]--;
                } else if (*helperIt == 1){
                    *helperIt = 0;
                    m_rowNonzeroIndices[columnIndex].push_back(*it);
                    m_rowCounts[columnIndex]++;
                }
            }
        }
    }

}

void PfiBasis::pivot(const SparseVector& column, int pivotRow, std::vector<ETM>* etaFile) {
    ETM newETM;
    newETM.eta = createEta(column, pivotRow);
    newETM.index = pivotRow;
    m_inverseNonzeros += newETM.eta->nonZeros();
    etaFile->emplace_back(newETM);
}

void PfiBasis::invertR() {

    //The upper triangular part is called R part
    DEVINFO(D::PFIMAKER, "Search for the R part and invert it");
    unsigned int rNum = 0;

    if (m_rowCountIndexList.getPartitionCount() > 1) {
        while (m_rowCountIndexList.firstElement(1) != -1 ) {
            int rowindex = m_rowCountIndexList.firstElement(1);
            //This part searches for rows with row count 1 and order them to the upper triangular part
            int columnindex = m_rowNonzeroIndices[rowindex].front();

            const SparseVector *currentColumn = m_basicColumns[columnindex];
            //Invert the chosen R column
            DEVINFO(D::PFIMAKER, "Inverting R column " << columnindex << " with pivot row " << rowindex);

            pivot(*currentColumn, rowindex, m_basis);
            m_basisNewHead[rowindex] = m_basicColumnIndices[columnindex];

            //Update the row lists and row counts
            SparseVector::NonzeroIterator it = currentColumn->beginNonzero();
            SparseVector::NonzeroIterator itend = currentColumn->endNonzero();
            for (; it < itend; ++it) {
                int index = it.getIndex();
                //If the row of the iterated element is still active
                if (m_rowCounts[index] > -1) {
                    m_rowCounts[index]--;
                    m_rowCountIndexList.move(index, m_rowCounts[index]);
                    m_rowNonzeroIndices[index].remove(columnindex);
                }
            }

            //Set the column count to zero to represent that which column has been chosen.
            m_columnCounts[columnindex] = -1;
            m_columnCountIndexList.remove(columnindex);
            m_rowCounts[rowindex] = -1;
            m_rowCountIndexList.remove(rowindex);

            rNum++;
        }
    }

    DEVINFO(D::PFIMAKER, "RPART num: " << rNum);
}

void PfiBasis::findC() {
    //The lower triangular part is called C part
    DEVINFO(D::PFIMAKER, "Search for C part");
    unsigned int cNum = 0;
    if (m_columnCountIndexList.getPartitionCount() > 1) {
        while (m_columnCountIndexList.firstElement(1) != -1) {
            int columnindex = m_columnCountIndexList.firstElement(1);
            //This part searches for rows with row count 1 and order them to the upper triangular part
            int rowindex = -1 ;
            SparseVector::NonzeroIterator it = m_basicColumns[columnindex]->beginNonzero();
            SparseVector::NonzeroIterator itend = m_basicColumns[columnindex]->endNonzero();
            for (; it < itend; ++it) {
                if (m_rowCounts[it.getIndex()] > 0) {
                    rowindex = it.getIndex();
                    break;
                }
            }
            const SparseVector *currentColumn = m_basicColumns[columnindex];

            //Store the chosen columns
            m_cColumns->push_back(currentColumn);
            m_cPivotIndexes->push_back(rowindex);
            m_basisNewHead[rowindex] = m_basicColumnIndices[columnindex];
            DEVINFO(D::PFIMAKER, "Submatrix column " << columnindex << " stored for C part with pivot index " << rowindex);

            //Update the column counts but the columns remains untouched
            std::list<int>::iterator listIt = m_rowNonzeroIndices[rowindex].begin();
            std::list<int>::iterator listItend = m_rowNonzeroIndices[rowindex].end();
            for (; listIt != listItend; listIt++) {
                int index = *listIt;
                if (m_columnCounts[index] > -1) {
                    m_columnCounts[index]--;
                    m_columnCountIndexList.move(index, m_columnCounts[index]);
                }
            }
            m_rowNonzeroIndices[rowindex].clear();
            //Set the row count set to zero to represent that which row has been chosen.
            m_rowCounts[rowindex] = -1;
            m_rowCountIndexList.remove(rowindex);
            m_columnCounts[columnindex] = -1;
            m_columnCountIndexList.remove(columnindex);

            cNum++;
        }
    }

    DEVINFO(D::PFIMAKER, "CPART num: " << cNum);
}

void PfiBasis::invertM() {

    //The middle (non-triangular) part is called M part
    DEVINFO(D::PFIMAKER, "Organize the M part and invert the columns");
    unsigned int mNum = 0;
    switch (m_nontriangularMethod) {
    case SEARCH:{
        //containsOne represents the exit variable
        bool containsOne;
        containsOne = true;
        while (containsOne) {
            containsOne = false;
            for (std::vector<int>::iterator it = m_rowCounts.begin(); it < m_rowCounts.end(); ++it) {
                if (*it > 0) {
                    DEVINFO(D::PFIMAKER, "Choosing M column with rowcount " << *it);
                    int rowindex = it - m_rowCounts.begin();
                    int columnindex = m_rowNonzeroIndices[rowindex].front();
                    const SparseVector* currentColumn = m_basicColumns[columnindex];
                    if (nontriangularCheck(rowindex, currentColumn, -1)) {
                        //Invert the chosen M column
                        DEVINFO(D::PFIMAKER, "Inverting M column " << columnindex << " with pivot row " << rowindex);
                        pivot(*currentColumn, rowindex, m_basis);
                        containsOne = true;
                        m_basisNewHead[rowindex] = m_basicColumnIndices[columnindex];
                        //Update the remaining columns
                        updateColumns(rowindex, columnindex);
                        //Update the row lists and row counts
                        SparseVector::NonzeroIterator vectorIt = currentColumn->beginNonzero();
                        SparseVector::NonzeroIterator vectorItend = currentColumn->endNonzero();
                        for (; vectorIt < vectorItend; ++vectorIt) {
                            if (m_rowCounts[vectorIt.getIndex()] >= 0) {
                                m_rowCounts[vectorIt.getIndex()]--;
//                                m_rowNonzeroIndices[vectorIt.getIndex()].remove(columnindex);
                            }
                        }
                        //Update the column counts too
                        std::list<int>::iterator listIt = m_rowNonzeroIndices[rowindex].begin();
                        std::list<int>::iterator listItend = m_rowNonzeroIndices[rowindex].end();
                        for (; listIt != listItend; ++listIt) {
                            int index = *listIt;
                            if (m_columnCounts[index] > -1) {
                                m_columnCounts[index]--;
                            }
                        }
                        //Set the column and row count to zero to represent that which column and row has been chosen.
                        m_rowCounts[rowindex] = -1;
                        m_columnCounts[columnindex] = -1;
                    } else {
                        LPWARNING("Non-triangular pivot position is numerically unstable, ignoring column:" << columnindex << ")");
                        //Update the row lists and row counts
                        SparseVector::NonzeroIterator it = currentColumn->beginNonzero();
                        SparseVector::NonzeroIterator itend = currentColumn->endNonzero();
                        for (; it < itend; ++it) {
                            int index = it.getIndex();
                            //If the row of the iterated element is still active
                            if (m_rowCounts[index] > -1) {
                                m_rowCounts[index]--;
                                m_rowNonzeroIndices[index].remove(columnindex);
                            }
                        }
                        //Set the column count to zero to represent that which column is unstable.
                        m_columnCounts[columnindex] = -1;
                    }
                    mNum++;
                }
            }
        }
        break;

    }
    case BLOCK_TRIANGULAR:{
        createBlockTriangular();
        if (m_mmBlocks->size() > 0) {
            int currentBlock = -1;
            int currentBlockSize = 0;
            for (std::vector<int>::iterator rowIt = m_rowSwapHash->begin(); rowIt < m_rowSwapHash->end(); rowIt++) {

                if (currentBlockSize == 0) {
                    currentBlock++;
                    currentBlockSize = (*m_mmBlocks)[currentBlock];
                    //LPINFO("Iterating block #"<<currentBlock<<" with block size: "<<currentBlockSize);
                }
                currentBlockSize--;
                //Rowindex: Ki volt eredetileg aki az iteratoradik sor
                int rowindex = (*m_mmRowIndices)[*rowIt];
                //Columnindex: Ki volt eredetileg az iteratoradik oszlop
                int columnindex = (*m_mmColumnIndices)[(*m_columnSwapHash)[rowIt - m_rowSwapHash->begin()]];
                const SparseVector* currentColumn = m_basicColumns[columnindex];
                if (nontriangularCheck(rowindex, currentColumn, currentBlock)) {
                    DEVINFO(D::PFIMAKER, "Inverting M column " << columnindex << " with pivot row " << rowindex);
                    //Invert the chosen M column

                    pivot(*currentColumn, rowindex, m_basis);
                    m_basisNewHead[rowindex] = m_basicColumnIndices[columnindex];
                    //Update the remaining columns
                    updateColumns(rowindex, columnindex);

                    //Update the row lists and row counts
                    SparseVector::NonzeroIterator it = currentColumn->beginNonzero();
                    SparseVector::NonzeroIterator itend = currentColumn->endNonzero();
                    for (; it < itend; ++it) {
                        int index = it.getIndex();
                        //If the row of the iterated element is still active
                        if (m_rowCounts[index] > -1) {
                            m_rowCounts[index]--;
//                            m_rowNonzeroIndices[index].remove(columnindex);
                        }
                    }
                    //Update the column counts too
                    std::list<int>::iterator listIt = m_rowNonzeroIndices[rowindex].begin();
                    std::list<int>::iterator listItend = m_rowNonzeroIndices[rowindex].end();
                    for (; listIt != listItend; listIt++) {
                        int index = *listIt;
                        if (m_columnCounts[index] > -1) {
                            m_columnCounts[index]--;
                        }
                    }
                    m_rowNonzeroIndices[rowindex].clear();
                    //Set the column and row count to zero to represent that which column and row has been chosen.
                    m_rowCounts[rowindex] = -1;
                    m_columnCounts[columnindex] = -1;


                } else {
                    LPWARNING("Non-triangular pivot position is numerically unstable, ignoring column:" << columnindex << ")");
                    //Update the row lists and row counts
                    SparseVector::NonzeroIterator it = currentColumn->beginNonzero();
                    SparseVector::NonzeroIterator itend = currentColumn->endNonzero();
                    for (; it < itend; ++it) {
                        int index = it.getIndex();
                        //If the row of the iterated element is still active
                        if (m_rowCounts[index] > -1) {
                            m_rowCounts[index]--;
//                            m_rowNonzeroIndices[index].remove(columnindex);
                        }
                    }
                    //Set the column count to zero to represent that which column is unstable.
                    m_columnCounts[columnindex] = -1;
                }
                mNum++;
            }
        }

        break;
    }
    case BLOCK_ORDERED_TRIANGULAR:{
        createBlockTriangular();
        {
            int blockStart = 0;
            for (std::vector<int>::iterator blockIt = m_mmBlocks->begin(); blockIt < m_mmBlocks->end(); blockIt++) {

                int currentBlock = blockIt - m_mmBlocks->begin();
                int currentBlockSize = *blockIt;
                std::multimap<int, int> columnOrders;
                int i;
                for (i = blockStart; i < blockStart + currentBlockSize; i++) {
                    columnOrders.insert(
                                std::make_pair((int) (*m_mmColumns)[i].nonZeros(),
                                               (*m_mmColumnIndices)[(*m_columnSwapHash)[i]]));
                }

                for (std::multimap<int, int>::iterator colIt = columnOrders.begin(); colIt != columnOrders.end(); colIt++) {

                    blockStart++;
                    int rowindex = -1;
                    int columnindex = colIt->second;
                    const SparseVector* currentColumn = m_basicColumns[columnindex];
                    if (nontriangularCheck(rowindex, currentColumn, currentBlock)) {
                        DEVINFO(D::PFIMAKER, "Inverting M column " << columnindex << " with pivot row " << rowindex);
                        //Invert the chosen M column

                        pivot(*currentColumn, rowindex, m_basis);
                        m_basisNewHead[rowindex] = m_basicColumnIndices[columnindex];
                        //Update the remaining columns
                        updateColumns(rowindex, columnindex);

                        //Update the row lists and row counts
                        SparseVector::NonzeroIterator it = currentColumn->beginNonzero();
                        SparseVector::NonzeroIterator itend = currentColumn->endNonzero();
                        for (; it < itend; ++it) {
                            int index = it.getIndex();
                            //If the row of the iterated element is still active
                            if (m_rowCounts[index] > -1) {
                                m_rowCounts[index]--;
                                m_rowNonzeroIndices[index].remove(columnindex);
                            }
                        }
                        //Update the column counts too
                        std::list<int>::iterator listIt = m_rowNonzeroIndices[rowindex].begin();
                        std::list<int>::iterator listItend = m_rowNonzeroIndices[rowindex].end();
                        for (; listIt != listItend; listIt++) {
                            int index = *listIt;
                            if (m_columnCounts[index] > -1) {
                                m_columnCounts[index]--;
                            }
                        }
                        m_rowNonzeroIndices[rowindex].clear();
                        //Set the column and row count to zero to represent that which column and row has been chosen.
                        m_rowCounts[rowindex] = -1;
                        m_columnCounts[columnindex] = -1;

                    } else {
                        LPWARNING("Non-triangular pivot position is numerically unstable, ignoring column:" << columnindex << ")");
                        //Update the row lists and row counts
                        SparseVector::NonzeroIterator it = currentColumn->beginNonzero();
                        SparseVector::NonzeroIterator itend = currentColumn->endNonzero();
                        for (; it < itend; ++it) {
                            int index = it.getIndex();
                            //If the row of the iterated element is still active
                            if (m_rowCounts[index] > -1) {
                                m_rowCounts[index]--;
                                m_rowNonzeroIndices[index].remove(columnindex);
                            }
                        }
                        //Set the column count to zero to represent that which column is unstable.
                        m_columnCounts[columnindex] = -1;
                    }
                    mNum++;

                }
            }
        }

        break;
    }
    default:
        LPWARNING("No triangulatrization method defined!")
                ;
        break;
    }
    DEVINFO(D::PFIMAKER, "MPART num: " << mNum);

}

void PfiBasis::invertC() {
    //The lower triangular part is called C part
    DEVINFO(D::PFIMAKER, "Invert the C part");
    for (std::vector<const SparseVector*>::reverse_iterator it = m_cColumns->rbegin(); it < m_cColumns->rend(); ++it) {
        DEVINFO(D::PFIMAKER, "Inverting C column " << m_cColumns->rend() - 1 - it <<
                " with pivot row " << (*m_cPivotIndexes)[m_cColumns->rend() - 1 - it]);
        pivot(*(*it), (*m_cPivotIndexes)[m_cColumns->rend() - 1 - it], m_basis);
    }
    return;
}

void PfiBasis::buildMM() {
    //Reset the data structures used for processing the non-triangular part
    m_mmRowIndices->clear();
    m_mmRows->clear();
    m_mmColumnIndices->clear();

    m_mmColumns->clear();
    m_rowSwapHash->clear();
    m_columnSwapHash->clear();
    m_columnSwapLog->clear();
    m_mmGraphOut->clear();
    m_mmGraphUsed->clear();
    m_stack->clear();
    m_mmBlocks->clear();

    //Collect the indices of the MM part
    //TODO: Ne foglaljuk ujra mindig ugyanakkora kell
    std::vector<int> rowMemory = std::vector<int>(m_rowCounts.size(), -1);
    for (std::vector<int>::iterator it = m_rowCounts.begin(); it < m_rowCounts.end(); ++it) {
        if (*it >= 0) {
            m_mmRowIndices->push_back(it - m_rowCounts.begin());
            rowMemory[it - m_rowCounts.begin()] = m_mmRowIndices->size() - 1;
        }
    }

    for (std::vector<int>::iterator it = m_columnCounts.begin(); it < m_columnCounts.end(); ++it) {
        if (*it >= 0) {
            m_mmColumnIndices->push_back(it - m_columnCounts.begin());
        }
    }
    if (m_mmRowIndices->size() != m_mmColumnIndices->size()) {
        LPERROR("Non-triangular part inversion failed!");
        LPERROR("The numbers of rows and columns with nonnegative counters are different!");
        return;
    }
    int mmSize = m_mmRowIndices->size();
    //Compute the average size
    m_mNumAverage += (mmSize - m_mNumAverage) / m_inversionCount;

    m_mmRows->resize(mmSize, SparseVector(mmSize));
    m_mmColumns->resize(mmSize, SparseVector(mmSize));
    m_rowSwapHash->resize(mmSize);
    m_columnSwapHash->resize(mmSize);
    m_columnSwapLog->resize(mmSize);

    for (int i = 0; i < mmSize; i++) {
        (*m_rowSwapHash)[i] = i;
        (*m_columnSwapHash)[i] = i;
        (*m_columnSwapLog)[i] = i;
        //     m_mmRows->at(i).setSparsityRatio(0.0);
        //   m_mmColumns->at(i).setSparsityRatio(0.0);
    }
    for (std::vector<int>::iterator it = m_mmColumnIndices->begin(); it < m_mmColumnIndices->end(); ++it) {
        SparseVector::NonzeroIterator vectorIt = m_basicColumns[*it]->beginNonzero();
        SparseVector::NonzeroIterator vectorItend = m_basicColumns[*it]->endNonzero();
        for (; vectorIt < vectorItend; ++vectorIt) {
            if (rowMemory[vectorIt.getIndex()] != -1) {
                int rowIndex = rowMemory[vectorIt.getIndex()];
                int columnIndex = it - m_mmColumnIndices->begin();
                //TODO: setnewnonzero -> preparefordata
                (*m_mmRows)[rowIndex].setNewNonzero(columnIndex, *vectorIt);
                (*m_mmColumns)[columnIndex].setNewNonzero(rowIndex, *vectorIt);
            }
        }
    }
    DEVINFO(D::PFIMAKER, "Nonzero pattern in MM built");
}

void PfiBasis::findTransversal() {
    DEVINFO(D::PFIMAKER, "Searching for transversal form");
    for (int i = 0; i < (int) m_rowSwapHash->size(); i++) {
        if ((*m_mmRows)[i].at(i) == 0) {
            //Find some with bigger index
            bool nextRow = false;
            DEVINFO(D::PFIMAKER, "Searching for diagonal nonzero at row " << i);
            SparseVector::NonzeroIterator vectorIt = (*m_mmColumns)[i].beginNonzero();
            SparseVector::NonzeroIterator vectorItend = (*m_mmColumns)[i].endNonzero();
            for (; vectorIt < vectorItend; ++vectorIt) {
                if ((int) vectorIt.getIndex() > i) {
                    nextRow = true;
                    DEVINFO(D::PFIMAKER, "Nonzero found below index " << i);
                    swapRows(vectorIt.getIndex(), i);
                    break;
                } else if ((*m_mmRows)[vectorIt.getIndex()].at(vectorIt.getIndex()) == 0) {
                    nextRow = true;
                    DEVINFO(D::PFIMAKER, "Nonzero found in singular row below index " << i);
                    swapRows(vectorIt.getIndex(), i);
                    break;
                }
            }
            if (!nextRow) {
                DEVINFO(D::PFIMAKER, "Nonzero not found, search columns");
                std::vector<int> searchedRows;
                vectorIt = (*m_mmColumns)[i].beginNonzero();
                vectorItend = (*m_mmColumns)[i].endNonzero();
                for (; vectorIt < vectorItend; ++vectorIt) {
                    int searchResult;
                    searchedRows.push_back(vectorIt.getIndex());
                    searchResult = searchColumn(vectorIt.getIndex(), i, searchedRows);

                    DEVINFO(D::PFIMAKER, "search result: " << searchResult);
                    if (searchResult > -1) {
                        if (searchResult != i) {
                            swapRows(searchResult, i);
                        }
                        break;
                    }
                }
            }
        }
    }
#ifndef NDEBUG
    DEVINFO(D::PFIMAKER, "Nonzero pattern with transversal ");
    //    printMM();
#endif //!NDEBUG
}

void PfiBasis::swapRows(int rowIndex1, int rowIndex2) {
    DEVINFO(D::PFIMAKER, "Swapping row " << rowIndex1 << " and " << rowIndex2);

    int rowHashIndex1 = (*m_rowSwapHash)[rowIndex1];
    int rowHashIndex2 = (*m_rowSwapHash)[rowIndex2];
    //TODO: gyorsitani
    SparseVector row1 = (*m_mmRows)[rowIndex1];
    SparseVector row2 = (*m_mmRows)[rowIndex2];

    for (SparseVector::NonzeroIterator vectorIt = row1.beginNonzero(); vectorIt < row1.endNonzero(); ++vectorIt) {
        (*m_mmColumns)[vectorIt.getIndex()].set(rowIndex1, 0);
    }
    for (SparseVector::NonzeroIterator vectorIt = row2.beginNonzero(); vectorIt < row2.endNonzero(); ++vectorIt) {
        (*m_mmColumns)[vectorIt.getIndex()].set(rowIndex2, 0);
    }
    for (SparseVector::NonzeroIterator vectorIt = row1.beginNonzero(); vectorIt < row1.endNonzero(); ++vectorIt) {
        (*m_mmColumns)[vectorIt.getIndex()].set(rowIndex2, *vectorIt);
    }
    for (SparseVector::NonzeroIterator vectorIt = row2.beginNonzero(); vectorIt < row2.endNonzero(); ++vectorIt) {
        (*m_mmColumns)[vectorIt.getIndex()].set(rowIndex1, *vectorIt);
    }
    //TODO: JOCO Swap fuggveny a vektorba
    (*m_mmRows)[rowIndex1] = row2;
    (*m_mmRows)[rowIndex2] = row1;

    (*m_rowSwapHash)[rowIndex1] = rowHashIndex2;
    (*m_rowSwapHash)[rowIndex2] = rowHashIndex1;
    //printSwapHashes();
}

void PfiBasis::swapColumns(int columnIndex1, int columnIndex2) {
    DEVINFO(D::PFIMAKER, "Swapping column " << columnIndex1 << " and " << columnIndex2);

    int columnHashIndex1 = (*m_columnSwapHash)[columnIndex1];
    int columnHashIndex2 = (*m_columnSwapHash)[columnIndex2];
    SparseVector column1 = (*m_mmColumns)[columnIndex1];
    SparseVector column2 = (*m_mmColumns)[columnIndex2];

    for (SparseVector::NonzeroIterator vectorIt = column1.beginNonzero(); vectorIt < column1.endNonzero(); ++vectorIt) {
        (*m_mmRows)[vectorIt.getIndex()].set(columnIndex1, 0);
    }
    for (SparseVector::NonzeroIterator vectorIt = column2.beginNonzero(); vectorIt < column2.endNonzero(); ++vectorIt) {
        (*m_mmRows)[vectorIt.getIndex()].set(columnIndex2, 0);
    }
    for (SparseVector::NonzeroIterator vectorIt = column1.beginNonzero(); vectorIt < column1.endNonzero(); ++vectorIt) {
        (*m_mmRows)[vectorIt.getIndex()].set(columnIndex2, *vectorIt);
    }
    for (SparseVector::NonzeroIterator vectorIt = column2.beginNonzero(); vectorIt < column2.endNonzero(); ++vectorIt) {
        (*m_mmRows)[vectorIt.getIndex()].set(columnIndex1, *vectorIt);
    }
    (*m_mmColumns)[columnIndex1] = column2;
    (*m_mmColumns)[columnIndex2] = column1;

    (*m_columnSwapLog)[columnHashIndex1] = columnIndex2;
    (*m_columnSwapLog)[columnHashIndex2] = columnIndex1;

    (*m_columnSwapHash)[columnIndex1] = columnHashIndex2;
    (*m_columnSwapHash)[columnIndex2] = columnHashIndex1;
    //printSwapHashes();
}

int PfiBasis::searchColumn(int columnIndex, int searchIndex, std::vector<int>& searchedRows) {
    DEVINFO(D::PFIMAKER, "Searching column " << columnIndex << " for nonzero with searchindex " << searchIndex);
    SparseVector::NonzeroIterator it = (*m_mmColumns)[columnIndex].beginNonzero();
    SparseVector::NonzeroIterator itend = (*m_mmColumns)[columnIndex].endNonzero();
    for (; it < itend; ++it) {
        if ((int) it.getIndex() >= searchIndex) {
            DEVINFO(D::PFIMAKER, "Nonzero found below index " << searchIndex);
            int searchResult = it.getIndex();
            swapRows(columnIndex, searchResult);
            return searchResult;
        } else if ((*m_mmRows)[it.getIndex()].at(it.getIndex()) == 0) {
            DEVINFO(D::PFIMAKER, "Nonzero found in singular row below index " << searchIndex);
            int searchResult = it.getIndex();
            swapRows(columnIndex, searchResult);
            return searchResult;
        }
    }
    SparseVector::NonzeroIterator vectorIt = (*m_mmColumns)[columnIndex].beginNonzero();
    SparseVector::NonzeroIterator vectorItend = (*m_mmColumns)[columnIndex].endNonzero();
    for (; vectorIt < vectorItend; ++vectorIt) {
        bool contains = false;
        for (std::vector<int>::iterator it = searchedRows.begin(); it < searchedRows.end(); ++it) {
            if (*it == (int) vectorIt.getIndex()) {
                contains = true;
            }
        }
        if (contains) {
            continue;
        }
        searchedRows.push_back(vectorIt.getIndex());
        int searchResult = searchColumn(vectorIt.getIndex(), searchIndex, searchedRows);
        if (searchResult > -1) {
            swapRows(columnIndex, searchResult);
            return searchResult;
        }
    }
    DEVINFO(D::PFIMAKER, "Searching ended in column " << columnIndex);
    return -1;
}

void PfiBasis::createGraph() {
    int mmSize = m_mmRows->size();
    m_mmGraphOut->resize(mmSize, std::vector<int>());
    m_mmGraphUsed->resize(mmSize, 0);
    for (int i=0; i < mmSize; i++) {
        (*m_mmGraphOut)[i].reserve(16);
    }
    auto it = m_mmRows->begin();
    auto itend = m_mmRows->end();
    for (int i=0 ; it < itend; ++it, i++) {
        SparseVector::NonzeroIterator vectorIt = it->beginNonzero();
        SparseVector::NonzeroIterator vectorItend = it->endNonzero();
        for (; vectorIt < vectorItend; ++vectorIt) {
            if ((int) vectorIt.getIndex() != it - m_mmRows->begin()) {
                (*m_mmGraphOut)[it - m_mmRows->begin()].push_back(vectorIt.getIndex());
            }
        }
    }
}

void PfiBasis::tarjan() {
    DEVINFO(D::PFIMAKER, "Tarjan begin");
    for (std::vector<char>::iterator it = m_mmGraphUsed->begin(); it < m_mmGraphUsed->end(); ++it) {
        DEVINFO(D::PFIMAKER, "Tarjan step" << it - m_mmGraphUsed->begin());
        if (*it == 0) {
            DEVINFO(D::PFIMAKER, "Tarjan step active" << it - m_mmGraphUsed->begin());
            PathNode node;
            node.index = it - m_mmGraphUsed->begin();
            node.lowest = node.index;
            node.nextEdge = 0;
            m_stack->push_back(node);
            *it = 1;
            searchNode();
        }
    }
}

int PfiBasis::searchNode() {
    //TODO: std::stack?
    PathNode currentNode = m_stack->back();
    int stackPosition = m_stack->size() - 1;
    int nextLowest = -1;
    DEVINFO(D::PFIMAKER, "Searching node " << currentNode.index);
    const std::vector<int> & outGoing = (*m_mmGraphOut)[currentNode.index];
    //Go the outgoing edge
    while ((int) outGoing.size() > currentNode.nextEdge) {
        //If the pointed node is still active (it has incoming edges)
        if ((*m_mmGraphUsed)[outGoing[currentNode.nextEdge]] != -1) {
            int next = outGoing[currentNode.nextEdge];
            DEVINFO(D::PFIMAKER, "Searching edge " << next);
            //If the node is available (new node), create it on the stack
            if((*m_mmGraphUsed)[next] == 0) {
                PathNode node;
                node.index = next;
                node.lowest = next;
                node.nextEdge = 0;
                m_stack->push_back(node);
                (*m_mmGraphUsed)[node.index] = 1;
                nextLowest = searchNode();
                if (nextLowest != -1) {
                    for (std::vector<PathNode>::iterator it = m_stack->begin(); it < m_stack->end(); ++it) {
                        if (it->index == currentNode.lowest) {
                            break;
                        }
                        if (it->index == nextLowest) {
                            currentNode.lowest = nextLowest;
                            (*m_stack)[stackPosition].lowest = nextLowest;
                            break;
                        }
                    }
                }
            } else {
                //The pointed node is in the stack
                for (std::vector<PathNode>::iterator it = m_stack->begin(); it < m_stack->end(); ++it) {
                    if (it->index == currentNode.lowest) {
                        break;
                    }
                    if (it->index == next) {
                        DEVINFO(D::PFIMAKER, "Setting current lowest to next " << next);
                        currentNode.lowest = it->lowest;
                        (*m_stack)[stackPosition].lowest = it->lowest;
                        break;
                    }
                }
            }
        }
        currentNode.nextEdge++;
        (*m_stack)[stackPosition].nextEdge++;
    }

    if (currentNode.index == currentNode.lowest) {
        DEVINFO(D::PFIMAKER, "Creating block #" << m_mmBlocks->size());
        int allBlocks = 0;
        for (std::vector<int>::iterator it = m_mmBlocks->begin(); it < m_mmBlocks->end(); ++it) {
            allBlocks += *it;
        }
#ifndef NDEBUG
//                        printStack();
#endif //!NDEBUG
        m_mmBlocks->push_back(0);
        int lastIndex;
        do {
            lastIndex = m_stack->back().index;
            (*m_mmGraphUsed)[lastIndex] = -1;
            DEVINFO(D::PFIMAKER, "Last index in the stack is " << lastIndex);
            swapRows((*m_columnSwapLog)[lastIndex], allBlocks);
            swapColumns((*m_columnSwapLog)[lastIndex], allBlocks);
            m_mmBlocks->back()++;
            allBlocks++;
            DEVINFO(D::PFIMAKER, "Block " << m_mmBlocks->size() - 1 << " now contains node " << lastIndex);
            m_stack->pop_back();
        } while (lastIndex != currentNode.index);
    } else {
        DEVINFO(D::PFIMAKER, "Node #" << currentNode.index << " is returning with " << currentNode.lowest);
        return currentNode.lowest;
    }
    return -1;
}

void PfiBasis::createBlockTriangular() {
    buildMM();
    findTransversal();
    createGraph();
    tarjan();

#ifndef NDEBUG
    //    printMM();
    //    printBlocks();
#endif //!NDEBUG
}

bool PfiBasis::nontriangularCheck(int& rowindex, const SparseVector* currentColumn, int blockNum) {
    std::vector<int> activeRows;
    std::vector<int> goodRows;
    Numerical::Double nontriangularMax = 0;
    int previousBlocks = 0;
    switch (m_nontriangularPivotRule) {
    case NONE:
        if (currentColumn->at(rowindex) == 0) {
            rowindex = -1;
            return false;
        } else {
            return true;
        }
        break;
    case THRESHOLD:
        //Ha szamoltunk blokkokat
        if (blockNum != -1) {
            //Megnezzuk hogy korabbi blokkok mennyi sort & oszlopot foglalnak
            for (int i = 0; i < blockNum; i++) {
                previousBlocks += (*m_mmBlocks)[i];
            }
            //            LPINFO("previousBlocks: "<<previousBlocks);
            //Activerows-ba kigyujtjuk a blokkhoz tartozo nemnullakat
            SparseVector::NonzeroIterator it = currentColumn->beginNonzero();
            SparseVector::NonzeroIterator itend = currentColumn->endNonzero();
            for (; it < itend; ++it) {
                if (m_rowCounts[it.getIndex()] > -1) {
                    //Ha MM-beli nemnullat talaltunk
                    for (int i = previousBlocks; i < previousBlocks + (*m_mmBlocks)[blockNum]; i++) {
                        //Ha a nemnulla a blokkon belul van
                        if ((int) it.getIndex() == (*m_mmRowIndices)[(*m_rowSwapHash)[i]]) {
                            //TODO: Ezt hatekonyabban is lehetne
                            activeRows.push_back(it.getIndex());
                            break;
                        }
                    }
                }
            }
        }
        //Ha nem szamoltunk blokkokat
        else {
            SparseVector::NonzeroIterator it = currentColumn->beginNonzero();
            SparseVector::NonzeroIterator itend = currentColumn->endNonzero();
            for (; it < itend; ++it) {
                if (m_rowCounts[it.getIndex()] > -1) {
                    activeRows.push_back(it.getIndex());
                }
            }
        }
        //Megnezzuk mi a legnagyobb elem
        //TODO Ezt az elozo lepes is ki tudja szamolni
        for (std::vector<int>::iterator it = activeRows.begin(); it < activeRows.end(); ++it) {
            if (currentColumn->at(*it) > 0) {
                if (currentColumn->at(*it) > nontriangularMax) {
                    nontriangularMax = currentColumn->at(*it);
                }
            } else {
                if (-(currentColumn->at(*it)) > nontriangularMax) {
                    nontriangularMax = -(currentColumn->at(*it));
                }
            }
        }

        //Levalogatjuk a thresholdnak megfeleloket
        for (std::vector<int>::iterator it = activeRows.begin(); it < activeRows.end(); ++it) {
            if (currentColumn->at(*it) > 0) {
                if (currentColumn->at(*it) > m_threshold * nontriangularMax) {
                    goodRows.push_back(*it);
                }
            } else {
                if (-(currentColumn->at(*it)) > m_threshold * nontriangularMax) {
                    goodRows.push_back(*it);
                }
            }
        }
        //Choose the sparsest row among the candidates
        if (goodRows.size() > 0) {
            rowindex = goodRows[0];
            for (std::vector<int>::iterator it = goodRows.begin(); it < goodRows.end(); ++it) {
                if (m_rowCounts[*it] < m_rowCounts[rowindex]) {
                    rowindex = *it;
                }
            }
            //			LPINFO("chosen row: "<<rowindex<<" with value: "<<currentColumn->at(rowindex));
            return true;
        } else {
            rowindex = -1;
            return false;
        }
        break;
    default:
        return true;
    }
    return true;
}

void PfiBasis::checkSingularity() {
    DEVINFO(D::PFIMAKER, "Checking singularity");
    int singularity = 0;
    for (std::vector<int>::iterator it = m_basisNewHead.begin(); it < m_basisNewHead.end(); ++it) {
        if (*it == -1) {
            DEVINFO(D::PFIMAKER, "Given basis column " << it - m_basisNewHead.begin() << " is singular, replacing with unit vector");
            *it = it - m_basisNewHead.begin() + m_model->getColumnCount();
            singularity++;
        }
    }
    if (singularity != 0) {
        m_singularityCounter += singularity;
        LPWARNING("The given basis is singular, the measure of singularity is " << singularity);
        throw NumericalException("The basis is singular.");
    }
}

void PfiBasis::printCounts() const {
#ifndef NDEBUG
    DEVINFO(D::PFIMAKER, "Row counts: ");
    for (std::vector<int>::const_iterator it = m_rowCounts.begin(); it < m_rowCounts.end(); ++it) {
        DEVINFO(D::PFIMAKER, "Row " << it - m_rowCounts.begin() << " has " << *it << " nonzeros");
    }
    DEVINFO(D::PFIMAKER, "Column counts: ");
    for (std::vector<int>::const_iterator it = m_columnCounts.begin(); it < m_columnCounts.end(); ++it) {
        DEVINFO(D::PFIMAKER, "Column " << it - m_columnCounts.begin() << " has " << *it << " nonzeros");
    }
#endif //!NDEBUG
    return;
}

void PfiBasis::printMM() const {
#ifndef NDEBUG
    LPWARNING( "MM pattern by rows");
    for (std::vector<SparseVector>::iterator it = m_mmRows->begin(); it < m_mmRows->end(); ++it) {
        std::string s;
        for (int i = 0; i < (int) it->length(); i++) {
            s += Numerical::equals(it->at(i), 0) ? "-" : "X";
        }
        LPWARNING( s);
    }
    //    LPWARNING( "MM pattern by columns");
    //    for (int i = 0; i < (int) m_mmColumns->size(); i++) {
    //        std::string s;
    //        for (std::vector<Vector>::iterator it = m_mmColumns->begin(); it < m_mmColumns->end(); ++it) {
    //            s += Numerical::equals(it->at(i), 0) ? "-" : "X";
    //        }
    //        LPWARNING( s);
    //    }
    //    LPWARNING( "MM pattern with values");
    //    for (std::vector<int>::iterator rowIt = m_rowSwapHash->begin(); rowIt < m_rowSwapHash->end(); rowIt++) {
    //        int rowindex = m_mmRowIndices->at(*rowIt);
    //        std::string s;
    //        for (std::vector<int>::iterator columnIt = m_columnSwapHash->begin(); columnIt < m_columnSwapHash->end(); columnIt++) {
    //            int columnindex = m_mmColumnIndices->at(*columnIt);
    //            s += Numerical::equals(m_basicColumns.at(columnindex)->at(rowindex), 0) ? "-" : "X";
    //        }
    //        LPWARNING( s);
    //    }
    //    LPWARNING( "MM pattern with values without equals");
    //    for (std::vector<int>::iterator rowIt = m_rowSwapHash->begin(); rowIt < m_rowSwapHash->end(); rowIt++) {
    //        int rowindex = m_mmRowIndices->at(*rowIt);
    //        std::string s;
    //        for (std::vector<int>::iterator columnIt = m_columnSwapHash->begin(); columnIt < m_columnSwapHash->end(); columnIt++) {
    //            int columnindex = m_mmColumnIndices->at(*columnIt);
    //            s += m_basicColumns.at(columnindex)->at(rowindex) == 0 ? "-" : "X";
    //        }
    //        LPWARNING( s);
    //    }

#endif //!NDEBUG
}

void PfiBasis::printSwapHashes() const {
#ifndef NDEBUG
    DEVINFO(D::PFIMAKER, "Row swap hash:");
    for (std::vector<int>::iterator it = m_rowSwapHash->begin(); it < m_rowSwapHash->end(); ++it) {
        DEVINFO(D::PFIMAKER, "    " << *it);
    }
    DEVINFO(D::PFIMAKER, "Column swap hash:");
    for (std::vector<int>::iterator it = m_columnSwapHash->begin(); it < m_columnSwapHash->end(); ++it) {
        DEVINFO(D::PFIMAKER, "    " << *it);
    }
#endif //!NDEBUG
}

void PfiBasis::printGraph() const {
#ifndef NDEBUG
    //Print the outgoing edges
//    DEVINFO(D::PFIMAKER, "Outgoing graph edges");
//    for (std::vector<std::vector<int> >::iterator it = m_mmGraphOut->begin(); it < m_mmGraphOut->end(); ++it) {
//        DEVINFO(D::PFIMAKER, "Node " << it - m_mmGraphOut->begin() << ":");
//        for (std::vector<int>::iterator nodeIt = it->begin(); nodeIt < it->end(); nodeIt++) {
//            DEVINFO(D::PFIMAKER, "    " << *nodeIt);
//        }
//    }
//    //Print the incoming edges
//    DEVINFO(D::PFIMAKER, "Incoming graph edges");
//    for (std::vector<std::vector<int> >::iterator it = m_mmGraphIn->begin(); it < m_mmGraphIn->end(); ++it) {
//        DEVINFO(D::PFIMAKER, "Node " << it - m_mmGraphIn->begin() << ":");
//        for (std::vector<int>::iterator nodeIt = it->begin(); nodeIt < it->end(); nodeIt++) {
//            DEVINFO(D::PFIMAKER, "    " << *nodeIt);
//        }
//    }
#endif //!NDEBUG
}

void PfiBasis::printStack() const {
#ifndef NDEBUG
    //Print the stack
    DEVINFO(D::PFIMAKER, "Tarjan algorithm stack (Top --> Bottom):");
    DEVINFO(D::PFIMAKER, "index \tlowest\t nextEdge");
    for (std::vector<PathNode>::reverse_iterator it = m_stack->rbegin(); it < m_stack->rend(); ++it) {
        DEVINFO(D::PFIMAKER, it->index << " \t " << it->lowest << " \t " << it->nextEdge);
    }
#endif //!NDEBUG
}

void PfiBasis::printBlocks() const {
#ifndef NDEBUG
    //Print the outgoing edges
    LPWARNING( "Block sizes:");
    for (std::vector<int>::iterator it = m_mmBlocks->begin(); it < m_mmBlocks->end(); ++it) {
        LPWARNING( "Block #" << it - m_mmBlocks->begin() << ": " << *it);
    }
    //printMM();
#endif //!NDEBUG
}

void PfiBasis::printStatistics() const {
    LPINFO("INVERSION FINISHED");
    LPINFO("Run parameters during the inversion:");
    switch (m_nontriangularMethod) {
    case SEARCH:
        LPINFO("Used non-triangular method: SEARCH")
                ;
        break;
    case BLOCK_TRIANGULAR:
        LPINFO("Used non-triangular method: BLOCK_TRIANGULAR")
                ;
        break;
    case BLOCK_ORDERED_TRIANGULAR:
        LPINFO("Used non-triangular method: BLOCK_ORDERED_TRIANGULAR")
                ;
        break;
    default:
        LPWARNING("No non-triangular method defined!")
                ;
        break;
    }
    switch (m_nontriangularPivotRule) {
    case NONE:
        LPINFO("Used non-triangular pivot rule: NONE")
                ;
        break;
    case THRESHOLD:
        LPINFO("Used non-triangular pivot rule: THRESHOLD")
                ;
        break;
    default:
        LPWARNING("No non-triangular pivot rule defined!")
                ;
        break;
    }
    LPINFO("Nonzero statistics: ");
    LPINFO("Nonzeros in the basis: " << m_basisNonzeros);
    LPINFO("Nonzeros in the inverse: " << m_inverseNonzeros);
    LPINFO("Fill in amount: " << ((Numerical::Double)m_inverseNonzeros / (Numerical::Double)m_basisNonzeros)*100.0 - 100 << "% )");
}

void PfiBasis::printTransformationStatistics() const {
    LPINFO("Inversion count: "<<m_inversionCount);
    //		LPINFO("Transformation count in the last inversion: "<<m_transformationCount);
    LPINFO("Transformation count average: "<<m_transformationAverage);
    LPINFO("Average size of the non-triangular part:"<<m_mNumAverage);
    printStatistics();
}
