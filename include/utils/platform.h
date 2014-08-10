/**
 * @file platform.h This file contains macros which identify the current platform.
 * @author Jozsef Smidla
 */

#ifndef PLATFORM_H
#define PLATFORM_H

//#include <cstdint>

/**
 * @def WIN32
 *
 * This macro is defined if the source is compiled into Windows platforms.
 */

/**
 * @def UNIX
 *
 * This macro is defined if the source is compiled into Unix compatible platforms.
 */

#ifdef _WIN32
#ifndef WIN32
#define WIN32
#endif
#endif

#ifdef __WIN32
#ifndef WIN32
#define WIN32
#endif
#endif

#ifdef __WIN32__
#ifndef WIN32
#define WIN32
#endif
#endif

#ifdef __unix__
#ifndef UNIX
#define UNIX
#endif
#endif

#ifdef __unix
#ifndef UNIX
#define UNIX
#endif
#endif

#ifdef __linux__
#ifndef UNIX
#define UNIX
#endif
#endif

#ifdef __linux
#ifndef UNIX
#define UNIX
#endif
#endif

#ifdef __GNUC__
    #define GNU_COMPILER
    #if __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 8)
    #error Too old gcc version. Use unless 4.8.0. or higher!
    #endif

#endif

#ifdef __INTEL_COMPILER
    #define INTEL_COMPILER
#endif

#ifdef GNU_COMPILER
    #define ALWAYS_INLINE __attribute__((always_inline))
#else
    #define ALWAYS_INLINE inline
#endif

/************************************************
 * ARCHITECTURES
 ***********************************************/

/*#if INTPTR_MAX == INT32_MAX
    #define ENVIRONMENT_32
#elif INTPTR_MAX == INT64_MAX
    #define ENVIRONMENT_64
#else
    #error "Environment not 32 or 64-bit."
#endif
*/

#if __GNUC__
#if __x86_64__ || __ppc64__
#define ENVIRONMENT_64
#else
#define ENVIRONMENT_32
#endif
#endif

// source: http://sourceforge.net/p/predef/wiki/Architectures/

#ifdef __x86_64
#define     PLATFORM_X86
#endif

#ifdef __x86_64__
#define     PLATFORM_X86
#endif

#ifdef i386
#define     PLATFORM_X86
#endif

#ifdef __i386
#define     PLATFORM_X86
#endif

#ifdef _i386_
#define     PLATFORM_X86
#endif

#ifdef __arm__
#define     PLATFORM_ARM
#endif

#ifdef __powerpc
#define     PLATFORM_POWERPC
#endif

#ifdef __powerpc__
#define     PLATFORM_POWERPC
#endif

#ifdef __POWERPC__
#define     PLATFORM_POWERPC
#endif

#ifdef __sparc__
#define     PLATFORM_SPARC
#endif


#endif // PLATFORM_H
