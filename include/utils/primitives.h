#ifndef PRIMITIVES_H
#define PRIMITIVES_H

#include <globals.h>
#include <utils/numerical.h>

extern bool __prim_debug;

extern Numerical::Double denseToDenseDotProductUnstable(const Numerical::Double * __restrict__ vec1,
                                                        const Numerical::Double * __restrict__ vec2,
                                                        size_t count);

extern Numerical::Double denseToSparseDotProductUnstable(const Numerical::Double * __restrict__ dense,
                                                         const Numerical::Double * __restrict__ sparse,
                                                         const unsigned int * __restrict__ indices,
                                                         size_t count);

extern Numerical::Double denseToDenseDotProductStable(const Numerical::Double * __restrict__ vec1,
                                                      const Numerical::Double * __restrict__ vec2,
                                                      size_t count,
                                                      Numerical::Double * __restrict__ negPtr);

extern Numerical::Double denseToSparseDotProductStable(const Numerical::Double * __restrict__ dense,
                                                       const Numerical::Double * __restrict__ sparse,
                                                       const unsigned int * __restrict__ indices,
                                                       size_t count,
                                                       Numerical::Double * __restrict__ negPtr);


#endif // PRIMITIVES_H
