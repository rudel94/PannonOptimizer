#include <coretest.h>
#include <globals.h>
#include <initpanopt.h>
#include <debug.h>
#include <cmath>
#include <utils/arch/x86.h>
#include <utils/primitives.h>
#include <limits>
#include <ctime>

#define MEMCPY_TESTBUFFER_MAX_SIZE      1024*10
#define MEMCPY_FIX_BORDER               10
#define MEMCPY_MAX_SMALL_SIZE           250

#define DOT_PRODUCT_MAX_SIZE            1024

enum MEMCPY_SIZE_CASES {
    LARGE,
    SMALL,
    EMPTHY
};

CoreTestSuite::CoreTestSuite(const char *name): UnitTest(name) {
    ADD_TEST(CoreTestSuite::memcpy);
    //ADD_TEST(CoreTestSuite::memset);
    //ADD_TEST(CoreTestSuite::denseToDenseDotProduct);
    //ADD_TEST(CoreTestSuite::denseToSparseDotProduct);
    //ADD_TEST(CoreTestSuite::denseToDenseAdd);
    //ADD_TEST(CoreTestSuite::denseToDenseAddPerformance);
}

void CoreTestSuite::memcpy() {
    unsigned int alignIndex;
    for (alignIndex = 0; alignIndex <= 1; alignIndex++) {
        char * testBufferA;
        char * testBufferB;
        if (alignIndex == 0) {
            testBufferB = new char[MEMCPY_TESTBUFFER_MAX_SIZE];
            testBufferA = new char[MEMCPY_TESTBUFFER_MAX_SIZE];
        } else {
            testBufferA = alloc<char, 16>(MEMCPY_TESTBUFFER_MAX_SIZE);
            testBufferB = alloc<char, 16>(MEMCPY_TESTBUFFER_MAX_SIZE);
        }

        unsigned int index;
        unsigned int startIndex;
        unsigned int endIndex;
        unsigned int sizeIndex;
        unsigned int testCase;

        for (index = 0; index < MEMCPY_TESTBUFFER_MAX_SIZE; index++) {
            testBufferA[index] = (index * index) % CHAR_MAX;
            testBufferB[index] = 0;
        }

        for (testCase = 0; testCase <= 2; testCase++) {
            switch (testCase) {
            case 0:
                LPINFO("Classic memcpy");
                break;
            case 1:
                LPINFO("Copy with SSE2, using cache");
                break;
            case 2:
                LPINFO("Copy with SSE4-1, without cache");
                break;
            }

            // copy from testBufferA to testBufferB
            for (startIndex = 0; startIndex <= 100; startIndex++) {
                for (endIndex = 0; endIndex <= 100; endIndex++) {
                    for (sizeIndex = 0; sizeIndex < 3; sizeIndex++) {

                        for (index = 0; index < MEMCPY_TESTBUFFER_MAX_SIZE; index++) {
                            testBufferB[index] = 0;
                        }


                        unsigned int start = MEMCPY_FIX_BORDER + startIndex;
                        unsigned int end;
                        switch (sizeIndex) {
                        case LARGE:
                            end = MEMCPY_TESTBUFFER_MAX_SIZE - MEMCPY_FIX_BORDER - endIndex;
                            break;
                        case SMALL:
                            end = MEMCPY_MAX_SMALL_SIZE - MEMCPY_FIX_BORDER - endIndex;
                            break;
                        case EMPTHY:
                            end = start;
                            break;
                        }
                        //end = start;
                        unsigned int size = end - start;

                        switch (testCase) {
                        case 0:
                            ::memcpy(testBufferB + start, testBufferA + start, size);
                            break;
                        case 1:
                            if (InitPanOpt::getInstance().getArchitecture().featureExists("SSE2") ) {
                                MEMCPY_CACHE_SSE2(testBufferB + start, testBufferA + start, size);
                            }
                            break;
                        case 2:
                            if (InitPanOpt::getInstance().getArchitecture().featureExists("SSE4-1") ) {
                                MEMCPY_NO_CACHE_SSE4_1(testBufferB + start, testBufferA + start, size);
                            }

                        }

                        // checking
                        for (index = 0; index < start; index++) {
                            if (testBufferB[index] != 0) {
                                TEST_ASSERT(testBufferB[index] == 0);
                                return;
                            }
                        }
                        for (index = start; index < end; index++) {
                            if (testBufferA[index] != testBufferB[index]) {
                                LPERROR( testBufferA[index] << " != " << testBufferB[index] );
                                TEST_ASSERT(testBufferA[index] == testBufferB[index]);
                                return;
                            }
                        }
                        for (index = end; index < MEMCPY_TESTBUFFER_MAX_SIZE; index++) {
                            if (testBufferB[index] != 0) {
                                TEST_ASSERT(testBufferB[index] == 0);
                                return;
                            }
                        }

                    }
                }
            }

        }
        if (alignIndex == 0) {
            delete [] testBufferA;
            delete [] testBufferB;
        } else {
            release(testBufferA);
            release(testBufferB);
        }
    }
}

void CoreTestSuite::memset() {

}

void CoreTestSuite::denseToDenseDotProduct() {
    double * arrayA = alloc<double, 32>(DOT_PRODUCT_MAX_SIZE);
    double * arrayB = alloc<double, 32>(DOT_PRODUCT_MAX_SIZE);
    double * arrayARef = alloc<double, 32>(DOT_PRODUCT_MAX_SIZE);
    double * arrayBRef = alloc<double, 32>(DOT_PRODUCT_MAX_SIZE);

    unsigned int index;
    unsigned int testCase;
    // generate stable input
    srand(0);
    for (index = 0; index < DOT_PRODUCT_MAX_SIZE; index++) {
        if (index < 100) {
            arrayA[index] = index / 16.0;
            arrayB[index] = (index * 1.25) * pow(-1, index);
        } else {
            arrayA[index] = (rand() % 1000 / 64.0) - 3;
            arrayB[index] = (rand() % 1000 / 16.0) - 6;
        }
        arrayARef[index] = arrayA[index];
        arrayBRef[index] = arrayB[index];
    }

    for (testCase = 0; testCase <= 50; testCase++) {
        unsigned int count;
        if (testCase < 40) {
            count = testCase;
        } else {
            count = testCase * 20;
        }
        double result;
        double resultRef = 0;
        for (index = 0; index < count; index++) {
            resultRef += arrayA[index + 4] * arrayB[index + 4];
        }
        unsigned int function;
        for (function = 0; function <= 3; function++) {
            result = rand() % 100 + 10;
            switch (function) {
            case 0: // default, platform independent implementation
                continue;
                break;
            case 1:
                result = denseToDenseDotProductUnstable(arrayA + 4, arrayB + 4, count);
                /*if (InitPanOpt::getInstance().getArchitecture().featureExists("FPU") &&
                        InitPanOpt::getInstance().getArchitecture().featureExists("CMOV")) {
                    continue;
                } else {

                }*/
                break;
            case 2:
                if (InitPanOpt::getInstance().getArchitecture().featureExists("SSE2") ) {
                    result = DENSE_TO_DENSE_DOTPRODUCT_UNSTABLE_SSE2(arrayA + 4, arrayB + 4, count);
                }
                break;
            case 3:
                if (InitPanOpt::getInstance().getArchitecture().featureExists("AVX") ) {
                    result = DENSE_TO_DENSE_DOTPRODUCT_UNSTABLE_AVX(arrayA + 4, arrayB + 4, count);
                }
                break;
            }
            TEST_ASSERT(result == resultRef);
            if (result != resultRef) {
                LPINFO(count << "   " << result << " != " << resultRef);
                return;
            }
            for (index = 0; index < DOT_PRODUCT_MAX_SIZE; index++) {
                if (arrayA[index] != arrayARef[index]) {
                    TEST_ASSERT(arrayA[index] == arrayARef[index]);
                    return;
                }
                if (arrayB[index] != arrayBRef[index]) {
                    TEST_ASSERT(arrayB[index] == arrayBRef[index]);
                    return;
                }
            }

        }
    }


    release(arrayA);
    release(arrayB);
}

void CoreTestSuite::denseToSparseDotProduct() {
    double * arrayA = alloc<double, 32>(DOT_PRODUCT_MAX_SIZE);
    double * arrayB = alloc<double, 32>(DOT_PRODUCT_MAX_SIZE);
    double * arrayARef = alloc<double, 32>(DOT_PRODUCT_MAX_SIZE);
    double * arrayBRef = alloc<double, 32>(DOT_PRODUCT_MAX_SIZE);
    unsigned int * indicesB = alloc<unsigned int, 32>(3);
    indicesB[0] = 1000;
    indicesB[1] = 1020;
    indicesB[2] = 1120;
    indicesB[3] = 1156;
    indicesB[4] = 1220;
    indicesB[5] = 1320;
    indicesB[6] = 1420;
    indicesB[7] = 1520;
    indicesB[8] = 1620;
    indicesB[9] = 2156;
    unsigned int index;
    for (index = 0; index < DOT_PRODUCT_MAX_SIZE; index++) {
        arrayA[index] = 0;
        arrayB[index] = 0;
    }
    arrayB[0] = -1;
    arrayB[1] = -2;
    arrayB[2] = -3;
    arrayB[3] = -4;
    arrayB[4] = -5;
    arrayB[5] = -6;
    arrayB[6] = -7;
    arrayB[7] = -8;
    arrayB[8] = -9;
    arrayB[9] = -10;
    arrayA[1000] = 1;
    arrayA[1020] = 2;
    arrayA[1120] = 3;
    arrayA[1156] = 4;
    arrayA[1220] = 5;
    arrayA[1320] = 6;
    arrayA[1420] = 7;
    arrayA[1520] = 8;
    arrayA[1620] = 9;
    arrayA[2156] = 10;

    LPINFO(arrayA);
    LPINFO(arrayB);
    LPINFO(indicesB);
    double res = DENSE_TO_SPARSE_DOTPRODUCT_UNSTABLE_AVX(arrayA,
                                                         arrayB,
                                                         indicesB,
                                                         10);
    LPINFO("res = " << res);

    unsigned int aSize = 1000000;
    unsigned int bSize = 10;
    unsigned int indexSize = bSize;
    unsigned int repeat = 10000000;
    unsigned int repIndex;

    double * a = alloc<double, 32>(aSize);
    double * b = alloc<double, 32>(bSize);
    unsigned int * indices = alloc<unsigned int, 32>(indexSize);
    for (index = 0; index < aSize; index++) {
        a[index] = rand() % 10000 / 1000.0 - 6;
    }
    for (index = 0; index < bSize; index++) {
        b[index] = rand() % 10000 / 1000.0;
    }
    // fill indices with random indices
    for (index = 0; index < indexSize; index++) {
        indices[index] = index;
    }
    for (index = 0; index < indexSize * 100; index++) {
        int i = rand() % indexSize;
        int j = rand() % indexSize;
        unsigned int temp = indices[i];
        indices[i] = indices[j];
        indices[j] = temp;
    }
    clock_t start, end;
    res = 0;
    start = clock();
    for (repIndex = 0; repIndex < repeat; repIndex++) {
        double neg = 0.0;
        res = DENSE_TO_SPARSE_DOTPRODUCT_STABLE_AVX(a, b, indices, indexSize, &neg);
        res += neg;
        //res = DENSE_TO_SPARSE_DOTPRODUCT_UNSTABLE_SSE2(a, b, indices, indexSize);
    }
    end = clock();
    LPINFO("time = " << ( end - start ) / (double)CLOCKS_PER_SEC );
    LPINFO("res = " << res);
    //return;

    start = clock();
    for (repIndex = 0; repIndex < repeat; repIndex++) {
        res = DENSE_TO_SPARSE_DOTPRODUCT_UNSTABLE_AVX(a, b, indices, indexSize);
    }
    end = clock();
    LPINFO("time = " << ( end - start ) / (double)CLOCKS_PER_SEC );
    LPINFO("res = " << res);

}

void CoreTestSuite::denseToDenseAdd()
{
    unsigned int i;
    unsigned int size;

    double * a = alloc<double, 32>(11);
    double * a2 = alloc<double, 32>(11);
    double * b = alloc<double, 32>(11);
    double * c = alloc<double, 32>(11);

    a[0] = 1;   b[0] = -1.1;
    a[1] = 0.001;   b[1] = -0.0011;
    a[2] = 10000;   b[2] = -10000.1;
    a[3] = 4;   b[3] = 4;
    a[4] = 5;   b[4] = 5;
    a[5] = 6;   b[5] = 6;
    a[6] = 7;   b[6] = -7.5;
    a[7] = 8;   b[7] = 8;
    a[8] = 9;   b[8] = 9;
    a[9] = 10;  b[9] = 10;
    a[10] = 11; b[10] = 11;

    Numerical::Double oldAbsTolerance = Numerical::AbsoluteTolerance;
    Numerical::Double oldRelTolerance = Numerical::RelativeTolerance;

    double lambda = 1.0;
    double absTolerance = 0.01;
    double relTolerance = 0.04;

    Numerical::AbsoluteTolerance = absTolerance;
    Numerical::RelativeTolerance = relTolerance;

    double nan = std::numeric_limits<double>::quiet_NaN();

    for (size = 0; size <= 11; size++) {
        for (i = 0; i < 11; i++) {
            c[i] = nan;
        }

        DENSE_TO_DENSE_ADD_ABS_REL_SSE2_CACHE(a, b, c, size, lambda, absTolerance, relTolerance);
        for (i = 0; i < size; i++) {
            TEST_ASSERT(c[i] == Numerical::stableAdd(a[i], b[i] * lambda));
            if (c[i] != Numerical::stableAdd(a[i], b[i] * lambda)) {
                LPERROR(c[i] << " != " << Numerical::stableAdd(a[i], b[i] * lambda));
            }
        }

        for (i = 0; i < 11; i++) {
            c[i] = nan;
        }

        DENSE_TO_DENSE_ADD_ABS_REL_SSE2_NOCACHE(a, b, c, size, lambda, absTolerance, relTolerance);
        for (i = 0; i < size; i++) {
            TEST_ASSERT(c[i] == Numerical::stableAdd(a[i], b[i] * lambda));
            if (c[i] != Numerical::stableAdd(a[i], b[i] * lambda)) {
                LPERROR(c[i] << " != " << Numerical::stableAdd(a[i], b[i] * lambda));
            }
        }

        for (i = 0; i < 11; i++) {
            c[i] = nan;
        }

        DENSE_TO_DENSE_ADD_ABS_SSE2_CACHE(a, b, c, size, lambda, absTolerance);
        for (i = 0; i < size; i++) {
            TEST_ASSERT(c[i] == Numerical::stableAddAbs(a[i], b[i] * lambda));
            if (c[i] != Numerical::stableAddAbs(a[i], b[i] * lambda)) {
                LPERROR(c[i] << " != " << Numerical::stableAddAbs(a[i], b[i] * lambda));
            }
        }

        for (i = 0; i < 11; i++) {
            c[i] = nan;
        }

        DENSE_TO_DENSE_ADD_ABS_SSE2_NOCACHE(a, b, c, size, lambda, absTolerance);
        for (i = 0; i < size; i++) {
            TEST_ASSERT(c[i] == Numerical::stableAddAbs(a[i], b[i] * lambda));
            if (c[i] != Numerical::stableAddAbs(a[i], b[i] * lambda)) {
                LPERROR(c[i] << " != " << Numerical::stableAddAbs(a[i], b[i] * lambda));
            }
        }

        for (i = 0; i < 11; i++) {
            c[i] = nan;
        }

        DENSE_TO_DENSE_ADD_SSE2_CACHE(a, b, c, size, lambda);
        for (i = 0; i < size; i++) {
            TEST_ASSERT(c[i] == a[i] + b[i] * lambda);
            if (c[i] != a[i] + b[i] * lambda) {
                LPERROR(c[i] << " != " << a[i] + b[i] * lambda);
            }
        }

        for (i = 0; i < 11; i++) {
            c[i] = nan;
        }

        DENSE_TO_DENSE_ADD_SSE2_NOCACHE(a, b, c, size, lambda);
        for (i = 0; i < size; i++) {
            TEST_ASSERT(c[i] == a[i] + b[i] * lambda);
            if (c[i] != a[i] + b[i] * lambda) {
                LPERROR(c[i] << " != " << a[i] + b[i] * lambda);
            }
        }

        for (i = 0; i < 11; i++) {
            c[i] = nan;
        }

        DENSE_TO_DENSE_ADD_ABS_REL_AVX_CACHE(a, b, c, size, lambda, absTolerance, relTolerance);
        for (i = 0; i < size; i++) {
            TEST_ASSERT(c[i] == Numerical::stableAdd(a[i], b[i] * lambda));
            if (c[i] != Numerical::stableAdd(a[i], b[i] * lambda)) {
                LPERROR(c[i] << " != " << Numerical::stableAdd(a[i], b[i] * lambda));
            }
        }

        for (i = 0; i < 11; i++) {
            c[i] = nan;
        }

        DENSE_TO_DENSE_ADD_ABS_REL_AVX_NOCACHE(a, b, c, size, lambda, absTolerance, relTolerance);
        for (i = 0; i < size; i++) {
            TEST_ASSERT(c[i] == Numerical::stableAdd(a[i], b[i] * lambda));
            if (c[i] != Numerical::stableAdd(a[i], b[i] * lambda)) {
                LPERROR(c[i] << " != " << Numerical::stableAdd(a[i], b[i] * lambda));
            }
        }

        for (i = 0; i < 11; i++) {
            c[i] = nan;
        }

        DENSE_TO_DENSE_ADD_ABS_AVX_CACHE(a, b, c, size, lambda, absTolerance);
        for (i = 0; i < size; i++) {
            TEST_ASSERT(c[i] == Numerical::stableAddAbs(a[i], b[i] * lambda));
            if (c[i] != Numerical::stableAddAbs(a[i], b[i] * lambda)) {
                LPERROR(c[i] << " != " << Numerical::stableAddAbs(a[i], b[i] * lambda));
            }
        }

        for (i = 0; i < 11; i++) {
            c[i] = nan;
        }

        DENSE_TO_DENSE_ADD_ABS_AVX_NOCACHE(a, b, c, size, lambda, absTolerance);
        for (i = 0; i < size; i++) {
            TEST_ASSERT(c[i] == Numerical::stableAddAbs(a[i], b[i] * lambda));
            if (c[i] != Numerical::stableAddAbs(a[i], b[i] * lambda)) {
                LPERROR(c[i] << " != " << Numerical::stableAddAbs(a[i], b[i] * lambda));
            }
        }

        for (i = 0; i < 11; i++) {
            c[i] = nan;
        }

        DENSE_TO_DENSE_ADD_AVX_CACHE(a, b, c, size, lambda);
        for (i = 0; i < size; i++) {
            TEST_ASSERT(c[i] == a[i] + b[i] * lambda);
            if (c[i] != a[i] + b[i] * lambda) {
                LPERROR(c[i] << " != " << a[i] + b[i] * lambda);
            }
        }

        for (i = 0; i < 11; i++) {
            c[i] = nan;
        }

        DENSE_TO_DENSE_ADD_AVX_NOCACHE(a, b, c, size, lambda);
        for (i = 0; i < size; i++) {
            TEST_ASSERT(c[i] == a[i] + b[i] * lambda);
            if (c[i] != a[i] + b[i] * lambda) {
                LPERROR(c[i] << " != " << a[i] + b[i] * lambda);
            }
        }


        for (i = 0; i < 11; i++) {
            c[i] = nan;
        }

        ::denseToDenseAddAbsRel(a, b, c, size, lambda, absTolerance, relTolerance);
        for (i = 0; i < size; i++) {
            TEST_ASSERT(c[i] == Numerical::stableAdd(a[i], b[i] * lambda));
            if (c[i] != Numerical::stableAdd(a[i], b[i] * lambda)) {
                LPERROR(c[i] << " != " << Numerical::stableAdd(a[i], b[i] * lambda));
            }
        }

        for (i = 0; i < 11; i++) {
            c[i] = nan;
        }

        ::denseToDenseAddAbs(a, b, c, size, lambda, absTolerance);
        for (i = 0; i < size; i++) {
            TEST_ASSERT(c[i] == Numerical::stableAddAbs(a[i], b[i] * lambda));
            if (c[i] != Numerical::stableAddAbs(a[i], b[i] * lambda)) {
                LPERROR(c[i] << " != " << Numerical::stableAddAbs(a[i], b[i] * lambda));
            }
        }

        for (i = 0; i < 11; i++) {
            c[i] = nan;
        }

        ::denseToDenseAdd(a, b, c, size, lambda);
        for (i = 0; i < size; i++) {
            TEST_ASSERT(c[i] == a[i] + b[i] * lambda);
            if (c[i] != a[i] + b[i] * lambda) {
                LPERROR(c[i] << " != " << a[i] + b[i] * lambda);
            }
        }
    }

    a2[0] = 1; a2[1] = 0.001; a2[2] = 10000;
    a2[3] = 4; a2[4] = 5; a2[5] = 6;
    a2[6] = 7; a2[7] = 8; a2[8] = 9;
    a2[9] = 10; a2[10] = 11;

    for (size = 0; size <= 11; size++) {
        a[0] = 1; a[1] = 0.001; a[2] = 10000;
        a[3] = 4; a[4] = 5; a[5] = 6;
        a[6] = 7; a[7] = 8; a[8] = 9;
        a[9] = 10; a[10] = 11;

        DENSE_TO_DENSE_ADD_ABS_REL_SSE2_CACHE(a, b, a, size, lambda, absTolerance, relTolerance);
        for (i = 0; i < size; i++) {
            TEST_ASSERT(a[i] == Numerical::stableAdd(a2[i], b[i] * lambda));
            if (a[i] != Numerical::stableAdd(a2[i], b[i] * lambda)) {
                LPERROR(a[i] << " != " << Numerical::stableAdd(a2[i], b[i] * lambda));
            }
        }

        a[0] = 1; a[1] = 0.001; a[2] = 10000;
        a[3] = 4; a[4] = 5; a[5] = 6;
        a[6] = 7; a[7] = 8; a[8] = 9;
        a[9] = 10; a[10] = 11;

        DENSE_TO_DENSE_ADD_ABS_REL_SSE2_NOCACHE(a, b, a, size, lambda, absTolerance, relTolerance);
        for (i = 0; i < size; i++) {
            TEST_ASSERT(a[i] == Numerical::stableAdd(a2[i], b[i] * lambda));
            if (a[i] != Numerical::stableAdd(a2[i], b[i] * lambda)) {
                LPERROR(a[i] << " != " << Numerical::stableAdd(a2[i], b[i] * lambda));
            }
        }

        a[0] = 1; a[1] = 0.001; a[2] = 10000;
        a[3] = 4; a[4] = 5; a[5] = 6;
        a[6] = 7; a[7] = 8; a[8] = 9;
        a[9] = 10; a[10] = 11;

        DENSE_TO_DENSE_ADD_ABS_SSE2_CACHE(a, b, a, size, lambda, absTolerance);
        for (i = 0; i < size; i++) {
            TEST_ASSERT(a[i] == Numerical::stableAddAbs(a2[i], b[i] * lambda));
            if (a[i] != Numerical::stableAddAbs(a2[i], b[i] * lambda)) {
                LPERROR(a[i] << " != " << Numerical::stableAddAbs(a2[i], b[i] * lambda));
            }
        }

        a[0] = 1; a[1] = 0.001; a[2] = 10000;
        a[3] = 4; a[4] = 5; a[5] = 6;
        a[6] = 7; a[7] = 8; a[8] = 9;
        a[9] = 10; a[10] = 11;

        DENSE_TO_DENSE_ADD_ABS_SSE2_NOCACHE(a, b, a, size, lambda, absTolerance);
        for (i = 0; i < size; i++) {
            TEST_ASSERT(a[i] == Numerical::stableAddAbs(a2[i], b[i] * lambda));
            if (a[i] != Numerical::stableAddAbs(a2[i], b[i] * lambda)) {
                LPERROR(a[i] << " != " << Numerical::stableAddAbs(a2[i], b[i] * lambda));
            }
        }

        a[0] = 1; a[1] = 0.001; a[2] = 10000;
        a[3] = 4; a[4] = 5; a[5] = 6;
        a[6] = 7; a[7] = 8; a[8] = 9;
        a[9] = 10; a[10] = 11;

        DENSE_TO_DENSE_ADD_SSE2_CACHE(a, b, a, size, lambda);
        for (i = 0; i < size; i++) {
            TEST_ASSERT(a[i] == a2[i] + b[i] * lambda);
            if (a[i] != a2[i] + b[i] * lambda) {
                LPERROR(a[i] << " != " << a2[i] + b[i] * lambda);
            }
        }

        a[0] = 1; a[1] = 0.001; a[2] = 10000;
        a[3] = 4; a[4] = 5; a[5] = 6;
        a[6] = 7; a[7] = 8; a[8] = 9;
        a[9] = 10; a[10] = 11;

        DENSE_TO_DENSE_ADD_SSE2_NOCACHE(a, b, a, size, lambda);
        for (i = 0; i < size; i++) {
            TEST_ASSERT(a[i] == a2[i] + b[i] * lambda);
            if (a[i] != a2[i] + b[i] * lambda) {
                LPERROR(a[i] << " != " << a2[i] + b[i] * lambda);
            }
        }

        a[0] = 1; a[1] = 0.001; a[2] = 10000;
        a[3] = 4; a[4] = 5; a[5] = 6;
        a[6] = 7; a[7] = 8; a[8] = 9;
        a[9] = 10; a[10] = 11;

        DENSE_TO_DENSE_ADD_ABS_REL_AVX_CACHE(a, b, a, size, lambda, absTolerance, relTolerance);
        for (i = 0; i < size; i++) {
            TEST_ASSERT(a[i] == Numerical::stableAdd(a2[i], b[i] * lambda));
            if (a[i] != Numerical::stableAdd(a2[i], b[i] * lambda)) {
                LPERROR(a[i] << " != " << Numerical::stableAdd(a2[i], b[i] * lambda));
            }
        }

        a[0] = 1; a[1] = 0.001; a[2] = 10000;
        a[3] = 4; a[4] = 5; a[5] = 6;
        a[6] = 7; a[7] = 8; a[8] = 9;
        a[9] = 10; a[10] = 11;

        DENSE_TO_DENSE_ADD_ABS_REL_AVX_NOCACHE(a, b, a, size, lambda, absTolerance, relTolerance);
        for (i = 0; i < size; i++) {
            TEST_ASSERT(a[i] == Numerical::stableAdd(a2[i], b[i] * lambda));
            if (a[i] != Numerical::stableAdd(a2[i], b[i] * lambda)) {
                LPERROR(a[i] << " != " << Numerical::stableAdd(a2[i], b[i] * lambda));
            }
        }

        a[0] = 1; a[1] = 0.001; a[2] = 10000;
        a[3] = 4; a[4] = 5; a[5] = 6;
        a[6] = 7; a[7] = 8; a[8] = 9;
        a[9] = 10; a[10] = 11;

        DENSE_TO_DENSE_ADD_ABS_AVX_CACHE(a, b, a, size, lambda, absTolerance);
        for (i = 0; i < size; i++) {
            TEST_ASSERT(a[i] == Numerical::stableAddAbs(a2[i], b[i] * lambda));
            if (a[i] != Numerical::stableAddAbs(a2[i], b[i] * lambda)) {
                LPERROR(a[i] << " != " << Numerical::stableAddAbs(a2[i], b[i] * lambda));
            }
        }

        a[0] = 1; a[1] = 0.001; a[2] = 10000;
        a[3] = 4; a[4] = 5; a[5] = 6;
        a[6] = 7; a[7] = 8; a[8] = 9;
        a[9] = 10; a[10] = 11;

        DENSE_TO_DENSE_ADD_ABS_AVX_NOCACHE(a, b, a, size, lambda, absTolerance);
        for (i = 0; i < size; i++) {
            TEST_ASSERT(a[i] == Numerical::stableAddAbs(a2[i], b[i] * lambda));
            if (a[i] != Numerical::stableAddAbs(a2[i], b[i] * lambda)) {
                LPERROR(a[i] << " != " << Numerical::stableAddAbs(a2[i], b[i] * lambda));
            }
        }

        a[0] = 1; a[1] = 0.001; a[2] = 10000;
        a[3] = 4; a[4] = 5; a[5] = 6;
        a[6] = 7; a[7] = 8; a[8] = 9;
        a[9] = 10; a[10] = 11;

        DENSE_TO_DENSE_ADD_AVX_CACHE(a, b, a, size, lambda);
        for (i = 0; i < size; i++) {
            TEST_ASSERT(a[i] == a2[i] + b[i] * lambda);
            if (a[i] != a2[i] + b[i] * lambda) {
                LPERROR(a[i] << " != " << a2[i] + b[i] * lambda);
            }
        }

        a[0] = 1; a[1] = 0.001; a[2] = 10000;
        a[3] = 4; a[4] = 5; a[5] = 6;
        a[6] = 7; a[7] = 8; a[8] = 9;
        a[9] = 10; a[10] = 11;

        DENSE_TO_DENSE_ADD_AVX_NOCACHE(a, b, a, size, lambda);
        for (i = 0; i < size; i++) {
            TEST_ASSERT(a[i] == a2[i] + b[i] * lambda);
            if (a[i] != a2[i] + b[i] * lambda) {
                LPERROR(a[i] << " != " << a2[i] + b[i] * lambda);
            }
        }

        a[0] = 1; a[1] = 0.001; a[2] = 10000;
        a[3] = 4; a[4] = 5; a[5] = 6;
        a[6] = 7; a[7] = 8; a[8] = 9;
        a[9] = 10; a[10] = 11;

        ::denseToDenseAddAbsRel(a, b, a, size, lambda, absTolerance, relTolerance);
        for (i = 0; i < size; i++) {
            TEST_ASSERT(a[i] == Numerical::stableAdd(a2[i], b[i] * lambda));
            if (a[i] != Numerical::stableAdd(a2[i], b[i] * lambda)) {
                LPERROR(a[i] << " != " << Numerical::stableAdd(a2[i], b[i] * lambda));
            }
        }

        a[0] = 1; a[1] = 0.001; a[2] = 10000;
        a[3] = 4; a[4] = 5; a[5] = 6;
        a[6] = 7; a[7] = 8; a[8] = 9;
        a[9] = 10; a[10] = 11;

        ::denseToDenseAddAbs(a, b, a, size, lambda, absTolerance);
        for (i = 0; i < size; i++) {
            TEST_ASSERT(a[i] == Numerical::stableAddAbs(a2[i], b[i] * lambda));
            if (a[i] != Numerical::stableAddAbs(a2[i], b[i] * lambda)) {
                LPERROR(a[i] << " != " << Numerical::stableAddAbs(a2[i], b[i] * lambda));
            }
        }

        a[0] = 1; a[1] = 0.001; a[2] = 10000;
        a[3] = 4; a[4] = 5; a[5] = 6;
        a[6] = 7; a[7] = 8; a[8] = 9;
        a[9] = 10; a[10] = 11;

        ::denseToDenseAdd(a, b, a, size, lambda);
        for (i = 0; i < size; i++) {
            TEST_ASSERT(a[i] == a2[i] + b[i] * lambda);
            if (a[i] != a2[i] + b[i] * lambda) {
                LPERROR(a[i] << " != " << a2[i] + b[i] * lambda);
            }
        }
    }

    Numerical::AbsoluteTolerance = oldAbsTolerance;
    Numerical::RelativeTolerance = oldRelTolerance;

    ::release(a);
    ::release(a2);
    ::release(b);
    ::release(c);
}

void CoreTestSuite::generateAddVectorInput(double * a,
                                           double * b,
                                           unsigned int count) {
    srand(0);
    unsigned int index;
    for (index = 0; index < count; index++) {
        a[index] = (rand() % 1000000) / 10000.0;
        if (rand() % 2 == 0) {
            b[index] = (rand() % 1000000) / 10000.0;
        } else {
            b[index] = a[index] * -1.000001;
        }
    }
}

void CoreTestSuite::denseToDenseAddPerformance()
{
    int n = 5000000;
    int m = 1000;
    double * a = alloc<double, 32>(n);
    double * b = alloc<double, 32>(n);
    double * c = alloc<double, 32>(n);

    generateAddVectorInput(a, b, n);

    Numerical::Double oldAbsTolerance = Numerical::AbsoluteTolerance;
    Numerical::Double oldRelTolerance = Numerical::RelativeTolerance;

    double lambda = 1.0;
    double absTolerance = 0.00001;
    double relTolerance = 0.004;

    Numerical::AbsoluteTolerance = absTolerance;
    Numerical::RelativeTolerance = relTolerance;

    clock_t startTime, endTime;

    int repeat;

    LPINFO("Benchmarks for dense + dense");

    startTime = clock();
    for (repeat = 0; repeat < m; repeat++) {
        ::denseToDenseAdd(a, b, a, n, lambda);
    }
    endTime = clock();
    LPINFO("Dense + dense fast add, C-style: " << (endTime - startTime) / (double)CLOCKS_PER_SEC << " sec");

    startTime = clock();
    for (repeat = 0; repeat < m; repeat++) {
         DENSE_TO_DENSE_ADD_SSE2_CACHE(a, b, a, n, lambda);
    }
    endTime = clock();
    LPINFO("Dense + dense fast add, SSE2, cache: " << (endTime - startTime) / (double)CLOCKS_PER_SEC << " sec");

    startTime = clock();
    for (repeat = 0; repeat < m; repeat++) {
         DENSE_TO_DENSE_ADD_SSE2_NOCACHE(a, b, a, n, lambda);
    }
    endTime = clock();
    LPINFO("Dense + dense fast add, SSE2, without cache: " << (endTime - startTime) / (double)CLOCKS_PER_SEC << " sec");

    startTime = clock();
    for (repeat = 0; repeat < m; repeat++) {
         DENSE_TO_DENSE_ADD_AVX_CACHE(a, b, a, n, lambda);
    }
    endTime = clock();
    LPINFO("Dense + dense fast add, AVX, cache: " << (endTime - startTime) / (double)CLOCKS_PER_SEC << " sec");

    startTime = clock();
    for (repeat = 0; repeat < m; repeat++) {
         DENSE_TO_DENSE_ADD_AVX_NOCACHE(a, b, a, n, lambda);
    }
    endTime = clock();
    LPINFO("Dense + dense fast add, AVX, without cache: " << (endTime - startTime) / (double)CLOCKS_PER_SEC << " sec");

////////////////////////////////////////////////
    LPINFO("--------------------------------------");

    startTime = clock();
    for (repeat = 0; repeat < m; repeat++) {
        ::denseToDenseAddAbs(a, b, a, n, lambda, absTolerance);
    }
    endTime = clock();
    LPINFO("Dense + dense abs add, C-style: " << (endTime - startTime) / (double)CLOCKS_PER_SEC << " sec");

    startTime = clock();
    for (repeat = 0; repeat < m; repeat++) {
        DENSE_TO_DENSE_ADD_ABS_SSE2_CACHE(a, b, a, n, lambda, absTolerance);
    }
    endTime = clock();
    LPINFO("Dense + dense abs add, SSE2, cache: " << (endTime - startTime) / (double)CLOCKS_PER_SEC << " sec");

    startTime = clock();
    for (repeat = 0; repeat < m; repeat++) {
        DENSE_TO_DENSE_ADD_ABS_SSE2_NOCACHE(a, b, a, n, lambda, absTolerance);
    }
    endTime = clock();
    LPINFO("Dense + dense abs add, SSE2, without cache: " << (endTime - startTime) / (double)CLOCKS_PER_SEC << " sec");

    startTime = clock();
    for (repeat = 0; repeat < m; repeat++) {
        DENSE_TO_DENSE_ADD_ABS_AVX_CACHE(a, b, a, n, lambda, absTolerance);
    }
    endTime = clock();
    LPINFO("Dense + dense abs add, AVX, cache: " << (endTime - startTime) / (double)CLOCKS_PER_SEC << " sec");

    startTime = clock();
    for (repeat = 0; repeat < m; repeat++) {
        DENSE_TO_DENSE_ADD_ABS_AVX_NOCACHE(a, b, a, n, lambda, absTolerance);
    }
    endTime = clock();
    LPINFO("Dense + dense abs add, AVX, without cache: " << (endTime - startTime) / (double)CLOCKS_PER_SEC << " sec");

////////////////////////////////////////////////
    LPINFO("--------------------------------------");

    startTime = clock();
    for (repeat = 0; repeat < m; repeat++) {
        ::denseToDenseAddAbsRel(a, b, a, n, lambda, absTolerance, relTolerance);
    }
    endTime = clock();
    LPINFO("Dense + dense abs-rel add, C-style: " << (endTime - startTime) / (double)CLOCKS_PER_SEC << " sec");

    startTime = clock();
    for (repeat = 0; repeat < m; repeat++) {
        DENSE_TO_DENSE_ADD_ABS_REL_SSE2_CACHE(a, b, a, n, lambda, absTolerance, relTolerance);
    }
    endTime = clock();
    LPINFO("Dense + dense abs-rel add, SSE2, cache: " << (endTime - startTime) / (double)CLOCKS_PER_SEC << " sec");

    startTime = clock();
    for (repeat = 0; repeat < m; repeat++) {
        DENSE_TO_DENSE_ADD_ABS_REL_SSE2_NOCACHE(a, b, a, n, lambda, absTolerance, relTolerance);
    }
    endTime = clock();
    LPINFO("Dense + dense abs-rel add, SSE2, without cache: " << (endTime - startTime) / (double)CLOCKS_PER_SEC << " sec");

    startTime = clock();
    for (repeat = 0; repeat < m; repeat++) {
        DENSE_TO_DENSE_ADD_ABS_REL_AVX_CACHE(a, b, a, n, lambda, absTolerance, relTolerance);
    }
    endTime = clock();
    LPINFO("Dense + dense abs-rel add, AVX, cache: " << (endTime - startTime) / (double)CLOCKS_PER_SEC << " sec");

    startTime = clock();
    for (repeat = 0; repeat < m; repeat++) {
        DENSE_TO_DENSE_ADD_ABS_REL_AVX_NOCACHE(a, b, a, n, lambda, absTolerance, relTolerance);
    }
    endTime = clock();
    LPINFO("Dense + dense abs-rel add, AVX, without cache: " << (endTime - startTime) / (double)CLOCKS_PER_SEC << " sec");


    Numerical::AbsoluteTolerance = oldAbsTolerance;
    Numerical::RelativeTolerance = oldRelTolerance;

    ::release(a);
    ::release(b);
    ::release(c);
}
