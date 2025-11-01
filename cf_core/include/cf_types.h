/**
 * @file cf_types.h
 * @brief Common type definitions for CFramework
 * @version 1.0.0
 * @date 2025-10-29
 * @author CFramework Contributors
 *
 * @copyright Copyright (c) 2025 CFramework
 * Licensed under MIT License
 */

#ifndef CF_TYPES_H
#define CF_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

//==============================================================================
// BOOLEAN TYPE
//==============================================================================

// Use standard C99 bool from stdbool.h
// No need to redefine

//==============================================================================
// FLOATING POINT TYPES
//==============================================================================

typedef float  float32_t;
typedef double float64_t;

//==============================================================================
// COMMON CONSTANTS
//==============================================================================

#ifndef NULL
    #define NULL ((void*)0)
#endif

#ifndef TRUE
    #define TRUE  1
#endif

#ifndef FALSE
    #define FALSE 0
#endif

//==============================================================================
// TIMEOUT VALUES
//==============================================================================

#define CF_WAIT_FOREVER     0xFFFFFFFFUL
#define CF_NO_WAIT          0

#ifdef __cplusplus
}
#endif

#endif /* CF_TYPES_H */
