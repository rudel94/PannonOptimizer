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
 * @file x86.h This file contains the API of the ArchitectureX86 class.
 * @author Jozsef Smidla
 */

#ifndef X86_H
#define X86_H

#include <globals.h>
#include <utils/architecture.h>
#include <cstring>
#include <utils/platform.h>
#include <utils/primitives.h>

#ifdef PLATFORM_X86

extern "C" void setFPUTo64Bit32();

extern "C" void _cpuinfo_64_linux(unsigned int eax,
                                  unsigned int ebx,
                                  unsigned int ecx,
                                  unsigned int edx,
                                  unsigned int * results);

extern "C" void cpuinfo_32(unsigned int eax,
                            unsigned int ebx,
                            unsigned int ecx,
                            unsigned int edx,
                            unsigned int * results);

extern "C" int cpuinfo_supported_32();

extern "C" int _avx_enabled_by_64();

extern "C" int _avx_enabled_by_32();
extern "C" int avx_enabled_by_32();

/***********************************************
 * Memcpy
 **********************************************/

extern "C" void * _memcpy_sse2_64_linux_cread_cwrite(void * dest, const void * src, unsigned long count);
extern "C" void * _memcpy_sse4_1_64_linux_ntread_ntwrite(void * dest, const void * src, unsigned long count);

/***********************************************
 * Dot product
 **********************************************/

extern "C" double _denseToDenseDotProduct_unstable_SSE2_64_linux(const double * vec1,
                                                                 const double * vec2,
                                                                 size_t count);

extern "C" double _denseToDenseDotProduct_unstable_AVX_64_linux(const double * vec1,
                                                                const double * vec2,
                                                                size_t count);

extern "C" double _denseToSparseDotProduct_unstable_SSE2_64_linux(const double * dense,
                                                                  const double * sparse,
                                                                  const unsigned int * indices,
                                                                  size_t count);

extern "C" double _denseToSparseDotProduct_unstable_AVX_64_linux(const double * dense,
                                                                  const double * sparse,
                                                                  const unsigned int * indices,
                                                                  size_t count);

extern "C" double _denseToSparseDotProduct_stable_SSE2_64_linux(const double * dense,
                                                                const double * sparse,
                                                                const unsigned int * indices,
                                                                size_t count,
                                                                double * negPtr);
extern "C" double _denseToSparseDotProduct_stable_AVX_64_linux(const double * dense,
                                                               const double * sparse,
                                                               const unsigned int * indices,
                                                               size_t count,
                                                               double * negPtr);

/***********************************************
 * Dense to dense add
 **********************************************/

extern "C" void _denseToDenseAddAbsRelSSE2_cache_64_linux(
        const double * a,
        const double * b,
        double * c,
        size_t count,
        double lambda,
        double absTolerance,
        double relTolerance);

extern "C" void _denseToDenseAddAbsRelSSE2_nocache_64_linux(
        const double * a,
        const double * b,
        double * c,
        size_t count,
        double lambda,
        double absTolerance,
        double relTolerance);

extern "C" void _denseToDenseAddAbsSSE2_cache_64_linux(
        const double * a,
        const double * b,
        double * c,
        size_t count,
        double lambda,
        double absTolerance);

extern "C" void _denseToDenseAddAbsSSE2_nocache_64_linux(
        const double * a,
        const double * b,
        double * c,
        size_t count,
        double lambda,
        double absTolerance);

extern "C" void _denseToDenseAddSSE2_cache_64_linux(
        const double * a,
        const double * b,
        double * c,
        size_t count,
        double lambda);

extern "C" void _denseToDenseAddSSE2_nocache_64_linux(
        const double * a,
        const double * b,
        double * c,
        size_t count,
        double lambda);

extern "C" void _denseToDenseAddAbsRelAVX_cache_64_linux(
        const double * a,
        const double * b,
        double * c,
        size_t count,
        double lambda,
        double absTolerance,
        double relTolerance);

extern "C" void _denseToDenseAddAbsRelAVX_nocache_64_linux(
        const double * a,
        const double * b,
        double * c,
        size_t count,
        double lambda,
        double absTolerance,
        double relTolerance);

extern "C" void _denseToDenseAddAbsAVX_cache_64_linux(
        const double * a,
        const double * b,
        double * c,
        size_t count,
        double lambda,
        double absTolerance);

extern "C" void _denseToDenseAddAbsAVX_nocache_64_linux(
        const double * a,
        const double * b,
        double * c,
        size_t count,
        double lambda,
        double absTolerance);

extern "C" void _denseToDenseAddAVX_cache_64_linux(
        const double * a,
        const double * b,
        double * c,
        size_t count,
        double lambda);

extern "C" void _denseToDenseAddAVX_nocache_64_linux(
        const double * a,
        const double * b,
        double * c,
        size_t count,
        double lambda);

// TODO: lehet hogy 32 bit alatt ugyanugy nez majd ki linux es windows alatt
// ezt meg ellenorizni kell, ha igaz, akkor egyszerubbe valik a kod kicsit



#ifdef ENVIRONMENT_32

/*****************************************
 * UNIX, 32 bit
 ****************************************/

#define CPUID(eax, ebx, ecx, edx, results) \
    cpuinfo_32(eax, ebx, ecx, edx, results);
#define SET_FPU_TO_64 setFPUTo64Bit32
#define AVX_ENABLED_BY_OS avx_enabled_by_32
/*#define MEMCPY_CACHE_SSE2 _memcpy_sse2_32_cread_cwrite
#define MEMCPY_NO_CACHE_SSE4_1 _memcpy_sse4_1_32_ntread_ntwrite

#define DENSE_TO_DENSE_UNSTABLE_SSE2 _denseToDenseDotProduct_unstable_SSE2_32
#define DENSE_TO_DENSE_UNSTABLE_AVX _denseToDenseDotProduct_unstable_AVX_32
*/
#define MEMCPY_CACHE_SSE2 ::memcpy
#define MEMCPY_NO_CACHE_SSE4_1 ::memcpy

#define DENSE_TO_DENSE_DOTPRODUCT_UNSTABLE_SSE2 ::denseToDenseDotProductUnstable
#define DENSE_TO_DENSE_DOTPRODUCT_UNSTABLE_AVX ::denseToDenseDotProductUnstable

#define DENSE_TO_SPARSE_DOTPRODUCT_UNSTABLE_SSE2 ::denseToSparseDotProductUnstable
#define DENSE_TO_SPARSE_DOTPRODUCT_UNSTABLE_AVX ::denseToSparseDotProductUnstable

#define DENSE_TO_SPARSE_DOTPRODUCT_STABLE_SSE2 ::denseToSparseDotProductStable
#define DENSE_TO_SPARSE_DOTPRODUCT_STABLE_AVX ::denseToSparseDotProductStable

#define DENSE_TO_DENSE_ADD_ABS_REL_SSE2_CACHE ::denseToDenseAddAbsRel
#define DENSE_TO_DENSE_ADD_ABS_REL_SSE2_NOCACHE ::denseToDenseAddAbsRel
#define DENSE_TO_DENSE_ADD_ABS_SSE2_CACHE ::denseToDenseAddAbs
#define DENSE_TO_DENSE_ADD_ABS_SSE2_NOCACHE ::denseToDenseAddAbs
#define DENSE_TO_DENSE_ADD_SSE2_CACHE ::denseToDenseAdd
#define DENSE_TO_DENSE_ADD_SSE2_NOCACHE ::denseToDenseAdd
#define DENSE_TO_DENSE_ADD_ABS_REL_AVX_CACHE ::denseToDenseAddAbsRel
#define DENSE_TO_DENSE_ADD_ABS_REL_AVX_NOCACHE ::denseToDenseAddAbsRel
#define DENSE_TO_DENSE_ADD_ABS_AVX_CACHE ::denseToDenseAddAbs
#define DENSE_TO_DENSE_ADD_ABS_AVX_NOCACHE ::denseToDenseAddAbs
#define DENSE_TO_DENSE_ADD_AVX_CACHE ::denseToDenseAdd
#define DENSE_TO_DENSE_ADD_AVX_NOCACHE ::denseToDenseAdd
#else
#ifdef UNIX

/*****************************************
 * UNIX, 64 bit
 ****************************************/

#define CPUID(eax, ebx, ecx, edx, results) \
    _cpuinfo_64_linux(eax, ebx, ecx, edx, results);
#define SET_FPU_TO_64 doNothing_seriously_nothing
#define AVX_ENABLED_BY_OS _avx_enabled_by_64
#define MEMCPY_CACHE_SSE2 _memcpy_sse2_64_linux_cread_cwrite
#define MEMCPY_NO_CACHE_SSE4_1 _memcpy_sse4_1_64_linux_ntread_ntwrite

#define DENSE_TO_DENSE_DOTPRODUCT_UNSTABLE_SSE2 _denseToDenseDotProduct_unstable_SSE2_64_linux
#define DENSE_TO_DENSE_DOTPRODUCT_UNSTABLE_AVX _denseToDenseDotProduct_unstable_AVX_64_linux

#define DENSE_TO_SPARSE_DOTPRODUCT_UNSTABLE_SSE2 _denseToSparseDotProduct_unstable_SSE2_64_linux
#define DENSE_TO_SPARSE_DOTPRODUCT_UNSTABLE_AVX _denseToSparseDotProduct_unstable_AVX_64_linux

#define DENSE_TO_SPARSE_DOTPRODUCT_STABLE_SSE2 _denseToSparseDotProduct_stable_SSE2_64_linux
#define DENSE_TO_SPARSE_DOTPRODUCT_STABLE_AVX _denseToSparseDotProduct_stable_AVX_64_linux

#define DENSE_TO_DENSE_ADD_ABS_REL_SSE2_CACHE ::_denseToDenseAddAbsRelSSE2_cache_64_linux
#define DENSE_TO_DENSE_ADD_ABS_REL_SSE2_NOCACHE ::_denseToDenseAddAbsRelSSE2_nocache_64_linux
#define DENSE_TO_DENSE_ADD_ABS_SSE2_CACHE ::_denseToDenseAddAbsSSE2_cache_64_linux
#define DENSE_TO_DENSE_ADD_ABS_SSE2_NOCACHE ::_denseToDenseAddAbsSSE2_nocache_64_linux
#define DENSE_TO_DENSE_ADD_SSE2_CACHE ::_denseToDenseAddSSE2_cache_64_linux
#define DENSE_TO_DENSE_ADD_SSE2_NOCACHE ::_denseToDenseAddSSE2_nocache_64_linux
#define DENSE_TO_DENSE_ADD_ABS_REL_AVX_CACHE ::_denseToDenseAddAbsRelAVX_cache_64_linux
#define DENSE_TO_DENSE_ADD_ABS_REL_AVX_NOCACHE ::_denseToDenseAddAbsRelAVX_nocache_64_linux
#define DENSE_TO_DENSE_ADD_ABS_AVX_CACHE ::_denseToDenseAddAbsAVX_cache_64_linux
#define DENSE_TO_DENSE_ADD_ABS_AVX_NOCACHE ::_denseToDenseAddAbsAVX_nocache_64_linux
#define DENSE_TO_DENSE_ADD_AVX_CACHE ::_denseToDenseAddAVX_cache_64_linux
#define DENSE_TO_DENSE_ADD_AVX_NOCACHE ::_denseToDenseAddAVX_nocache_64_linux

#else
#ifdef WIN32
/*****************************************
 * Windows, 64 bit
 ****************************************/
#define CPUID(eax, ebx, ecx, edx, results) \
//    _cpuinfo_64_windows(eax, ebx, ecx, edx, results);
#define SET_FPU_TO_64 doNothing_seriously_nothing
#define AVX_ENABLED_BY_OS _avx_enabled_by_64
/*#define MEMCPY_CACHE_SSE2 _memcpy_sse2_64_windows_cread_cwrite
#define MEMCPY_NO_CACHE_SSE4_1 _memcpy_sse4_1_64_windows_ntread_ntwrite

#define DENSE_TO_DENSE_UNSTABLE_SSE2 _denseToDenseDotProduct_unstable_SSE2_64_windows
#define DENSE_TO_DENSE_UNSTABLE_AVX _denseToDenseDotProduct_unstable_AVX_64_windows
*/

#define MEMCPY_CACHE_SSE2 ::memcpy
#define MEMCPY_NO_CACHE_SSE4_1 ::memcpy

#define DENSE_TO_DENSE_DOTPRODUCT_UNSTABLE_SSE2 ::denseToDenseDotProductUnstable
#define DENSE_TO_DENSE_DOTPRODUCT_UNSTABLE_AVX ::denseToDenseDotProductUnstable

#define DENSE_TO_SPARSE_DOTPRODUCT_UNSTABLE_SSE2 ::denseToSparseDotProductUnstable
#define DENSE_TO_SPARSE_DOTPRODUCT_UNSTABLE_AVX ::denseToSparseDotProductUnstable

#define DENSE_TO_SPARSE_DOTPRODUCT_STABLE_SSE2 ::denseToSparseDotProductStable
#define DENSE_TO_SPARSE_DOTPRODUCT_STABLE_AVX ::denseToSparseDotProductStable

#define DENSE_TO_DENSE_ADD_ABS_REL_SSE2_CACHE ::denseToDenseAddAbsRel
#define DENSE_TO_DENSE_ADD_ABS_REL_SSE2_NOCACHE ::denseToDenseAddAbsRel
#define DENSE_TO_DENSE_ADD_ABS_SSE2_CACHE ::denseToDenseAddAbs
#define DENSE_TO_DENSE_ADD_ABS_SSE2_NOCACHE ::denseToDenseAddAbs
#define DENSE_TO_DENSE_ADD_SSE2_CACHE ::denseToDenseAdd
#define DENSE_TO_DENSE_ADD_SSE2_NOCACHE ::denseToDenseAdd
#define DENSE_TO_DENSE_ADD_ABS_REL_AVX_CACHE ::denseToDenseAddAbsRel
#define DENSE_TO_DENSE_ADD_ABS_REL_AVX_NOCACHE ::denseToDenseAddAbsRel
#define DENSE_TO_DENSE_ADD_ABS_AVX_CACHE ::denseToDenseAddAbs
#define DENSE_TO_DENSE_ADD_ABS_AVX_NOCACHE ::denseToDenseAddAbs
#define DENSE_TO_DENSE_ADD_AVX_CACHE ::denseToDenseAdd
#define DENSE_TO_DENSE_ADD_AVX_NOCACHE ::denseToDenseAdd

#else
#ifdef MACINTOSH

/*****************************************
 * Macintosh, 64 bit
 ****************************************/
#define CPUID(eax, ebx, ecx, edx, results) \
    //_cpuinfo_64_macintosh(eax, ebx, ecx, edx, results);
#define SET_FPU_TO_64 doNothing_seriously_nothing
#define AVX_ENABLED_BY_OS doNothing_seriously_nothing
/*#define MEMCPY_CACHE_SSE2 _memcpy_sse2_64_windows_cread_cwrite
#define MEMCPY_NO_CACHE_SSE4_1 _memcpy_sse4_1_64_windows_ntread_ntwrite

#define DENSE_TO_DENSE_UNSTABLE_SSE2 _denseToDenseDotProduct_unstable_SSE2_64_windows
#define DENSE_TO_DENSE_UNSTABLE_AVX _denseToDenseDotProduct_unstable_AVX_64_windows
*/

#define MEMCPY_CACHE_SSE2 ::memcpy
#define MEMCPY_NO_CACHE_SSE4_1 ::memcpy

#define DENSE_TO_DENSE_DOTPRODUCT_UNSTABLE_SSE2 ::denseToDenseDotProductUnstable
#define DENSE_TO_DENSE_DOTPRODUCT_UNSTABLE_AVX ::denseToDenseDotProductUnstable

#define DENSE_TO_SPARSE_DOTPRODUCT_UNSTABLE_SSE2 ::denseToSparseDotProductUnstable
#define DENSE_TO_SPARSE_DOTPRODUCT_UNSTABLE_AVX ::denseToSparseDotProductUnstable

#define DENSE_TO_SPARSE_DOTPRODUCT_STABLE_SSE2 ::denseToSparseDotProductStable
#define DENSE_TO_SPARSE_DOTPRODUCT_STABLE_AVX ::denseToSparseDotProductStable

#define DENSE_TO_DENSE_ADD_ABS_REL_SSE2_CACHE ::denseToDenseAddAbsRel
#define DENSE_TO_DENSE_ADD_ABS_REL_SSE2_NOCACHE ::denseToDenseAddAbsRel
#define DENSE_TO_DENSE_ADD_ABS_SSE2_CACHE ::denseToDenseAddAbs
#define DENSE_TO_DENSE_ADD_ABS_SSE2_NOCACHE ::denseToDenseAddAbs
#define DENSE_TO_DENSE_ADD_SSE2_CACHE ::denseToDenseAdd
#define DENSE_TO_DENSE_ADD_SSE2_NOCACHE ::denseToDenseAdd
#define DENSE_TO_DENSE_ADD_ABS_REL_AVX_CACHE ::denseToDenseAddAbsRel
#define DENSE_TO_DENSE_ADD_ABS_REL_AVX_NOCACHE ::denseToDenseAddAbsRel
#define DENSE_TO_DENSE_ADD_ABS_AVX_CACHE ::denseToDenseAddAbs
#define DENSE_TO_DENSE_ADD_ABS_AVX_NOCACHE ::denseToDenseAddAbs
#define DENSE_TO_DENSE_ADD_AVX_CACHE ::denseToDenseAdd
#define DENSE_TO_DENSE_ADD_AVX_NOCACHE ::denseToDenseAdd

#else
#error Unindentified platform!
#endif

#endif
#endif
#endif

/**
 * This class implements detection of properties of x86 architectures.
 * It is able to execute architecture-specific functions for high-performance operations.
 */
class ArchitectureX86: public Architecture {
public:

    /**
     * Default constructor of the ArchitectureX86 class.
     *
     * @constructor
     */
    ArchitectureX86();

    /**
     * Detects the properties of the x86 architecture.
     * Gathers information about the CPU, cache and computing features of the system.
     */
    void detect();

    /**
     * Custom X86 architecture specific custom function for effective dense vector-dense vector addition.
     * This function also returns the nonzero count of the result vector.
     *
     * @return The result vector and its nonzero count in a specific variable type.
     * For details, see Architecture::AddVecDenseToDense_NonzCount.
     */
    AddVecDenseToDense_NonzCount getAddVecDenseToDense_NonzCount() const;

    /**
     * Custom X86 architecture specific custom function for effective dense vector-dense vector addition.
     *
     * @return The result vector in a specific variable type.
     * For details, see Architecture::AddVecDenseToDense.
     */
    AddVecDenseToDense getAddVecDenseToDense() const;

    /**
     * Loads an X86 specific parameter file.
     */
    void loadParameters();

    /**
     * Generates an X86 specific parameter file.
     */
    void generateParameterFile() const;

protected:


    std::string m_vendor;

    unsigned int m_largestFunction;

    unsigned int m_largestExtendedFunction;

    /**
     * Struct for storing the values of registers.
     */
    struct Registers {
        unsigned int m_eax;
        unsigned int m_ebx;
        unsigned int m_ecx;
        unsigned int m_edx;
    };

    /**
     * Returns the bits of the pattern between selected indices.
     *
     * @param pattern The pattern.
     * @param start The start index of the returned bits.
     * @param end The end index of the returned bits.
     * @return The bits of pattern between start and end.
     */
    unsigned int getBits(unsigned int pattern,
                         unsigned int start,
                         unsigned int end);

    bool cpuidSupported();

    /**
     * Queries a cpuid command from the processor.
     *
     * @param args The register values the return of the cpuid command to be extracted to.
     * @return The register values containing the input of the cpuid command.
     */
    Registers cpuid(Registers args) const;

    /**
     * Queries and returns the cache info of a cache with specific ID.
     * The cache info is stored in the cache list of the architecture class.
     *
     * @see Architecture::m_caches
     * @param cacheId The ID of the cache to be queried.
     */
    void loadCacheInfoIntel(int cacheId);

    /**
     * Returns the number of caches in the current computer.
     *
     * @return The number of caches.
     */
    unsigned int getCacheCountIntel() const;

    /**
     * Gets the name of the CPU from the processor.
     * This gets stored in the m_cpuName private variable.
     *
     * @see Architecture::m_cpuName
     */
    void setCPUData();

    /**
     * Checks if the feature with the given bit exists, and adds it to the feature list if it is so.
     *
     * @param reg The register address with the given feature.
     * @param bit The bit id of the feature.
     * @param name The name of the feature.
     */
    void loadFeature(unsigned int reg, unsigned int bit,
                     const std::string & name);

    /**
     * Queries which features do the processor supports.
     */
    void setFeatureList();

    void setCPUTopologyData();

    /**
     * Queries the cache count and the cache data from each cache from the processor.
     */
    void setCacheData();

    void setCacheDataIntel();

    void setCacheDataAMD();

    unsigned int getAMDL2L3CacheAssociativity(unsigned int pattern);

    void showAMDCacheInfo(const Cache & info);

    /**
     * Queries the memory size from the system.
     */
    void setMemoryData();

    void setPrimitives();

    void setMemcpy();

    void setDotProduct();

    void setAdd();

};

#endif

#endif // X86_H
