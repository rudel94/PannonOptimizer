/**
 * @file vector.cpp
 */

#include <debug.h>
#include <linalg/matrix.h>
#include <linalg/vector.h>
//#include <linalg/arch/archlinalg.h>
#include <utils/numerical.h>
#include <utils/heap.h>
#include <linalg/linalgparameterhandler.h>
//#include <simplex/linalg.h>

#include <climits>
#include <cstring>
#include <cmath>

unsigned int ELBOWROOM = LinalgParameterHandler::getInstance().getParameterValue("elbowroom");
Numerical::Double SPARSITY_RATIO = LinalgParameterHandler::getInstance().getParameterValue("sparsity_ratio");
Numerical::Double * Vector::sm_fullLengthVector = 0;
unsigned int Vector::sm_fullLengthVectorLenght = 0;
unsigned int Vector::sm_fullLenghtReferenceCounter = 0;
unsigned long * Vector::sm_countingSortBitVector = 0;
unsigned int Vector::sm_countingSortBitVectorLength = 0;

Vector::Vector(unsigned int dimension)
{
    sm_fullLenghtReferenceCounter++;
    init(dimension);
    CHECK;
}

Vector::Vector(void *, void *, void *)
{
    sm_fullLenghtReferenceCounter++;
    m_data = 0;
    m_index = 0;
    m_size = 0;
    m_nonZeros = 0;
    m_capacity = 0;
    m_dimension = 0;
    m_sorted = true;
    m_sparsityRatio = SPARSITY_RATIO;
    m_sparsityThreshold = (unsigned int) Numerical::round(m_dimension * m_sparsityRatio);
    //CHECK;
}

void Vector::reInit(unsigned int dimension)
{
    freeData(m_data);
    if (m_vectorType == SPARSE_VECTOR) {
        freeIndex(m_index);
    }
    init(dimension);
    CHECK;
}

Numerical::Double * Vector::allocateData(unsigned int capacity)
{
    if (capacity == 0) {
        return 0;
    }
    return new Numerical::Double[capacity];
}

unsigned int * Vector::allocateIndex(unsigned int capacity)
{
    if (capacity == 0) {
        return 0;
    }
    return new unsigned int[capacity];
}

void Vector::freeData(Numerical::Double * & data)
{
    delete [] data;
    data = 0;
}

void Vector::freeIndex(unsigned int * & index)
{
    delete [] index;
    index = 0;
}

void Vector::init(unsigned int dimension)
{
    m_size = 0;
    m_data = 0;
    m_dataEnd = 0;
    m_index = 0;
    m_nonZeros = 0;
    m_capacity = ELBOWROOM; //capacity;
    m_sparsityRatio = SPARSITY_RATIO;
    m_dimension = dimension;
    m_sorted = true;
    m_sparsityThreshold = (unsigned int) Numerical::round(m_dimension * m_sparsityRatio);
    if (m_sparsityThreshold == 0) {
        m_vectorType = DENSE_VECTOR;
        m_sorted = true;
        if (m_dimension > 0) {
            resizeDense(m_dimension, ELBOWROOM);
        } else {
            if (m_capacity > 0) {
                m_data = allocateData(m_capacity);
                m_dataEnd = m_data;
            }
        }
    } else {
        m_vectorType = SPARSE_VECTOR;
        if (m_capacity > 0) {
            resizeSparse(m_capacity);
        }
    }
}

Vector::~Vector()
{
    freeData(m_data);
    freeIndex(m_index);
    sm_fullLenghtReferenceCounter--;
    if (sm_fullLenghtReferenceCounter == 0) {
        delete [] sm_fullLengthVector;
        sm_fullLengthVector = 0;
        sm_fullLengthVectorLenght = 0;

        delete [] sm_countingSortBitVector;
        sm_countingSortBitVector = 0;
        sm_countingSortBitVectorLength = 0;
    }
}

Vector::Vector(const Vector & original)
{
    m_data = 0;
    copy(original);
    sm_fullLenghtReferenceCounter++;
    CHECK;
}

Vector::Vector(const Vector& original, Numerical::Double lambda)
{
    m_vectorType = original.m_vectorType;
    m_size = original.m_size;
    m_dimension = original.m_dimension;
    m_capacity = original.m_capacity;
    m_nonZeros = original.m_nonZeros;
    m_sparsityRatio = original.m_sparsityRatio;
    m_sparsityThreshold = original.m_sparsityThreshold;
    m_sorted = original.m_sorted;
    if (m_capacity == 0) {
        m_data = 0;
        m_index = 0;
    } else {
        if (m_vectorType == DENSE_VECTOR) {
            m_data = allocateData(m_capacity);
            m_index = 0;
        } else {
            m_data = allocateData(m_capacity);
            m_index = allocateIndex(m_capacity);
            memcpy(m_index, original.m_index, m_size * sizeof (unsigned int));
        }
    }
    m_dataEnd = m_data + m_size;
    register Numerical::Double * actualData = m_data;
    register Numerical::Double * originalData = original.m_data;
    register const Numerical::Double * end = original.m_dataEnd;
    for (; originalData < end; originalData++, actualData++) {
        *actualData = *originalData * lambda;
    }

    sm_fullLenghtReferenceCounter++;
    CHECK;
}

void Vector::show() const
{
    LPINFO("");
    for (unsigned int i = 0; i < length(); i++) {
        LPINFO(i << ".:" << at(i));
    }
}

void Vector::clear()
{
    m_nonZeros = 0;
    if (m_sparsityThreshold == 0) {
        Numerical::Double * ptr = m_data;
        for (; ptr < m_dataEnd; ptr++) {
            *ptr = 0.0;
        }
    } else {
        if (m_vectorType == DENSE_VECTOR) {

            m_index = allocateIndex(m_capacity);
            m_size = 0;
            m_dataEnd = m_data;
            m_vectorType = SPARSE_VECTOR;

        } else {
            m_size = 0;
            m_dataEnd = m_data;
        }
    }
    m_sorted = true;
}

void Vector::fill(Numerical::Double value)
{
    m_sorted = true;
    if (value == 0.0) {
        clear();
        return;
    }

    if (this->m_sparsityThreshold == 0.0) {
        Numerical::Double * ptr = m_data;
        for (; ptr < m_dataEnd; ptr++) {
            *ptr = value;
        }
        m_nonZeros = m_size;
    } else {

        if (m_vectorType == SPARSE_VECTOR) {
            sparseToDense();
        }
        Numerical::Double * ptr = m_data;
        for (; ptr < m_dataEnd; ptr++) {
            *ptr = value;
        }
        m_nonZeros = m_size;
    }
}

void Vector::copy(const Vector & original)
{
    m_vectorType = original.m_vectorType;
    m_size = original.m_size;
    m_dimension = original.m_dimension;
    m_capacity = original.m_capacity;
    m_nonZeros = original.m_nonZeros;
    m_sparsityRatio = original.m_sparsityRatio;
    m_sparsityThreshold = original.m_sparsityThreshold;
    m_sorted = original.m_sorted;
    if (m_capacity == 0) {
        m_data = 0;
        m_index = 0;
    } else {
        if (m_vectorType == DENSE_VECTOR) {
            m_data = allocateData(m_capacity);
            COPY_DOUBLES(m_data, original.m_data, m_size);
            m_index = 0;
        } else {
            m_data = allocateData(m_capacity);
            m_index = allocateIndex(m_capacity);
            COPY_DOUBLES(m_data, original.m_data, m_size);
            memcpy(m_index, original.m_index, m_size * sizeof (unsigned int));
        }
    }
    m_dataEnd = m_data + m_size;
    //    CHECK;
}

void Vector::resize(unsigned int length)
{
    if (length == m_dimension) {
        return;
    }
    if (m_vectorType == DENSE_VECTOR) {

        if (m_capacity >= length) {

            Numerical::Double * ptr = m_dataEnd;
            m_size = length;

            m_dataEnd = m_data + m_size;
            if (length > m_dimension) {
                while (ptr < m_dataEnd) {
                    *ptr = 0.0;
                    ptr++;
                }
            } else {
                ptr--;
                while (ptr >= m_dataEnd) {
                    if (*ptr != 0.0) {
                        m_nonZeros--;
                    }
                    ptr--;
                }
            }

        } else {

            m_capacity = length + ELBOWROOM;
            Numerical::Double * newData = allocateData(m_capacity);
            m_dataEnd = newData + length;
            Numerical::Double * actual;
            //            if (length > m_size) {
            memcpy(newData, m_data, sizeof (Numerical::Double) * m_size);
            actual = newData + m_size;
            for (; actual < m_dataEnd; actual++) {
                *actual = 0.0;
            }

            freeData(m_data);
            m_data = newData;
            m_size = length;
        }
    } else {
        if (length < m_dimension && m_nonZeros > 0) {

            Numerical::Double * actualData = m_data;
            unsigned int * actualIndex = m_index;
            Numerical::Double * lastData = m_dataEnd - 1;
            unsigned int * lastIndex = m_index + m_size - 1;
            while (actualData <= lastData) {
                if (*actualIndex >= length) {
                    *actualIndex = *lastIndex;
                    *actualData = *lastData;
                    lastData--;
                    lastIndex--;
                } else {
                    actualData++;
                    actualIndex++;
                }
            }
            m_dataEnd = lastData + 1;
            m_size = m_dataEnd - m_data;
            m_nonZeros = m_size;
        }
    }
    m_dimension = length;
    m_sparsityThreshold = (unsigned int) Numerical::round(m_dimension * m_sparsityRatio);
    if (m_vectorType == DENSE_VECTOR) {
        if (m_nonZeros < m_sparsityThreshold) {
            denseToSparse();
        }
    } else {
        if (m_nonZeros >= m_sparsityThreshold) {
            sparseToDense();
        }
    }

}

void Vector::set(unsigned int index, Numerical::Double value)
{
    if (m_vectorType == DENSE_VECTOR) {
        Numerical::Double & original = m_data[index];
        if (original == 0.0 && value != 0.0) {
            m_nonZeros++;
        } else if (original != 0.0 && value == 0.0) {
            m_nonZeros--;
        }
        m_data[index] = value;
        if (m_nonZeros < m_sparsityThreshold) {
            denseToSparse();
        }
    } else {
        m_sorted = false;
        Numerical::Double * ptr = getElementSparse(index);
        if (value != 0) {
            if (ptr) {
                *ptr = value;
            } else {
                if (m_capacity <= m_size) {
                    resizeSparse(m_size + 1 + ELBOWROOM);
                }
                m_data[m_size] = value;
                m_index[m_size] = index;
                m_size++;
                m_dataEnd++;
                m_nonZeros += value != 0.0;
            }
        } else {
            if (ptr) {
                *ptr = m_data[ m_size - 1 ];
                m_index[ ptr - m_data ] = m_index[ m_size - 1 ];
                m_size--;
                m_dataEnd--;
                m_nonZeros--;
            }
        }
        if (m_nonZeros >= m_sparsityThreshold) {
            sparseToDense();
        }
    }
    CHECK;
}

void Vector::change(unsigned int index, Numerical::Double value)
{
    if (m_vectorType == DENSE_VECTOR) {
        m_data[index] = value;
        if (value == 0.0) {
            m_nonZeros--;
            if (m_nonZeros < m_sparsityThreshold) {
                denseToSparse();
            }
        }
    } else {
        m_sorted = false;
        Numerical::Double * ptr = getElementSparse(index);
        if (value != 0.0) {
            *ptr = value;
        } else {
            *ptr = m_data[ m_size - 1 ];
            m_index[ ptr - m_data ] = m_index[ m_size - 1 ];
            m_size--;
            m_dataEnd--;
            m_nonZeros--;
        }
    }
    CHECK;
}

void Vector::scaleByLambdas(const std::vector<Numerical::Double> & lambdas)
{
    if (m_vectorType == DENSE_VECTOR) {
        register unsigned int index = 0;
        register Numerical::Double * dataPtr = m_data;
        for (; index < m_size; index++, dataPtr++) {
            if (*dataPtr != 0.0) {
                *dataPtr *= lambdas[index];
                if (*dataPtr == 0.0) {
                    m_nonZeros--;
                }
            }
        }
    } else {
        // TODO: lekezelni azt, mikor 0-val szorzunk be
        register Numerical::Double * dataPtr = m_data;
        register unsigned int * indexPtr = m_index;
        Numerical::Double * lastData = m_dataEnd - 1;
        unsigned int * lastIndex = m_index + m_size - 1;

        while (dataPtr <= lastData) {
            *dataPtr *= lambdas[*indexPtr];
            if (*dataPtr == 0.0) {
                m_nonZeros--;
                *dataPtr = *lastData;
                *indexPtr = *lastIndex;
                lastData--;
                lastIndex--;
            } else {
                dataPtr++;
                indexPtr++;
            }
        }
        m_dataEnd = lastData + 1;
        m_size = m_dataEnd - m_data;
        m_nonZeros = m_size;

        m_sorted = false;
    }

    if (m_nonZeros >= m_sparsityThreshold && m_vectorType == SPARSE_VECTOR) {
        sparseToDense();
    } else if (m_nonZeros < m_sparsityThreshold && m_vectorType == DENSE_VECTOR) {
        denseToSparse();
    }
}

void Vector::scaleElementBy(unsigned int index, Numerical::Double lambda)
{
    if (m_vectorType == DENSE_VECTOR) {
        Numerical::Double & original = m_data[index];
        if (original != 0.0 && lambda == 0.0) {
            m_nonZeros--;
        }
        m_data[index] = original * lambda;
        if (m_nonZeros < m_sparsityThreshold) {
            denseToSparse();
        }
    } else {
        m_sorted = false;
        Numerical::Double * ptr = getElementSparse(index);
        if (lambda != 0.0) {
            if (ptr) {
                *ptr *= lambda;
            }
        } else {
            if (ptr) {
                *ptr = m_data[ m_size - 1 ];
                m_index[ ptr - m_data ] = m_index[ m_size - 1 ];
                m_size--;
                m_dataEnd--;
                m_nonZeros--;
            }
        }
        if (m_nonZeros >= m_sparsityThreshold) {
            sparseToDense();
        }
    }
    CHECK;
}

void Vector::setNewNonzero(unsigned int index, Numerical::Double value)
{
    m_nonZeros++;
    if (m_vectorType == DENSE_VECTOR) {
        m_data[index] = value;
        //        if (m_nonZeros < m_sparsityThreshold) {
        //            denseToSparse();
        //        }
    } else {
        m_sorted = false;
        if (m_capacity <= m_size) {
            resizeSparse(m_size + 1 + ELBOWROOM);
        }
        m_data[m_size] = value;
        m_index[m_size] = index;
        m_size++;
        m_dataEnd++;
        if (m_nonZeros >= m_sparsityThreshold) {
            sparseToDense();
        }
    }
    CHECK;
}

Numerical::Double Vector::operator[](unsigned int index) const {
    CHECK;
    return at(index);
}

Vector Vector::operator*(const Matrix& matrix) const
{
    Vector result(matrix.columnCount());
    Vector* justifiedLeftVector = 0;

    if (matrix.rowCount() != this->length()) {
        justifiedLeftVector = new Vector(matrix.rowCount());
        for (unsigned int i = 0; i < matrix.rowCount(); i++) {
            if (i >= this->length()) {
                justifiedLeftVector->set(i, 0.0);
            } else {
                justifiedLeftVector->set(i, this->at(i));
            }
        }
    } else {
        justifiedLeftVector = new Vector(*this);
    }

    for (unsigned int j = 0; j < matrix.columnCount(); j++) {
        Numerical::Double columnResult = 0.0;
        for (unsigned int i = 0; i < matrix.rowCount(); i++) {
            columnResult += matrix.get(i, j) * justifiedLeftVector->at(i);
        }
        result.set(j, columnResult);
    }
    delete justifiedLeftVector;
    justifiedLeftVector = 0;
    return result;
}

Vector Vector::operator*(Numerical::Double m) const
{
    Vector ret(*this);
    ret.scaleBy(m);
    return ret;
}

Vector Vector::operator-(const Vector& vector) const
{
    Vector ret(*this);
    ret.addVector(-1, vector);
    return ret;
}

Vector Vector::operator+(const Vector& vector) const
{
    Vector ret(*this);
    ret.addVector(1, vector);
    return ret;
}

Numerical::Double Vector::at(unsigned int index) const
{

    if (m_vectorType == DENSE_VECTOR) {
        CHECK;
        return m_data[index];
    } else {
        Numerical::Double * value = getElementSparse(index);
        if (value) {
            CHECK;
            return *value;
        } else {
            CHECK;
            return 0.0;
        }
    }
    CHECK;
    return 0;
}

const Numerical::Double * Vector::getDenseElementPointer(unsigned int index) const {
    return m_data + index;
}

unsigned int Vector::length() const
{
    //    CHECK;
    return m_dimension;
}

unsigned int Vector::capacity() const
{
    CHECK;
    return m_capacity;
}

unsigned int Vector::maxIndex() const
{
    if (m_nonZeros == 0) {
        return 0;
    }
    if (m_vectorType == DENSE_VECTOR) {
        CHECK;
        register Numerical::Double * ptr = m_dataEnd - 1;
        register unsigned int index = m_dimension - 1;
        while (*ptr == 0.0) {
            ptr--;
            index--;
        }
        return index;
    } else {
        register const unsigned int * indexPtr = m_index;
        register const unsigned int * const indexPtrEnd = m_index + m_size;
        register unsigned int maxIndex = 0;
        while (indexPtr < indexPtrEnd) {
            register unsigned int actual = *indexPtr;
            if (actual > maxIndex) {
                maxIndex = actual;
            }
            indexPtr++;
        }
        CHECK;
        return maxIndex;
    }
    CHECK;
    return 0;
}

Vector & Vector::scaleBy(Numerical::Double lambda)
{
    if (lambda == 0.0) {
        m_nonZeros = 0;
        if (m_vectorType == DENSE_VECTOR) {
            if (m_nonZeros < m_sparsityThreshold) {
                m_size = 0;
                m_capacity = ELBOWROOM;
                m_vectorType = SPARSE_VECTOR;
                m_dataEnd = m_data;
                m_sorted = true;
                m_index = allocateIndex(m_capacity);
                if (m_capacity == 0) {
                    freeData(m_data);
                    m_dataEnd = 0;
                }
            } else {
                register Numerical::Double * ptr = m_data;
                while (ptr < m_dataEnd) {
                    *ptr = 0;
                    ptr++;
                }
            }
        } else {
            m_sorted = true;
            m_size = 0;
            m_dataEnd = m_data;
        }
    } else {
        register Numerical::Double * ptr = m_data;
        while (ptr < m_dataEnd) {
            *ptr *= lambda;
            ptr++;
        }
    }
    CHECK;
    return *this;
}

Numerical::Double Vector::euclidNorm() const
{
    register Numerical::Double result = 0.0;
    register Numerical::Double * dataPtr = m_data;
    for (; dataPtr < m_dataEnd; dataPtr++) {
        register const Numerical::Double value = *dataPtr;
        result += value * value;
    }
    return Numerical::sqrt(result);
}

Numerical::Double Vector::dotProduct(const Vector & vector) const
{
    if (m_size == 0 || vector.m_size == 0) {
        CHECK;
        return 0.0;
    }


    if (m_vectorType == SPARSE_VECTOR && vector.m_vectorType == SPARSE_VECTOR &&
        m_sorted && vector.m_sorted) {
        //        static unsigned int _counter = 0;
        //        _counter++;
        //        if (_counter % 10000 == 0) {
        //            LPERROR(_counter);
        //        }

        Numerical::Double positive = 0.0;
        Numerical::Double negative = 0.0;

        register unsigned int * index1 = m_index;
        register unsigned int * index2 = vector.m_index;
        register const unsigned int * endIndex1 = m_index + m_nonZeros;
        register const unsigned int * endIndex2 = vector.m_index + vector.m_nonZeros;

        //LPERROR(*this << vector);

        while (index1 < endIndex1 && index2 < endIndex2) {
            if (*index1 > *index2) {
                //LPERROR(*index1 << " > " << *index2 << " -> index2 ugrik ide: " << (index2[1]));
                index2++;
            } else if (*index2 > *index1) {
                //LPERROR(*index2 << " > " << *index1 << " -> index1 ugrik ide: " << (index1[1]));
                index1++;
            } else {
                Numerical::Double value = m_data[ index1 - m_index ] *
                    vector.m_data[ index2 - vector.m_index ];
                if (value > 0.0) {
                    positive += value;
                } else {
                    negative += value;
                }
                index1++;
                index2++;
            }
        }

        return Numerical::stableAdd(negative, positive);

    }
#ifdef SUPERACCUMLATOR
    Numerical::SuperAccumlator & superAccumlator = Numerical::getSuperAccumlator(Numerical::DOT_PRODUCT);
#else
    Numerical::Double positive = 0.0;
    Numerical::Double negative = 0.0;
#endif
    Numerical::Double temp;
    if (m_vectorType == DENSE_VECTOR && vector.m_vectorType == DENSE_VECTOR) {
        register const Numerical::Double * ptr1 = m_data;
        register const Numerical::Double * ptr2 = vector.m_data;
        register const Numerical::Double * end = m_dataEnd;
        Numerical::Double result = 0.0;
        while (ptr1 < end) {
            temp = *ptr1 * *ptr2;
#ifdef SUPERACCUMLATOR
            superAccumlator.add(temp);
#else
            if (temp < 0.0) {
                negative += temp;
            } else {
                positive += temp;
            }
#endif
            //result += *ptr1 * *ptr2;
            ptr1++;
            ptr2++;
        }
#ifdef SUPERACCUMLATOR
        result = superAccumlator.getSum();
#else
        result = Numerical::stableAdd(negative, positive);

#endif

        CHECK;
        if (Numerical::fabs(result) < Numerical::AbsoluteTolerance) {
            result = 0.0;
        }
        return result;
    }

    Numerical::Double * data;
    unsigned int * index;
    unsigned int size;
    bool needScatter;
    register Numerical::Double * denseVector = sm_fullLengthVector;
    unsigned int * origIndex = 0;
    unsigned int origSize = 0;
    // when both of them are sparse, it has to be converted to dense
    if (m_vectorType == DENSE_VECTOR || vector.m_vectorType == DENSE_VECTOR) {
        needScatter = false;
        if (m_vectorType == DENSE_VECTOR) {
            denseVector = m_data;
            data = vector.m_data;
            index = vector.m_index;
            size = vector.m_size;
        } else {
            denseVector = vector.m_data;
            data = m_data;
            index = m_index;
            size = m_size;
        }
    } else {
        // both of them are sparse
        needScatter = true;
        if (m_nonZeros >= vector.m_nonZeros) {
            scatter(sm_fullLengthVector, sm_fullLengthVectorLenght, *this);
            denseVector = sm_fullLengthVector;
            data = vector.m_data;
            index = vector.m_index;
            size = vector.m_size;
            origIndex = m_index;
            origSize = m_size;
        } else {
            scatter(sm_fullLengthVector, sm_fullLengthVectorLenght, vector);
            denseVector = sm_fullLengthVector;
            data = m_data;
            index = m_index;
            size = m_size;
            origIndex = vector.m_index;
            origSize = vector.m_size;
        }
    }

    Numerical::Double result = 0.0;

    register const Numerical::Double * ptrSparse = data;
    register const Numerical::Double * const ptrSparseEnd = ptrSparse + size;
    register const unsigned int * ptrIndex = index;

    while (ptrSparse < ptrSparseEnd) {

        temp = denseVector[ *ptrIndex ] * *ptrSparse;
#ifdef SUPERACCUMLATOR
        superAccumlator.add(temp);
#else
        if (temp < 0.0) {
            negative += temp;
        } else {
            positive += temp;
        }
#endif
        if (needScatter) {
            // denseVector[ *ptrIndex ] = 0.0;
        }
        ptrSparse++;
        ptrIndex++;
    }
    if (needScatter) {
        clearFullLenghtVector(sm_fullLengthVector, origIndex, origSize);
    }
#ifdef SUPERACCUMLATOR
    result = superAccumlator.getSum();
#else
    result = Numerical::stableAdd(negative, positive);
#endif
    CHECK;

    if (Numerical::fabs(result) < Numerical::AbsoluteTolerance) {
        result = 0.0;
    }
    return result;
}

Vector & Vector::addVector(Numerical::Double lambda, const Vector & vector)
{
    if (vector.m_size == 0 || lambda == 0.0) {
        CHECK;
        return *this;
    }
    if (m_vectorType == DENSE_VECTOR) {
        if (vector.m_vectorType == DENSE_VECTOR) {
            addDenseToDense(lambda, vector);
        } else {
            addSparseToDense(lambda, vector);
        }
    } else {
        if (vector.m_vectorType == Vector::SPARSE_VECTOR) {
            addSparseToSparse(lambda, vector);
        } else {
            addDenseToSparse(lambda, vector);
        }
    }

    CHECK;
    return *this;
}

void Vector::addDenseToDense(Numerical::Double lambda, const Vector & vector)
{

    register Numerical::Double * ptr1 = m_data;
    register const Numerical::Double * ptr2 = vector.m_data;
    register const Numerical::Double * end = m_dataEnd;
    //    int nonZeros1 = 0; // DO NOT DELETE YET!!!
    //    int nonZeros2 = 0; // DO NOT DELETE YET!!!
    while (ptr1 < end) {
        if (*ptr2 != 0.0) {
            if (*ptr1 == 0.0) {
                m_nonZeros++;
            }
            *ptr1 = Numerical::stableAdd(*ptr1, *ptr2 * lambda);
            // Numerical::isZero is unnecessary after stableAdd
            if (*ptr1 == 0.0) {
                *ptr1 = 0.0;
                m_nonZeros--;
            }
        }
        ptr1++;
        ptr2++;
    }
    if (m_nonZeros < m_sparsityThreshold) {
        denseToSparse();
    }

    CHECK;
}

void Vector::addDenseToSparse(Numerical::Double lambda, const Vector & vector)
{
    m_sorted = false;
    sparseToDense();
    addDenseToDense(lambda, vector);
    CHECK;
}

void Vector::addSparseToDense(Numerical::Double lambda, const Vector & vector)
{
    register const Numerical::Double * ptrData = vector.m_data;
    register const Numerical::Double * const ptrDataEnd = vector.m_dataEnd;
    register const unsigned int * ptrIndex = vector.m_index;
    while (ptrData < ptrDataEnd) {
        Numerical::Double & data = m_data[ *ptrIndex ];
        if (data == 0.0) {
            m_nonZeros++;
        }
        data = Numerical::stableAdd(data, lambda * *ptrData);
        if (data == 0.0) {
            data = 0.0;
            m_nonZeros--;
        }
        ptrData++;
        ptrIndex++;
    }

    if (m_nonZeros < m_sparsityThreshold) {
        denseToSparse();
    }
    CHECK;
}

void Vector::addSparseToSparse(Numerical::Double lambda, const Vector & vector)
{
    m_sorted = false;
    scatter(sm_fullLengthVector, sm_fullLengthVectorLenght, vector);

    register Numerical::Double * denseVector = sm_fullLengthVector;
    register Numerical::Double * ptrActualVector = m_data;
    register unsigned int * ptrIndex = m_index;
    while (ptrActualVector < m_dataEnd) {
        if (denseVector[ *ptrIndex ] != 0.0) {

            Numerical::Double result = Numerical::stableAdd(*ptrActualVector, lambda * denseVector[ *ptrIndex ]);
            denseVector[ *ptrIndex ] = 0.0;
            if (result != 0.0) {
                *ptrActualVector = result;
                ptrActualVector++;
                ptrIndex++;
            } else {
                m_dataEnd--;
                m_size--;
                m_nonZeros--;
                if (ptrActualVector < m_dataEnd) {
                    *ptrActualVector = *m_dataEnd;
                    *ptrIndex = m_index[ m_size ];
                }
            }
        } else {
            ptrActualVector++;
            ptrIndex++;
        }
    }
    ptrIndex = vector.m_index;
    ptrActualVector = vector.m_data;
    while (ptrActualVector < vector.m_dataEnd) {
        if (sm_fullLengthVector[ *ptrIndex ] != 0.0) {
            Numerical::Double value = lambda * *ptrActualVector;
            if (value != 0.0) {
                if (m_capacity <= m_size) {
                    resizeSparse(m_size + 1 + ELBOWROOM);
                }
                m_data[m_nonZeros] = value;
                m_index[m_nonZeros] = *ptrIndex;
                m_size++;
                m_dataEnd++;
                m_nonZeros++;
            }
            sm_fullLengthVector[ *ptrIndex ] = 0.0;
        }
        ptrIndex++;
        ptrActualVector++;
    }
    if (m_nonZeros >= m_sparsityThreshold) {
        sparseToDense();
    }
    CHECK;
}

Vector & Vector::elementaryFtran(const Vector & eta, unsigned int pivot)
{
    //OLD IMPLEMENTATION
    Numerical::Double pivotValue = at(pivot);
    if (pivotValue == 0.0) {
        CHECK;
        return *this;
    }
    m_sorted = m_vectorType == DENSE_VECTOR;
    Numerical::Double atPivot = eta.at(pivot);
    addVector(pivotValue, eta);
    set(pivot, atPivot * pivotValue);
    return *this;

    //NEW IMPLEMENTATION
    //    Numerical::Double * denseVector;
    //
    //    // 1. lepes: ha kell akkor atvaltjuk dense-re
    //
    //    if (m_vectorType == Vector::DENSE_VECTOR) {
    //        denseVector = m_data;
    //    } else {
    //        Vector::scatter(Vector::sm_fullLengthVector, Vector::sm_fullLengthVectorLenght,
    //            m_data, m_index, m_size, m_dimension);
    //        denseVector = Vector::sm_fullLengthVector;
    //    }
    //
    //    // 2. lepes: vegigmegyunk minden eta vektoron es elvegezzuk a hozzaadast
    //    const Numerical::Double pivotValue = denseVector[ pivot ];
    //
    //    if (eta.m_vectorType == Vector::DENSE_VECTOR) {
    //        register Numerical::Double * ptrValue2 = eta.m_data;
    //        register Numerical::Double * ptrValue1 = denseVector;
    //        register const Numerical::Double * ptrValueEnd = denseVector + m_dimension;
    //        while (ptrValue1 < ptrValueEnd) {
    //            register const Numerical::Double value = *ptrValue2;
    //            if (value != 0.0) {
    //                const Numerical::Double val = Numerical::stableAdd(*ptrValue1, pivotValue * value);
    //                if (*ptrValue1 == 0.0 && val != 0.0) {
    //                    m_nonZeros++;
    //                } else if (*ptrValue1 != 0.0 && val == 0.0) {
    //                    m_nonZeros--;
    //                }
    //                *ptrValue1 = val;
    //            }
    //            ptrValue1++;
    //            ptrValue2++;
    //        }
    //        //A vegen lerendezzuk a pivot poziciot is:
    //        const Numerical::Double val = pivotValue * eta.m_data[pivot];
    //        if (denseVector[pivot] == 0.0 && val != 0.0) {
    //            m_nonZeros++;
    //        } else if (denseVector[pivot] != 0.0 && val == 0.0) {
    //            m_nonZeros--;
    //        }
    //        denseVector[pivot] = val;
    //
    //    } else {
    //        register Numerical::Double * ptrEta = eta.m_data;
    //        register unsigned int * ptrIndex = eta.m_index;
    //        register const unsigned int * ptrIndexEnd = ptrIndex + eta.m_size;
    //        register const unsigned int pivotPosition = pivot;
    //        while (ptrIndex < ptrIndexEnd) {
    //            Numerical::Double & originalValue = denseVector[*ptrIndex];
    //            if (*ptrEta != 0.0) {
    //                Numerical::Double val;
    //                if (*ptrIndex != pivotPosition) {
    //                    val = Numerical::stableAdd(originalValue, pivotValue * *ptrEta);
    //                    if (originalValue == 0.0 && val != 0.0) {
    //                        m_nonZeros++;
    //                    } else if (originalValue != 0.0 && val == 0.0) {
    //                        m_nonZeros--;
    //                    }
    //                } else {
    //                    val = pivotValue * *ptrEta;
    //                }
    //                originalValue = val;
    //            }
    //            ptrIndex++;
    //            ptrEta++;
    //        }
    //    }
    //
    //    // 3. lepes: ha kell akkor v-t atvaltani, adatokat elmenteni
    //    Vector::VECTOR_TYPE newType;
    //
    //    if (m_nonZeros < m_sparsityThreshold) {
    //        newType = Vector::SPARSE_VECTOR;
    //    } else {
    //        newType = Vector::DENSE_VECTOR;
    //    }
    //
    //    if (m_vectorType == Vector::DENSE_VECTOR) {
    //        if (newType == Vector::DENSE_VECTOR) {
    //
    //        } else {
    //            denseToSparse();
    //        }
    //    } else {
    //        prepareForData(m_nonZeros, m_dimension, false);
    //        register Numerical::Double * ptrValue = denseVector;
    //        register const Numerical::Double * ptrValueEnd = denseVector + m_dimension;
    //        register unsigned int index = 0;
    //        while (ptrValue < ptrValueEnd) {
    //            if (*ptrValue != 0.0) {
    //                newNonZero(*ptrValue, index);
    //                *ptrValue = 0.0;
    //            }
    //            ptrValue++;
    //            index++;
    //        }
    //    }

}

Vector & Vector::elementaryBtran(const Vector & eta, unsigned int pivot)
{
    set(pivot, dotProduct(eta));
    return *this;
}

void Vector::removeElement(unsigned int index)
{
    bool minus = false;
    if (m_vectorType == DENSE_VECTOR) {
        minus = m_data[index] != 0.0;
        register Numerical::Double * ptrData1 = m_data + index;
        register const Numerical::Double * ptrData2 = ptrData1 + 1;
        while (ptrData2 < m_dataEnd) {
            *ptrData1 = *ptrData2;
            ptrData1++;
            ptrData2++;
        }
        m_size--;
        m_dataEnd--;
    } else {
        m_sorted = false;
        register unsigned int * indexPtr = m_index;
        register const unsigned int * const indexEnd = m_index + m_size;
        int index2 = -1;
        while (indexPtr < indexEnd) {
            if (*indexPtr > index) {
                (*indexPtr)--;
            } else if (*indexPtr == index) {
                index2 = indexPtr - m_index;
                minus = m_data[ index2 ] != 0.0;
            }
            indexPtr++;
        }

        if (index2 >= 0) {
            removeElementSparse((unsigned int) index2);
        }

        // minden ennel nagyobb indexet csokkenteni kell
    }
    m_dimension--;
    m_sparsityThreshold = (unsigned int) Numerical::round(m_dimension * m_sparsityRatio);
    if (minus) {
        m_nonZeros--;
    }
    if (m_vectorType == DENSE_VECTOR) {
        if (m_nonZeros < m_sparsityThreshold) {
            denseToSparse();
        }
    } else if (m_nonZeros >= m_sparsityThreshold) {
        sparseToDense();
    }


    CHECK;
}

// TODO: az index-edik ele szur be, ha ez az N. elem, akkor a vegere

void Vector::insertElement(unsigned int index, Numerical::Double value)
{
    if (index == m_dimension) {
        append(value);
        CHECK;
        return;
    }
    if (m_vectorType == DENSE_VECTOR) {
        int sub = 0;
        if (m_capacity <= m_size) {
            resizeDense(m_size + 1, ELBOWROOM);
            sub = 1;
        }

        register Numerical::Double * ptrData1 = m_data + m_size - sub;
        register const Numerical::Double * ptrData2 = ptrData1 - 1;
        register Numerical::Double * const stop = m_data + index;
        while (ptrData2 >= stop) {
            *ptrData1 = *ptrData2;
            ptrData1--;
            ptrData2--;
        }
        m_data[index] = value;
        if (value != 0.0) {
            m_nonZeros++;
        }
        if (sub == 0) {
            m_size++;
            m_dimension++;
            m_dataEnd++;
        }
        m_sorted = true;
    } else {
        m_sorted = false;
        register unsigned int * indexPtr = m_index;
        register const unsigned int * const indexEnd = m_index + m_size;
        while (indexPtr < indexEnd) {
            if (*indexPtr >= index) {
                (*indexPtr)++;
            }
            indexPtr++;
        }
        if (value != 0.0) {
            m_nonZeros++;
            if (m_capacity <= m_size) {
                resizeSparse(m_size + 1 + ELBOWROOM);
            }
            m_data[m_size] = value;
            m_index[m_size] = index;
            m_size++;
            m_dataEnd++;
        }
        m_dimension++;
    }

    m_sparsityThreshold = (unsigned int) Numerical::round(m_dimension * m_sparsityRatio);
    if (m_nonZeros >= m_sparsityThreshold && m_vectorType == SPARSE_VECTOR) {
        sparseToDense();
    } else if (m_nonZeros < m_sparsityThreshold && m_vectorType == DENSE_VECTOR) {
        denseToSparse();
    }

    CHECK;
}

void Vector::append(Numerical::Double value)
{
    if (value != 0.0) {
        m_nonZeros++;
    }
    if (m_vectorType == DENSE_VECTOR) {
        if (m_capacity <= m_size) {
            resizeDense(m_size + 1, ELBOWROOM);
            m_data[m_size - 1] = value;
        } else {
            m_data[m_size] = value;
            m_size++;
            m_dataEnd++;
            m_dimension++;
        }

    } else {
        m_sorted = false;
        if (value != 0.0) {
            if (m_capacity <= m_size) {
                resizeSparse(m_size + 1 + ELBOWROOM);
            }
            m_data[m_size] = value;
            m_index[m_size] = m_dimension;
            m_size++;
            m_dataEnd++;
        }
        m_dimension++;
    }

    m_sparsityThreshold = (unsigned int) Numerical::round(m_dimension * m_sparsityRatio);
    if (m_nonZeros >= m_sparsityThreshold && m_vectorType == SPARSE_VECTOR) {
        sparseToDense();
    } else if (m_nonZeros < m_sparsityThreshold && m_vectorType == DENSE_VECTOR) {
        denseToSparse();
    }
    CHECK;
}

Vector & Vector::operator=(const Vector & vector)
{
    freeData(m_data);
    freeIndex(m_index);
    copy(vector);
    CHECK;
    return *this;
}

void Vector::sortElements() const
{
    if (m_vectorType == DENSE_VECTOR || m_nonZeros < 2) {
        return;
    }
    m_sorted = true;
    if (m_nonZeros < 10) {
        selectionSort();
        return;
    }
    // calculating number of steps in different sorting algoirthms

    //bool sorted = true;
    register unsigned int *minPtr = m_index;
    register unsigned int *maxPtr = m_index;
    register unsigned int * actual = m_index;
    const register unsigned int * end = m_index + m_nonZeros;
    for (; actual < end; actual++) {
        if (*actual < *minPtr) {
            minPtr = actual;
        } else if (*actual > *maxPtr) {
            maxPtr = actual;
        }
    }

    unsigned int selectionCount = m_nonZeros * m_nonZeros / 2;
    unsigned int countingCount = m_nonZeros * 2 + (*maxPtr - *minPtr);
    unsigned int heapsortCount = 1;
    while ((1U << heapsortCount) < m_nonZeros) {
        heapsortCount++;
    }
    heapsortCount *= m_nonZeros;
    heapsortCount <<= 1;

    unsigned int sortingAlgorithm = 0;
    unsigned int min = selectionCount;
    if (countingCount < min) {
        min = countingCount;
        sortingAlgorithm = 1;
    }
    if (heapsortCount < min) {
        min = heapsortCount;
        sortingAlgorithm = 2;
    }

    switch (sortingAlgorithm) {
        case 0:
            insertionSort();
            //selectionSort();
            break;
        case 1:
            countingSort();
            break;
        case 2:
            heapSort();
            break;
    }
}

void Vector::countingSort() const
{
    const unsigned int bitCount = sizeof (unsigned long) * CHAR_BIT;
    const unsigned int shift = bitCount == 16 ? 4 : (bitCount == 32 ? 5 : 6);
    const unsigned int arraySize = m_dimension / bitCount + 1;
    unsigned long mask = bitCount == 16 ? 0xf : (bitCount == 32 ? 0x1f : 0x3f);
    register unsigned long * actualBits = sm_countingSortBitVector;
    const unsigned long * endBits = sm_countingSortBitVector + arraySize;

    if (sm_countingSortBitVectorLength < arraySize) {
        delete [] sm_countingSortBitVector;
        sm_countingSortBitVectorLength = arraySize;
        sm_countingSortBitVector = new unsigned long[ arraySize ];
        actualBits = sm_countingSortBitVector;
        endBits = sm_countingSortBitVector + arraySize;
        for (; actualBits < endBits; actualBits++) {
            *actualBits = 0UL;
        }

    }


    scatter(sm_fullLengthVector, sm_fullLengthVectorLenght, *this);
    static int count = 0;
    count++;

    //std::cin.get();


    unsigned int max = 0;
    unsigned int min = 0;
    register unsigned int * actual = m_index;
    const register unsigned int * end = m_index + m_nonZeros;

    for (; actual < end; actual++) {
        //originalIndices[*actual] = actual - m_index;
        const register unsigned int index = *actual >> shift;
        const register unsigned int bitIndex = *actual & mask;
        sm_countingSortBitVector[index] |= 1UL << bitIndex;
        if (max < index) {
            max = index;
        } else if (min > index) {
            min = index;
        }
    }

    Numerical::Double * actualData = m_data;
    actual = m_index;
    actualBits = sm_countingSortBitVector + min;
    unsigned int position = 0;
    const unsigned long * endBits2 = sm_countingSortBitVector + max + 1;
    for (; actualBits < endBits2; actualBits++, position += bitCount) {
        register unsigned int index = 0;
        mask = 0x1;
        const register unsigned long actualElement = *actualBits;
        do {

            if (actualElement & mask) {
                *actual = position + index;
                actual++;
                *actualData = sm_fullLengthVector[ position + index ];
                actualData++;
                sm_fullLengthVector[ position + index ] = 0.0;
            }

            mask <<= 1;
            index++;
        } while (index < bitCount);
        *actualBits = 0;
    }

}

void Vector::heapSort() const
{
    Heap<unsigned int, Numerical::Double> heap(m_nonZeros);
    register unsigned int * actualIndex = m_index;
    register Numerical::Double * actualData = m_data;
    heap.startBuild();
    for (; actualData < m_dataEnd; actualData++, actualIndex++) {
        heap.addForBuild(*actualIndex, *actualData);
    }
    heap.build();
    //actualIndex = m_index + m_nonZeros - 1;
    //actualData = m_data + m_nonZeros - 1;
    actualIndex--;
    actualData--;
    for (; actualData >= m_data; actualData--, actualIndex--) {
        *actualData = heap.getData(0);
        *actualIndex = heap.getMax();
        heap.remove(0);
    }
}

void Vector::selectionSort() const
{
    register unsigned int * actual = m_index;
    const unsigned int * end1 = m_index + m_nonZeros - 1;
    const unsigned int * end2 = end1 + 1;
    for (; actual < end1; actual++) {
        register unsigned int * minPtr = actual;
        register unsigned int * actual2 = actual + 1;
        for (; actual2 < end2; actual2++) {
            if (*actual2 < *minPtr) {
                minPtr = actual2;
            }
        }

        // swap of indices
        register unsigned int tempIndex = *minPtr;
        *minPtr = *actual;
        *actual = tempIndex;

        // swap of values
        register Numerical::Double tempValue = m_data[ minPtr - m_index ];
        m_data[ minPtr - m_index ] = m_data[ actual - m_index ];
        m_data[ actual - m_index ] = tempValue;
    }
}

void Vector::insertionSort() const
{
    if (m_nonZeros < 2) {
        return;
    }
    register unsigned int * actualIndex;
    register unsigned int * previousIndex;
    register Numerical::Double * actualData;
    register Numerical::Double * previousData;
    unsigned int index = 1;
    for (; index < m_nonZeros; index++) {
        actualIndex = m_index + index;
        previousIndex = actualIndex - 1;
        actualData = m_data + index;
        previousData = actualData - 1;
        const Numerical::Double insertData = *actualData;
        const unsigned int insertIndex = *actualIndex;
        //        LPWARNING("index: " << index);
        //        LPWARNING("element: " << insertIndex << ", " << insertData);
        //        LPWARNING("vector: " << *this);
        while (previousIndex >= m_index && *previousIndex > insertIndex) {
            *actualIndex = *previousIndex;
            *actualData = *previousData;

            actualData--;
            actualIndex--;
            previousIndex--;
            previousData--;
        }
        *actualData = insertData;
        *actualIndex = insertIndex;

        //        LPWARNING("insert position: " << (previousIndex - m_index));
        //        LPWARNING("after insert: " << *this);
        //        std::cin.get();
    }
}

void Vector::resizeDense(unsigned int size, unsigned int elbowroom)
{
    m_capacity = size + elbowroom;
    Numerical::Double * temp = allocateData(m_capacity);
    COPY_DOUBLES(temp, m_data, (size > m_size ? m_size : size));
    freeData(m_data);
    m_data = temp;
    m_dataEnd = m_data + size;
    // a maradekot le kell nullazni
    register Numerical::Double * ptr = m_data + m_size;
    while (ptr < m_dataEnd) {
        *ptr = 0.0;
        ptr++;
    }
    m_size = size;
    m_dimension = size;
    m_sparsityThreshold = (unsigned int) Numerical::round(m_dimension * m_sparsityRatio);
    CHECK;
}

void Vector::resizeSparse(unsigned int capacity)
{
    if (capacity == 0) {
        freeData(m_data);
        freeIndex(m_index);
        m_dataEnd = 0;
        m_nonZeros = 0;
        m_size = 0;
        m_sparsityThreshold = 0;
        m_capacity = 0;
        return;
    }
    if (capacity < m_size) {
        capacity = m_size;
    }

    Numerical::Double * tempData = allocateData(capacity);
    unsigned int * tempIndex = allocateIndex(capacity);
    COPY_DOUBLES(tempData, m_data, m_size);
    memcpy(tempIndex, m_index, m_size * sizeof (unsigned int));
    freeData(m_data);
    m_data = tempData;
    freeIndex(m_index);
    m_index = tempIndex;
    m_dataEnd = m_data + m_size;
    m_capacity = capacity;

    m_sparsityThreshold = (unsigned int) Numerical::round(m_dimension * m_sparsityRatio);
    CHECK;
}

Numerical::Double * Vector::getElementSparseLinear(unsigned int index) const
{
    register unsigned int * indexPtr = m_index;
    register const unsigned int * const indexPtrEnd = m_index + m_size;
    while (indexPtr < indexPtrEnd && *indexPtr != index) {
        indexPtr++;
    }
    if (indexPtr < indexPtrEnd) {
        CHECK;
        return m_data + (indexPtr - m_index);
    }
    CHECK;
    return 0;
}

Numerical::Double * Vector::getElementSparseBinary(unsigned int index) const
{
    if (m_nonZeros == 0) {
        return 0;
    }
    register unsigned int * middle;
    register int min = 0;
    register int max = m_nonZeros - 1;
    do {
        middle = m_index + ((min + max) >> 1);
        if (index > *middle) {
            min = (middle - m_index) + 1;
        } else {
            max = (middle - m_index) - 1;
        }
    } while (*middle != index && min <= max);
    if (*middle == index) {
        CHECK;
        return m_data + (middle - m_index);
    }
    CHECK;
    return 0;
}

Numerical::Double * Vector::getElementSparse(unsigned int index) const
{
    if (m_sorted == false) {
        return getElementSparseLinear(index);
    }
    return getElementSparseBinary(index);
}

void Vector::addNewElementSparse(unsigned int index, Numerical::Double value)
{
    if (value == 0.0) {
        return;
    }
    if (m_size >= m_capacity) {
        if (ELBOWROOM == 0) {
            m_capacity++;
        } else {
            m_capacity += ELBOWROOM;
        }
        resizeSparse(m_capacity);
    }
    m_data[m_size] = value;
    m_index[m_size] = index;
    m_size++;
    m_dataEnd++;
    //CHECK;
}

void Vector::removeElementSparse(unsigned int index)
{
    m_data[index] = m_data[m_size - 1];
    m_index[index] = m_index[m_size - 1];
    if (m_index[index] >= m_dimension) {
        LPERROR("Invalid index: " << index << ", but greater index = " << m_dimension - 1);
    }
    m_dataEnd--;
    m_size--;
    CHECK;
}

unsigned int Vector::gather(Numerical::Double * denseVector, Numerical::Double * sparseVector,
    unsigned int * indexVector, unsigned int denseLength,
    bool setZero)
{
    // a denseVector-bol kigyujti a nem nulla elemeket a sparseVector-ba
    // es az indexeket az indexVector-ba
    register unsigned int index = 0;
    register Numerical::Double * ptrDense = denseVector;
    register Numerical::Double * ptrSparse = sparseVector;
    register unsigned int * ptrIndex = indexVector;
    while (index < denseLength) {
        if (*ptrDense != 0.0) {
            *ptrIndex = index;
            *ptrSparse = *ptrDense;
            // setting to zero is NECESSARY!
            //TODO: Ez sojha nincs hasznalva, ki kellene venni
            if (setZero)
                *ptrDense = 0.0;
            ptrIndex++;
            ptrSparse++;
        }
        ptrDense++;
        index++;
    }
    return ptrIndex - indexVector;
}

Numerical::Double * Vector::scatterWithPivot(Numerical::Double * & denseVector, unsigned int & denseLength,
    Numerical::Double * sparseVector, unsigned int * index,
    unsigned int sparseLength, unsigned int sparseMaxIndex, unsigned int pivot)
{
    // sparse -> dense
    Numerical::Double * res = 0;
    if (denseLength < sparseMaxIndex) {
        delete [] denseVector;
        denseVector = allocateData(sparseMaxIndex + 1);
        register Numerical::Double * ptrDense = denseVector;
        register const Numerical::Double * const ptrDenseEnd = denseVector + sparseMaxIndex + 1;
        while (ptrDense < ptrDenseEnd) {
            *ptrDense = 0.0;
            ptrDense++;
        }
        denseLength = sparseMaxIndex;
    }
    if (sparseVector == 0) {
        return 0;
    }
    register Numerical::Double * ptrSparse = sparseVector;
    register const unsigned int * ptrIndex = index;
    register const Numerical::Double * const ptrSparseEnd = sparseVector + sparseLength;
    register Numerical::Double * ptrDense = denseVector;

    while (ptrSparse < ptrSparseEnd) {
        if (*ptrIndex == pivot) {
            res = ptrSparse;
        }
        ptrDense[ *ptrIndex ] = *ptrSparse;
        ptrIndex++;
        ptrSparse++;
    }
    return res;
}

void Vector::scatter(Numerical::Double * & denseVector, unsigned int & denseLength,
    const Vector& sparseVector)
{
    // sparse -> dense
    if (denseLength < sparseVector.m_dimension) {
        delete [] denseVector;
        denseVector = allocateData(sparseVector.m_dimension + 1);
        register Numerical::Double * ptrDense = denseVector;
        register const Numerical::Double * const ptrDenseEnd = denseVector + sparseVector.m_dimension + 1;
        while (ptrDense < ptrDenseEnd) {
            *ptrDense = 0.0;
            ptrDense++;
        }
        denseLength = sparseVector.m_dimension;
    }
    if (sparseVector.m_data == 0) {
        return;
    }
    register const Numerical::Double * ptrSparse = sparseVector.m_data;
    register const unsigned int * ptrIndex = sparseVector.m_index;
    register const Numerical::Double * const ptrSparseEnd = sparseVector.m_data + sparseVector.m_size;
    register Numerical::Double * ptrDense = denseVector;
    while (ptrSparse < ptrSparseEnd) {
        //LPINFO("*ptrIndex: " << *ptrIndex);
        ptrDense[ *ptrIndex ] = *ptrSparse;
        ptrIndex++;
        ptrSparse++;
    }

}

//void Vector::scatter(Numerical::Double * & denseVector, unsigned int & denseLength,
//    Numerical::Double * sparseVector, unsigned int * index,
//    unsigned int sparseLength, unsigned int sparseMaxIndex)
//{
//    // sparse -> dense
//    if (denseLength < sparseMaxIndex) {
//        delete [] denseVector;
//        denseVector = allocateData(sparseMaxIndex + 1);
//        register Numerical::Double * ptrDense = denseVector;
//        register const Numerical::Double * const ptrDenseEnd = denseVector + sparseMaxIndex + 1;
//        while (ptrDense < ptrDenseEnd) {
//            *ptrDense = 0.0;
//            ptrDense++;
//        }
//        denseLength = sparseMaxIndex;
//    }
//    if (sparseVector == 0) {
//        return;
//    }
//    register const Numerical::Double * ptrSparse = sparseVector;
//    register const unsigned int * ptrIndex = index;
//    register const Numerical::Double * const ptrSparseEnd = sparseVector + sparseLength;
//    register Numerical::Double * ptrDense = denseVector;
//    while (ptrSparse < ptrSparseEnd) {
//        //LPINFO("*ptrIndex: " << *ptrIndex);
//        ptrDense[ *ptrIndex ] = *ptrSparse;
//        ptrIndex++;
//        ptrSparse++;
//    }

//}

void Vector::clearFullLenghtVector(Numerical::Double * denseVector,
    unsigned int * sparseIndex, unsigned int sparseLength)
{
    register const unsigned int * ptrIndex = sparseIndex;
    register const unsigned int * const ptrIndexEnd = sparseIndex + sparseLength;
    while (ptrIndex < ptrIndexEnd) {
        denseVector[ *ptrIndex ] = 0.0;
        ptrIndex++;
    }
}

void Vector::denseToSparse()
{
    m_capacity = m_nonZeros + ELBOWROOM;
    m_index = allocateIndex(m_capacity);
    Numerical::Double * temp = allocateData(m_capacity);
    gather(m_data, temp, m_index, m_size, false);
    m_sorted = true;
    m_size = m_nonZeros;
    freeData(m_data);
    m_data = temp;
    m_dataEnd = m_data + m_size;
    m_vectorType = SPARSE_VECTOR;
    CHECK;
}

void Vector::sparseToDense()
{
    m_sorted = true;
    m_capacity = m_dimension + ELBOWROOM;
    Numerical::Double * temp = allocateData(m_capacity);
    register Numerical::Double * ptr = temp;
    register const Numerical::Double * const end = temp + m_dimension;
    while (ptr < end) {
        *ptr = 0.0;
        ptr++;
    }
    scatter(temp, m_dimension, *this);

    freeData(m_data);
    m_data = temp;
    freeIndex(m_index);
    m_size = m_dimension;
    m_dataEnd = m_data + m_size;
    m_vectorType = DENSE_VECTOR;
    CHECK;
}

void Vector::setSparsityRatio(Numerical::Double ratio)
{
    m_sparsityRatio = ratio;
    m_sparsityThreshold = (unsigned int) Numerical::round(m_dimension * m_sparsityRatio);
    if (m_vectorType == DENSE_VECTOR && m_nonZeros < m_sparsityThreshold) {
        denseToSparse();
    } else if (m_vectorType == SPARSE_VECTOR && m_nonZeros >= m_sparsityThreshold) {
        sparseToDense();
    }
    CHECK;
}

Numerical::Double Vector::getSparsityRatio() const
{
    CHECK;
    return m_sparsityRatio;
}

bool Vector::operator==(const Vector & vector) const
{
    if (m_nonZeros != vector.m_nonZeros) {
        return false;
    }
    Vector::NonzeroIterator iterator1 = vector.beginNonzero();
    Vector::NonzeroIterator iterator2 = beginNonzero();
    Vector::NonzeroIterator iteratorEnd1 = vector.endNonzero();
    for (; iterator1 < iteratorEnd1; iterator1++, iterator2++) {
        if (iterator1.getIndex() != iterator2.getIndex()) {
            return false;
        }
        if (*iterator1 != *iterator2) {
            return false;
        }
    }
    return true;
}

std::ostream & operator<<(std::ostream & os, const Vector & vector)
{
    unsigned int index;
#ifndef NODEBUG
    os << "data members:" << std::endl;
    os << "\tm_vectorType: " << (vector.m_vectorType == Vector::DENSE_VECTOR ?
        "DENSE_VECTOR" : "SPARSE_VECTOR") << std::endl;
    os << "\tm_size: " << vector.m_size << std::endl;
    os << "\tm_dimension: " << vector.m_dimension << std::endl;
    os << "\tm_capacity: " << vector.m_capacity << std::endl;
    os << "\tm_data: " << vector.m_data << std::endl;
    os << "\tm_dataEnd: " << vector.m_dataEnd << std::endl;
    os << "\tm_index: " << vector.m_index << std::endl;
    os << "\tm_nonZeros: " << vector.m_nonZeros << std::endl;
    os << "\tm_sparsityRatio: " << vector.m_sparsityRatio << std::endl;
    os << "\tm_sparsityThreshold: " << vector.m_sparsityThreshold << std::endl;
    os << "\tm_sorted: " << vector.m_sorted << std::endl;
    os << "m_data: " << std::endl;
    for (index = 0; index < vector.m_size; index++) {
        os << vector.m_data[index] << " ";
    }
    os << std::endl;
    if (vector.m_vectorType == Vector::SPARSE_VECTOR) {
        for (index = 0; vector.m_index && index < vector.m_size; index++) {
            os << "[ " << vector.m_index[index] << "; " << vector.m_data[index] << "] ";
        }
    }
    os << std::endl;

    //    if (vector.m_vectorType == Vector::DENSE_VECTOR) {
    //        for (index = 0; index < vector.m_size; index++) {
    //            os << vector.m_data[index] << " ";
    //        }
    //    } else {
    for (unsigned int index = 0; index < vector.m_dimension; index++) {
        os << vector.at(index) << " ";
    }
    os << std::endl;
    //    }

#endif
    return os;
}

Numerical::Double Vector::absMaxElement()
{
    Numerical::Double result = 0;
    if (nonZeros() > 0) {
        result = *beginNonzero();
        for (Vector::NonzeroIterator it = beginNonzero(); it < endNonzero(); it++) {
            if (*it > 0) {
                if (*it > result)
                    result = (*it);
            } else {
                if (-(*it) > result)
                    result = -(*it);
            }
        }
    }
    return result;
}

Vector operator*(Numerical::Double m, const Vector& v)
{
    Vector ret(v);
    ret.scaleBy(m);
    return ret;
}
