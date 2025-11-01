# ThreadPool Demo Example

## Overview

This example demonstrates the **ThreadPool** component - the core task dispatcher of CFramework.

## Features Demonstrated

- ThreadPool initialization with default/custom configuration
- Task submission with different priorities (LOW, NORMAL, HIGH, CRITICAL)
- Priority-based task scheduling
- Task execution tracking (active/pending counts)
- Waiting for task completion
- Statistics and monitoring

## What is ThreadPool?

ThreadPool is the **heart of CFramework**. It provides:

- **Worker threads**: Pre-allocated threads ready to execute tasks
- **Task queue**: Priority-based queue for pending tasks
- **Dynamic dispatch**: Tasks are automatically assigned to available workers
- **Priority support**: Critical tasks execute before low-priority ones

## Architecture

```
User Code
    ↓ cf_threadpool_submit()
Task Queue (priority: CRITICAL → HIGH → NORMAL → LOW)
    ↓
Worker Thread Pool (4 workers by default)
    ↓
Task Execution
```

## Task Types in Demo

### 1. Simple Tasks
- Basic message printing
- Demonstrates priority ordering

### 2. Sensor Tasks
- Simulates reading from multiple sensors
- Shows parallel execution

### 3. Processing Tasks
- Simulates data processing pipeline
- Demonstrates queue management

### 4. Critical Tasks
- High-priority interruption
- Shows priority preemption

## Configuration

Edit `cf_user_config.h`:

```c
// Customize ThreadPool
#define CF_THREADPOOL_THREAD_COUNT   8     // More workers
#define CF_THREADPOOL_QUEUE_SIZE     50    // Larger queue
#define CF_THREADPOOL_STACK_SIZE     4096  // Bigger stack
```

## Usage Pattern

```c
// Initialize
cf_threadpool_init();

// Submit task
cf_threadpool_submit(my_function, my_arg, 
                     CF_THREADPOOL_PRIORITY_NORMAL, 
                     CF_WAIT_FOREVER);

// Wait for completion
cf_threadpool_wait_idle(5000);

// Cleanup
cf_threadpool_deinit(true);
```

## Hardware Requirements

- STM32L4/L1/F4/F1 or ESP32
- UART for logging output
- Sufficient RAM for worker threads

## Expected Output

```
[I][AppMain] === CFramework ThreadPool Demo ===
[I][AppMain] ThreadPool initialized successfully
[I][AppMain] --- Demo 1: Priority-based task submission ---
[I][TPWorker0] Task executing: High Priority Task
[I][TPWorker1] Task executing: Normal Priority Task 1
[I][TPWorker2] Task executing: Normal Priority Task 2
[I][TPWorker3] Task executing: Low Priority Task 1
...
[I][AppMain] All tasks completed successfully
[I][AppMain] Active tasks: 0
[I][AppMain] Pending tasks: 0
[I][AppMain] Idle: Yes
```

## Notes

- ThreadPool is the foundation for all async operations in CFramework
- Worker threads run at NORMAL priority by default
- Task queues are separate per priority level
- Use CRITICAL priority sparingly (for urgent operations only)

## Next Steps

- See `03_event_system` for event-driven architecture with ThreadPool
- See `04_sensor_node` for real-world IoT application
