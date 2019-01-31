/*
 * Compile-time configuration for ramfuck.
 *
 * Override defaults by modifying this file or using `-D` compile-time flags.
 */
#ifndef DEFINES_H_INCLUDED
#define DEFINES_H_INCLUDED

/* ADDR_BITS (32 or 64) specifies the maximum supported address size in bits */
#ifndef ADDR_BITS
# define ADDR_BITS 64
#endif

/* Define NO_64BIT_VALUES to exclude s64/u64 value support (unimplemented) */
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
#if ADDR_BITS == 64
# ifdef NO_64BIT_VALUES
#  error "ADDR_BITS=64 unsupported when NO_64BIT_VALUES is defined"
# endif
# define _LARGE_FILE 1
# define _LARGEFILE_SOURCE 1
# ifndef _FILE_OFFSET_BITS
#   define _FILE_OFFSET_BITS 64
# endif
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
#else
# error "unsupported ADDR_BITS (must be 32 or 64)"
#endif

#endif
