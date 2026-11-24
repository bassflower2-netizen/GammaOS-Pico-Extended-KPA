/* Compatibility for the ARM EABI newlib used by the standalone Handy probe.
 * Only supply missing C99 format macros, and check the exact argument types.
 * Do not apply this header to the original Pico cores or Linux host build. */
#ifndef HANDY_NATIVE_INTTYPES_H
#define HANDY_NATIVE_INTTYPES_H
#include <stdint.h>
#include <inttypes.h>
#include <limits.h>
#if !defined(__arm__) || !defined(__ARM_EABI__) || UINTPTR_MAX != UINT32_MAX
#error "Handy native compatibility is only for 32-bit ARM EABI"
#endif
#if defined(__cplusplus)
#include <type_traits>
static_assert(std::is_same<int64_t, long long>::value, "ARM int64_t type changed");
static_assert(std::is_same<uint64_t, unsigned long long>::value, "ARM uint64_t type changed");
static_assert(std::is_same<uintptr_t, unsigned int>::value || std::is_same<uintptr_t, unsigned long>::value, "ARM uintptr_t type changed");
#else
_Static_assert(__builtin_types_compatible_p(int64_t, long long), "ARM int64_t type changed");
_Static_assert(__builtin_types_compatible_p(uint64_t, unsigned long long), "ARM uint64_t type changed");
_Static_assert(__builtin_types_compatible_p(uintptr_t, unsigned int) || __builtin_types_compatible_p(uintptr_t, unsigned long), "ARM uintptr_t type changed");
#endif
#ifndef PRId64
#define PRId64 "lld"
#endif
#ifndef PRIu64
#define PRIu64 "llu"
#endif
#ifndef PRIx64
#define PRIx64 "llx"
#endif
#ifndef PRIX64
#define PRIX64 "llX"
#endif
#ifndef PRIuPTR
#ifndef HANDY_PRIUPTR
#error "Configure the uintptr_t format with the compiler probe"
#endif
#define PRIuPTR HANDY_PRIUPTR
#endif
#endif
