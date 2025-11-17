idf_component_register(
    SRCS
        # CF Core - OS
        "cf_core/src/os/cf_mutex.c"
        "cf_core/src/os/cf_queue.c"
        "cf_core/src/os/cf_task.c"
        "cf_core/src/os/cf_timer.c"
        # CF Core - Utils
        "cf_core/src/utils/cf_log.c"
        # CF Core - Status and Assert
        "cf_core/src/cf_assert.c"
        "cf_core/src/cf_status.c"
        # CF Middleware - Threadpool
        "cf_middleware/threadpool/cf_threadpool.c"
        # CF Middleware - event
        "cf_middleware/event/cf_event.c"

    INCLUDE_DIRS
        "cf_core/include"
        "cf_middleware"
    
    REQUIRES
        freertos
        Application
)