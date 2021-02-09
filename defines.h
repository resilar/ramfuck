/*
 * Compile-time configuration for ramfuck.
 *
 * Override defaults by modifying this file or using `-D` compile-time flags.
 */
#ifndef DEFINES_H_INCLUDED
#define DEFINES_H_INCLUDED

/* Define ADDR_BITS to specify the maximum address size in bits (32 or 64) */
#if 0
# define ADDR_BITS 64
#endif

/* Define NO_64BIT_VALUES to exclude s64/u64 value support */
#if 0
# define NO_64BIT_VALUES
#endif

/* Define NO_FLOAT_VALUES to exclude f32/f64 value support */
#if 0
# define NO_FLOAT_VALUES
#endif

/*
 * Internal definitions and configuration sanity checking.
 */
#ifndef ADDR_BITS
# ifdef NO_64BIT_VALUES
#  define ADDR_BITS 32
# else
#  define ADDR_BITS 64
# endif
#elif ADDR_BITS == 64
# ifdef NO_64_BIT_VALUES
#  error "ADDR_BITS=64 unsupported when NO_64BIT_VALUES is defined"
# endif
# define _LARGE_FILE 1
# define _LARGEFILE_SOURCE 1
# ifndef _FILE_OFFSET_BITS
#   define _FILE_OFFSET_BITS 64
# endif
#elif ADDR_BITS != 32
# error "unsupported ADDR_BITS (expected 32 or 64)"
#endif

#include <inttypes.h>
#include <stdint.h>

#if ADDR_BITS == 32
typedef uint32_t addr_t;
# define ADDR_MAX UINT32_MAX
# define PRIaddr PRIx32
# define PRIaddru PRIu32
# define SCNaddr SCNx32
#elif ADDR_BITS == 64
typedef uint64_t addr_t;
# define ADDR_MAX UINT64_MAX
# define PRIaddr PRIx64
# define PRIaddru PRIu64
# define SCNaddr SCNx64
#endif

#ifdef NO_64BIT_VALUES
typedef int32_t smax_t;
typedef uint32_t umax_t;
# define PRIsmax PRId32
# define PRIumax PRIu32
# define PRIxmax PRIx32
# define SMAX_MIN INT32_MIN
# define SMAX_MAX INT32_MAX
# define UMAX_MAX UINT32_MAX
#else
typedef int64_t smax_t;
typedef uint64_t umax_t;
# define PRIsmax PRId64
# define PRIumax PRIu64
# define PRIxmax PRIx64
# define SMAX_MIN INT64_MIN
# define SMAX_MAX INT64_MAX
# define UMAX_MAX UINT64_MAX
#endif

#endif
