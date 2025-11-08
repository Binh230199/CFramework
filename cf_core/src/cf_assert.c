/**
 * @file cf_assert.c
 * @brief Implementation of assertion system
 */

#include "cf_assert.h"

//==============================================================================
// PRIVATE VARIABLES
//==============================================================================

static cf_assert_handler_t g_assert_handler = NULL;

//==============================================================================
// PUBLIC API IMPLEMENTATION
//==============================================================================

void cf_assert_set_handler(cf_assert_handler_t handler)
{
    g_assert_handler = handler;
}

void cf_assert_failed(const char* file, int line, const char* expr)
{
    // Call custom handler if set
    if (g_assert_handler != NULL) {
        g_assert_handler(file, line, expr);
        // Handler should not return, but if it does, fall through
    }

    // Default behavior: infinite loop
    // In real implementation, this might:
    // - Print to debug console
    // - Trigger breakpoint
    // - Reset system
    // - Log to persistent storage

    CF_UNUSED(file);
    CF_UNUSED(line);
    CF_UNUSED(expr);

    // Disable interrupts and halt
    #if defined(__GNUC__) || defined(__clang__)
       // __asm volatile ("cpsid i" ::: "memory");  // ARM Cortex-M
    #endif

    while(1) {
        // Infinite loop
    }
}
