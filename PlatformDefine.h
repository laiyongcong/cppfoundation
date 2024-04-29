#pragma once

#define CPPFD_PLATFORM_WIN32 (1)
#define CPPFD_PLATFORM_LINUX (2)
#define CPPFD_PLATFORM_APPLE (3)
#define CPPFD_PLATFORM_APPLE_IOS (4)
#define CPPFD_PLATFORM_ANDROID (5)

#define CPPFD_COMPILER_MSVC 1
#define CPPFD_COMPILER_GNUC 2
#define CPPFD_COMPILER_BORL 3
#define CPPFD_COMPILER_WINSCW 4
#define CPPFD_COMPILER_GCCE 5
#define CPPFD_COMPILER_CLANG 6

#define CPPFD_ARCHITECTURE_32 1
#define CPPFD_ARCHITECTURE_64 2

#if defined(__GCCE__)
#  define CPPFD_COMPILER CPPFD_COMPILER_GCCE
#  define CPPFD_COMP_VER _MSC_VER
//# include <staticlibinit_gcce.h> // This is a GCCE toolchain workaround needed when compiling with GCCE
#elif defined(__WINSCW__)
#  define CPPFD_COMPILER CPPFD_COMPILER_WINSCW
#  define CPPFD_COMP_VER _MSC_VER
#elif defined(_MSC_VER)
#  define CPPFD_COMPILER CPPFD_COMPILER_MSVC
#  define CPPFD_COMP_VER _MSC_VER
#elif defined(__clang__)
#  define CPPFD_COMPILER CPPFD_COMPILER_CLANG
#  define CPPFD_COMP_VER (((__clang_major__)*100) + (__clang_minor__ * 10) + __clang_patchlevel__)
#elif defined(__GNUC__)
#  define CPPFD_COMPILER CPPFD_COMPILER_GNUC
#  define CPPFD_COMP_VER (((__GNUC__)*100) + (__GNUC_MINOR__ * 10) + __GNUC_PATCHLEVEL__)
#elif defined(__BORLANDC__)
#  define CPPFD_COMPILER CPPFD_COMPILER_BORL
#  define CPPFD_COMP_VER __BCPLUSPLUS__
#  define __FUNCTION__ __FUNC__
#else
#  pragma error "No known compiler. Abort! Abort!"
#endif

#if CPPFD_COMPILER == CPPFD_COMPILER_MSVC
#  if CPPFD_COMP_VER >= 1200
#    define FORCEINLINE __forceinline
#  endif
#elif defined(__MINGW32__)
#  if !defined(FORCEINLINE)
#    define FORCEINLINE __inline
#  endif
#elif defined(__APPLE__)
#  if !defined(FORCEINLINE)
#    define FORCEINLINE inline __attribute__((always_inline))
#  endif
#elif defined(__ANDROID__)
#  if !defined(FORCEINLINE)
#    define FORCEINLINE inline __attribute__((always_inline))
#  endif
#else
#  define FORCEINLINE __inline
#endif

#if defined(__WIN32__) || defined(_WIN32)
#  define CPPFD_PLATFORM  CPPFD_PLATFORM_WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif  // !WIN32_LEAN_AND_MEAN
#elif defined(__APPLE_CC__)
// Device                                                     Simulator
// Both requiring OS version 4.0 or greater
#  if __ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__ >= 40000 || __IPHONE_OS_VERSION_MIN_REQUIRED >= 40000
#    define CPPFD_PLATFORM CPPFD_PLATFORM_APPLE_IOS
#  else
#    define CPPFD_PLATFORM CPPFD_PLATFORM_APPLE
#  endif
#elif defined(__ANDROID__)
#  define CPPFD_PLATFORM CPPFD_PLATFORM_ANDROID
#else
#  define CPPFD_PLATFORM CPPFD_PLATFORM_LINUX
#endif

#if defined(__x86_64__) || defined(_M_X64) || defined(__powerpc64__) || defined(__alpha__) || defined(__ia64__) || defined(__s390__) || defined(__s390x__)
#  define CPPFD_ARCH_TYPE CPPFD_ARCHITECTURE_64
#else
#  define CPPFD_ARCH_TYPE CPPFD_ARCHITECTURE_32
#endif


#if defined(_M_ARM) || defined(_M_ARM64) || defined(_M_ARM64EC)
#  define CPPFD_CPU_ARM_FAMILY (1)
#elif defined(_M_IX86) || defined(_M_X64)
#  define CPPFD_CPU_X86_FAMILY (1)
#endif
