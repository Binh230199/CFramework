/**
 * @file cf_status.c
 * @brief Implementation of status code utilities
 */

#include "cf_status.h"

//==============================================================================
// STATUS TO STRING CONVERSION
//==============================================================================

const char* cf_status_to_string(cf_status_t status)
{
    switch (status) {
        case CF_OK:                         return "CF_OK";
        case CF_ERROR:                      return "CF_ERROR";
        case CF_ERROR_FAILED:               return "CF_ERROR_FAILED";

        case CF_ERROR_INVALID_PARAM:        return "CF_ERROR_INVALID_PARAM";
        case CF_ERROR_NULL_POINTER:         return "CF_ERROR_NULL_POINTER";
        case CF_ERROR_INVALID_RANGE:        return "CF_ERROR_INVALID_RANGE";
        case CF_ERROR_INVALID_STATE:        return "CF_ERROR_INVALID_STATE";

        case CF_ERROR_NO_MEMORY:            return "CF_ERROR_NO_MEMORY";
        case CF_ERROR_NO_RESOURCE:          return "CF_ERROR_NO_RESOURCE";
        case CF_ERROR_BUSY:                 return "CF_ERROR_BUSY";
        case CF_ERROR_IN_USE:               return "CF_ERROR_IN_USE";

        case CF_ERROR_TIMEOUT:              return "CF_ERROR_TIMEOUT";
        case CF_ERROR_NOT_SUPPORTED:        return "CF_ERROR_NOT_SUPPORTED";
        case CF_ERROR_NOT_IMPLEMENTED:      return "CF_ERROR_NOT_IMPLEMENTED";
        case CF_ERROR_NOT_INITIALIZED:      return "CF_ERROR_NOT_INITIALIZED";
        case CF_ERROR_ALREADY_INITIALIZED:  return "CF_ERROR_ALREADY_INITIALIZED";
        case CF_ERROR_NOT_FOUND:            return "CF_ERROR_NOT_FOUND";

        case CF_ERROR_HARDWARE:             return "CF_ERROR_HARDWARE";
        case CF_ERROR_HAL:                  return "CF_ERROR_HAL";
        case CF_ERROR_DEVICE_NOT_FOUND:     return "CF_ERROR_DEVICE_NOT_FOUND";
        case CF_ERROR_DEVICE_BUSY:          return "CF_ERROR_DEVICE_BUSY";

        case CF_ERROR_COMM:                 return "CF_ERROR_COMM";
        case CF_ERROR_COMM_TIMEOUT:         return "CF_ERROR_COMM_TIMEOUT";
        case CF_ERROR_COMM_CRC:             return "CF_ERROR_COMM_CRC";
        case CF_ERROR_COMM_NACK:            return "CF_ERROR_COMM_NACK";

        case CF_ERROR_OS:                   return "CF_ERROR_OS";
        case CF_ERROR_MUTEX:                return "CF_ERROR_MUTEX";
        case CF_ERROR_SEMAPHORE:            return "CF_ERROR_SEMAPHORE";
        case CF_ERROR_QUEUE_FULL:           return "CF_ERROR_QUEUE_FULL";
        case CF_ERROR_QUEUE_EMPTY:          return "CF_ERROR_QUEUE_EMPTY";

        default:                            return "UNKNOWN_STATUS";
    }
}
