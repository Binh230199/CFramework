/**
 * @file cf_status.h
 * @brief Unified status code definitions for CFramework
 * @version 1.0.0
 * @date 2025-10-29
 * @author CFramework Contributors
 *
 * @copyright Copyright (c) 2025 CFramework
 * Licensed under MIT License
 */

#ifndef CF_STATUS_H
#define CF_STATUS_H

#ifdef __cplusplus
extern "C" {
#endif

//==============================================================================
// STATUS CODE DEFINITIONS
//==============================================================================

/**
 * @brief Unified status codes for all framework APIs
 *
 * All framework functions that can fail MUST return cf_status_t.
 * Success is always CF_OK (0), errors are non-zero.
 */
typedef enum {
    CF_OK = 0,                      /**< Operation successful */

    // Generic errors (1-9)
    CF_ERROR,                       /**< Generic error */
    CF_ERROR_FAILED,                /**< Operation failed */

    // Parameter errors (10-19)
    CF_ERROR_INVALID_PARAM,         /**< Invalid parameter value */
    CF_ERROR_NULL_POINTER,          /**< NULL pointer passed */
    CF_ERROR_INVALID_RANGE,         /**< Value out of valid range */
    CF_ERROR_INVALID_STATE,         /**< Invalid state for operation */

    // Resource errors (20-29)
    CF_ERROR_NO_MEMORY,             /**< Out of memory */
    CF_ERROR_NO_RESOURCE,           /**< No free resource available */
    CF_ERROR_BUSY,                  /**< Resource is busy */
    CF_ERROR_IN_USE,                /**< Resource is in use */

    // Operation errors (30-39)
    CF_ERROR_TIMEOUT,               /**< Operation timed out */
    CF_ERROR_NOT_SUPPORTED,         /**< Operation not supported */
    CF_ERROR_NOT_IMPLEMENTED,       /**< Feature not implemented */
    CF_ERROR_NOT_INITIALIZED,       /**< Module not initialized */
    CF_ERROR_ALREADY_INITIALIZED,   /**< Module already initialized */
    CF_ERROR_NOT_FOUND,             /**< Item/resource not found */

    // Hardware errors (40-49)
    CF_ERROR_HARDWARE,              /**< Hardware error */
    CF_ERROR_HAL,                   /**< HAL layer error */
    CF_ERROR_DEVICE_NOT_FOUND,      /**< Device not found */
    CF_ERROR_DEVICE_BUSY,           /**< Device is busy */

    // Communication errors (50-59)
    CF_ERROR_COMM,                  /**< Communication error */
    CF_ERROR_COMM_TIMEOUT,          /**< Communication timeout */
    CF_ERROR_COMM_CRC,              /**< CRC error */
    CF_ERROR_COMM_NACK,             /**< Not acknowledged */

    // OS errors (60-69)
    CF_ERROR_OS,                    /**< OS error */
    CF_ERROR_MUTEX,                 /**< Mutex error */
    CF_ERROR_SEMAPHORE,             /**< Semaphore error */
    CF_ERROR_QUEUE_FULL,            /**< Queue is full */
    CF_ERROR_QUEUE_EMPTY,           /**< Queue is empty */

    // Total count
    CF_STATUS_COUNT                 /**< Total number of status codes */
} cf_status_t;

//==============================================================================
// STATUS CODE CHECKING MACROS
//==============================================================================

/**
 * @brief Check if status indicates success
 */
#define CF_IS_OK(status)        ((status) == CF_OK)

/**
 * @brief Check if status indicates error
 */
#define CF_IS_ERROR(status)     ((status) != CF_OK)

//==============================================================================
// STATUS CODE STRING CONVERSION
//==============================================================================

/**
 * @brief Convert status code to string
 *
 * @param[in] status Status code to convert
 * @return Constant string representation (never NULL)
 *
 * @note This function is useful for debugging and logging
 */
const char* cf_status_to_string(cf_status_t status);

#ifdef __cplusplus
}
#endif

#endif /* CF_STATUS_H */
