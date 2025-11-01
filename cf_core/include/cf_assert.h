/**
 * @file cf_assert.h
 * @brief Assertion and verification system for CFramework
 * @version 1.0.0
 * @date 2025-10-29
 * @author CFramework Contributors
 *
 * @copyright Copyright (c) 2025 CFramework
 * Licensed under MIT License
 */

#ifndef CF_ASSERT_H
#define CF_ASSERT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cf_common.h"

//==============================================================================
// ASSERTION HANDLER
//==============================================================================

/**
 * @brief Assert failure handler callback type
 *
 * @param[in] file Source file name
 * @param[in] line Line number
 * @param[in] expr Expression string
 *
 * @note This function should NOT return. Implementation should:
 *       - Log the error
 *       - Optionally reset the system
 *       - Enter infinite loop if no reset
 */
typedef void (*cf_assert_handler_t)(const char* file, int line, const char* expr);

/**
 * @brief Set custom assert handler
 *
 * @param[in] handler Custom handler function (NULL for default)
 *
 * @note Default handler will enter infinite loop
 */
void cf_assert_set_handler(cf_assert_handler_t handler);

/**
 * @brief Default assert failure handler
 *
 * @param[in] file Source file name
 * @param[in] line Line number
 * @param[in] expr Expression string
 *
 * @note This function does NOT return
 */
void cf_assert_failed(const char* file, int line, const char* expr);

//==============================================================================
// ASSERTION MACROS
//==============================================================================

#if CF_ASSERT_ENABLED

/**
 * @brief Debug assertion (only in debug builds)
 *
 * Checks condition and calls assertion handler if false.
 * Disabled in release builds (CF_DEBUG=0).
 *
 * @param[in] expr Expression to check
 *
 * Usage:
 *   CF_ASSERT(ptr != NULL);
 *   CF_ASSERT(count > 0);
 */
#define CF_ASSERT(expr) \
    do { \
        if (!(expr)) { \
            cf_assert_failed(__FILE__, __LINE__, #expr); \
        } \
    } while(0)

/**
 * @brief Assert with custom message
 */
#define CF_ASSERT_MSG(expr, msg) \
    do { \
        if (!(expr)) { \
            cf_assert_failed(__FILE__, __LINE__, msg); \
        } \
    } while(0)

#else

/**
 * @brief Assert disabled in release builds
 */
#define CF_ASSERT(expr) ((void)0)
#define CF_ASSERT_MSG(expr, msg) ((void)0)

#endif /* CF_ASSERT_ENABLED */

//==============================================================================
// RUNTIME VERIFICATION (Always Enabled)
//==============================================================================

/**
 * @brief Runtime verification (never disabled)
 *
 * Similar to CF_ASSERT but ALWAYS enabled, even in release builds.
 * Use for critical checks that must always run.
 *
 * @param[in] expr Expression to verify
 *
 * Usage:
 *   CF_VERIFY(critical_operation() == true);
 */
#define CF_VERIFY(expr) \
    do { \
        if (!(expr)) { \
            cf_assert_failed(__FILE__, __LINE__, #expr); \
        } \
    } while(0)

/**
 * @brief Verify with custom message
 */
#define CF_VERIFY_MSG(expr, msg) \
    do { \
        if (!(expr)) { \
            cf_assert_failed(__FILE__, __LINE__, msg); \
        } \
    } while(0)

//==============================================================================
// STATIC ASSERTIONS (Compile-time)
//==============================================================================

/**
 * @brief Compile-time assertion
 *
 * Generates compile error if condition is false.
 *
 * Usage:
 *   CF_STATIC_ASSERT(sizeof(int) == 4, "int must be 4 bytes");
 */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
    #define CF_STATIC_ASSERT(expr, msg) _Static_assert(expr, msg)
#else
    #define CF_STATIC_ASSERT(expr, msg) \
        typedef char cf_static_assert_##__LINE__[(expr) ? 1 : -1]
#endif

#ifdef __cplusplus
}
#endif

#endif /* CF_ASSERT_H */
