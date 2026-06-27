#pragma once
#include "types.h"
#include <stdatomic.h>
#include <stdbool.h>

#define JOB_DEQUE_CAP 1024  /* must be power of 2 */

typedef void (*JobFn)(void *data);

typedef struct {
    JobFn  fn;
    void  *data;
} Job;

typedef struct {
    _Atomic(i64) top;
    _Atomic(i64) bottom;
    Job          jobs[JOB_DEQUE_CAP];
} WorkerDeque;

typedef struct JobSystem {
    u32           worker_count;
    void        **threads;      /* HANDLE[], opaque to callers */
    WorkerDeque  *deques;
    _Atomic(bool) running;
} JobSystem;

void job_system_init(JobSystem *js, u32 worker_count);
void job_system_shutdown(JobSystem *js);
void job_system_submit(JobSystem *js, u32 worker_hint, JobFn fn, void *data);