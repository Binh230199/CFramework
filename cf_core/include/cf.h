/**
 * @file cf.h
 * @brief CFramework master include file
 * @version 1.0.0
 * @date 2025-10-29
 * @author CFramework Contributors
 *
 * @copyright Copyright (c) 2025 CFramework
 * Licensed under MIT License
 *
 * This is the main header file for CFramework.
 * Include this file to access all framework functionality.
 */

#ifndef CF_H
#define CF_H

#ifdef __cplusplus
extern "C" {
#endif

//==============================================================================
// CORE INCLUDES
//==============================================================================

#include "cf_common.h"
#include "cf_types.h"
#include "cf_status.h"
#include "cf_config.h"
#include "cf_assert.h"

//==============================================================================
// OS ABSTRACTION
//==============================================================================

#if CF_RTOS_ENABLED
    #include "os/cf_mutex.h"
    #include "os/cf_task.h"
    #include "os/cf_queue.h"
    #include "os/cf_timer.h"
    #include "os/cf_time.h"
    #include "os/cf_critical.h"
#endif

//==============================================================================
// UTILITIES
//==============================================================================

#include "utils/cf_string.h"
#include "utils/cf_ringbuf.h"

#if CF_LOG_ENABLED
    #include "utils/cf_log.h"
#endif

//==============================================================================
// MIDDLEWARE
//==============================================================================

#if CF_THREADPOOL_ENABLED
    #include "threadpool/cf_threadpool.h"
#endif

#if CF_EVENT_ENABLED
    #include "event/cf_event.h"
#endif

//==============================================================================
// FRAMEWORK VERSION
//==============================================================================

/**
 * @brief Get framework version string
 *
 * @return Version string (e.g., "1.0.0")
 */
static inline const char* cf_get_version(void)
{
    return CF_VERSION_STRING;
}

#ifdef __cplusplus
}
#endif

#endif /* CF_H */
