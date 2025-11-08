/**
 * @file cf_common.h
 * @brief Common includes and macros for CFramework
 * @version 1.0.0
 * @date 2025-10-29
 * @author CFramework Contributors
 *
 * @copyright Copyright (c) 2025 CFramework
 * Licensed under MIT License
 *
 * This is the main header file that should be included by all framework modules.
 */

#ifndef CF_COMMON_H
#define CF_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

//==============================================================================
// CORE INCLUDES
//==============================================================================

#include "cf_types.h"
#include "cf_status.h"
#include "cf_config.h"

// FreeRTOS memory allocation (if RTOS enabled)
#if CF_RTOS_ENABLED && !defined(CF_COMMON_NO_FREERTOS)
    #ifdef ESP_PLATFORM
        #include "freertos/FreeRTOS.h"
    #else
        #include "FreeRTOS.h"
    #endif
    // pvPortMalloc and vPortFree available
#endif

//==============================================================================
// COMPILER ATTRIBUTES
//==============================================================================

/**
 * @brief Mark function as weak (can be overridden)
 */
#ifndef CF_WEAK
    #if defined(__GNUC__) || defined(__clang__)
        #define CF_WEAK __attribute__((weak))
    #elif defined(__IAR_SYSTEMS_ICC__)
        #define CF_WEAK __weak
    #elif defined(__CC_ARM)
        #define CF_WEAK __weak
    #else
        #define CF_WEAK
    #endif
#endif

/**
 * @brief Mark variable/function for specific memory section
 */
#ifndef CF_SECTION
    #if defined(__GNUC__) || defined(__clang__)
        #define CF_SECTION(x) __attribute__((section(x)))
    #else
        #define CF_SECTION(x)
    #endif
#endif

/**
 * @brief Mark function as always inline
 */
#ifndef CF_INLINE
    #if defined(__GNUC__) || defined(__clang__)
        #define CF_INLINE static inline __attribute__((always_inline))
    #elif defined(__IAR_SYSTEMS_ICC__)
        #define CF_INLINE static inline
    #elif defined(__CC_ARM)
        #define CF_INLINE static __inline
    #else
        #define CF_INLINE static inline
    #endif
#endif

/**
 * @brief Suppress unused variable warning
 */
#define CF_UNUSED(x) ((void)(x))

//==============================================================================
// UTILITY MACROS
//==============================================================================

/**
 * @brief Get array element count
 */
#define CF_ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

/**
 * @brief Get minimum of two values
 */
#define CF_MIN(a, b) (((a) < (b)) ? (a) : (b))

/**
 * @brief Get maximum of two values
 */
#define CF_MAX(a, b) (((a) > (b)) ? (a) : (b))

/**
 * @brief Clamp value between min and max
 */
#define CF_CLAMP(val, min, max) (CF_MAX(min, CF_MIN(val, max)))

/**
 * @brief Align value up to alignment boundary
 */
#define CF_ALIGN_UP(val, align) (((val) + (align) - 1) & ~((align) - 1))

/**
 * @brief Align value down to alignment boundary
 */
#define CF_ALIGN_DOWN(val, align) ((val) & ~((align) - 1))

//==============================================================================
// POINTER SAFETY MACROS
//==============================================================================

/**
 * @brief Check if pointer is NULL
 */
#define CF_PTR_IS_NULL(ptr) ((ptr) == NULL)

/**
 * @brief Check if pointer is not NULL
 */
#define CF_PTR_IS_NOT_NULL(ptr) ((ptr) != NULL)

/**
 * @brief Check pointer and return error if NULL
 */
#define CF_PTR_CHECK(ptr) \
    do { \
        if ((ptr) == NULL) { \
            return CF_ERROR_NULL_POINTER; \
        } \
    } while(0)

/**
 * @brief Check pointer and return value if NULL
 */
#define CF_PTR_CHECK_RET(ptr, ret) \
    do { \
        if ((ptr) == NULL) { \
            return (ret); \
        } \
    } while(0)

//==============================================================================
// BIT MANIPULATION MACROS
//==============================================================================

/**
 * @brief Set bit at position
 */
#define CF_BIT_SET(val, bit) ((val) |= (1U << (bit)))

/**
 * @brief Clear bit at position
 */
#define CF_BIT_CLEAR(val, bit) ((val) &= ~(1U << (bit)))

/**
 * @brief Toggle bit at position
 */
#define CF_BIT_TOGGLE(val, bit) ((val) ^= (1U << (bit)))

/**
 * @brief Check if bit is set
 */
#define CF_BIT_IS_SET(val, bit) (((val) & (1U << (bit))) != 0)

/**
 * @brief Check if bit is clear
 */
#define CF_BIT_IS_CLEAR(val, bit) (((val) & (1U << (bit))) == 0)

/**
 * @brief Create bit mask
 */
#define CF_BIT_MASK(bit) (1U << (bit))

//==============================================================================
// STRING MACROS
//==============================================================================

/**
 * @brief Convert preprocessor define to string
 */
#define CF_STRINGIFY(x) #x
#define CF_TOSTRING(x) CF_STRINGIFY(x)

//==============================================================================
// FRAMEWORK VERSION
//==============================================================================

#define CF_VERSION_MAJOR    1
#define CF_VERSION_MINOR    0
#define CF_VERSION_PATCH    0

#define CF_VERSION_STRING   CF_TOSTRING(CF_VERSION_MAJOR) "." \
                            CF_TOSTRING(CF_VERSION_MINOR) "." \
                            CF_TOSTRING(CF_VERSION_PATCH)

#ifdef __cplusplus
}
#endif

#endif /* CF_COMMON_H */
