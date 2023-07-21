/* Threads functions
 *
 * (c) Copyright 2023, redeflesq
 * This file is provided as is (no warranties).
 */
#pragma library Threads

#define INFINITE 0xFFFFFFFF  // Infinite timeout

/* Create thread by function name */
native create_thread(const function_name[]);

/* Destroy thread by index. Crash app */
native destroy_thread(thread);

/* Resume thread by index */
native resume_thread(thread);

/* Suspend thread by index */
native suspend_thread(thread);

/* Wait for thread end by index. Ms */
native wait_thread(thread, time=INFINITE);