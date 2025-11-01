/**
 * @file cf_string.h
 * @brief Safe string utilities
 * @version 1.0.0
 * @date 2025-10-29
 * @author CFramework Contributors
 *
 * @copyright Copyright (c) 2025 CFramework
 * Licensed under MIT License
 */

#ifndef CF_STRING_H
#define CF_STRING_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cf_common.h"
#include <string.h>
#include <stdio.h>

//==============================================================================
// STRING CHECK MACROS
//==============================================================================

/**
 * @brief Check if string is empty or NULL
 */
#define CF_STR_IS_EMPTY(s) (((s) == NULL) || ((s)[0] == '\0'))

/**
 * @brief Check if string is not empty
 */
#define CF_STR_IS_NOT_EMPTY(s) (!CF_STR_IS_EMPTY(s))

//==============================================================================
// SAFE STRING COPY MACROS
//==============================================================================

/**
 * @brief Safely copy string with null-termination guarantee
 *
 * @param[out] dest Destination buffer
 * @param[in] src Source string
 * @param[in] size Size of destination buffer
 *
 * @note Always null-terminates the destination
 * @note Truncates if source is too long
 */
#define CF_STRNCPY(dest, src, size) \
    do { \
        if ((dest) != NULL && (src) != NULL && (size) > 0) { \
            strncpy((dest), (src), (size) - 1); \
            (dest)[(size) - 1] = '\0'; \
        } \
    } while(0)

/**
 * @brief Safely format string with null-termination guarantee
 *
 * @param[out] dest Destination buffer
 * @param[in] size Size of destination buffer
 * @param[in] fmt Format string
 * @param[in] ... Format arguments
 *
 * @note Always null-terminates the destination
 */
#define CF_SNPRINTF(dest, size, fmt, ...) \
    do { \
        if ((dest) != NULL && (size) > 0) { \
            int _len = snprintf((dest), (size), (fmt), ##__VA_ARGS__); \
            if (_len < 0 || _len >= (int)(size)) { \
                (dest)[(size) - 1] = '\0'; \
            } \
        } \
    } while(0)

/**
 * @brief Safely concatenate string with null-termination guarantee
 *
 * @param[in,out] dest Destination buffer
 * @param[in] src Source string to append
 * @param[in] size Size of destination buffer
 *
 * @note Always null-terminates the destination
 */
#define CF_STRNCAT(dest, src, size) \
    do { \
        if ((dest) != NULL && (src) != NULL && (size) > 0) { \
            size_t _dest_len = strlen(dest); \
            if (_dest_len < (size) - 1) { \
                strncat((dest), (src), (size) - _dest_len - 1); \
                (dest)[(size) - 1] = '\0'; \
            } \
        } \
    } while(0)

//==============================================================================
// STRING FUNCTIONS
//==============================================================================

/**
 * @brief Get safe string length (handles NULL)
 *
 * @param[in] str String to measure
 * @param[in] maxlen Maximum length to check
 *
 * @return Length of string (0 if NULL)
 */
static inline size_t cf_strlen_safe(const char* str, size_t maxlen)
{
    if (str == NULL) {
        return 0;
    }

    size_t len = 0;
    while (len < maxlen && str[len] != '\0') {
        len++;
    }

    return len;
}

/**
 * @brief Compare strings safely (handles NULL)
 *
 * @param[in] str1 First string
 * @param[in] str2 Second string
 *
 * @return true if strings are equal, false otherwise
 * @note NULL strings are considered equal to each other
 */
static inline bool cf_streq(const char* str1, const char* str2)
{
    if (str1 == NULL && str2 == NULL) {
        return true;
    }

    if (str1 == NULL || str2 == NULL) {
        return false;
    }

    return strcmp(str1, str2) == 0;
}

#ifdef __cplusplus
}
#endif

#endif /* CF_STRING_H */
