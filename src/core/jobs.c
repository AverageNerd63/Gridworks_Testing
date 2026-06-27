#include "jobs.h"
#include <stdlib.h>
#include <windows.h>

/* Chase-Lev work-stealing deque ----------------------------------------- */

static void deque_push(WorkerDeque *d, Job job) {
    i64 b = atomic_load_explicit(&d->bottom, memory_order_relaxed);
    i64 t = atomic_load_explicit(&d->top,    memory_order_acquire);
    if (b - t >= JOB_DEQUE_CAP) return;  /* full — drop job */
    d->jobs[b & (JOB_DEQUE_CAP - 1)] = job;
    atomic_thread_fence(memory_order_release);
    atomic_store_explicit(&d->bottom, b + 1, memory_order_relaxed);
}

static bool deque_pop(WorkerDeque *d, Job *out) {
    i64 b = atomic_load_explicit(&d->bottom, memory_order_relaxed) - 1;
    atomic_store_explicit(&d->bottom, b, memory_order_relaxed);
    atomic_thread_fence(memory_order_seq_cst);
    i64 t = atomic_load_explicit(&d->top, memory_order_relaxed);
    if (t > b) {
        atomic_store_explicit(&d->bottom, b + 1, memory_order_relaxed);
        return false;
    }
    *out = d->jobs[b & (JOB_DEQUE_CAP - 1)];
    if (t == b) {
        i64 expected = t;
        bool ok = atomic_compare_exchange_strong_explicit(
            &d->top, &expected, t + 1, memory_order_seq_cst, memory_order_relaxed);
        atomic_store_explicit(&d->bottom, b + 1, memory_order_relaxed);
        return ok;
    }
    return true;
}

static bool deque_steal(WorkerDeque *d, Job *out) {
    i64 t = atomic_load_explicit(&d->top,    memory_order_acquire);
    atomic_thread_fence(memory_order_seq_cst);
    i64 b = atomic_load_explicit(&d->bottom, memory_order_acquire);
    if (t >= b) return false;
    *out = d->jobs[t & (JOB_DEQUE_CAP - 1)];
    return atomic_compare_exchange_strong_explicit(
        &d->top, &t, t + 1, memory_order_seq_cst, memory_order_relaxed);
}

/* Worker thread ----------------------------------------------------------- */

typedef struct {
    JobSystem *js;
    u32        index;
} WorkerCtx;

static DWORD WINAPI worker_thread(LPVOID arg) {
    WorkerCtx *ctx = (WorkerCtx *)arg;
    JobSystem *js  = ctx->js;
    u32        idx = ctx->index;

    while (atomic_load_explicit(&js->running, memory_order_acquire)) {
        Job job;
        if (deque_pop(&js->deques[idx], &job)) {
            job.fn(job.data);
            continue;
        }
        /* try to steal from another worker */
        bool stole = false;
        for (u32 i = 1; i < js->worker_count; i++) {
            u32 victim = (idx + i) % js->worker_count;
            if (deque_steal(&js->deques[victim], &job)) {
                job.fn(job.data);
                stole = true;
                break;
            }
        }
        if (!stole) Sleep(0);  /* yield rather than spin */
    }

    free(ctx);
    return 0;
}

/* Public API -------------------------------------------------------------- */

void job_system_init(JobSystem *js, u32 worker_count) {
    js->worker_count = worker_count;
    js->deques       = calloc(worker_count, sizeof(WorkerDeque));
    js->threads      = malloc(worker_count * sizeof(HANDLE));
    atomic_store(&js->running, true);

    for (u32 i = 0; i < worker_count; i++) {
        WorkerCtx *ctx = malloc(sizeof(WorkerCtx));
        ctx->js    = js;
        ctx->index = i;
        js->threads[i] = CreateThread(NULL, 0, worker_thread, ctx, 0, NULL);
    }
}

void job_system_shutdown(JobSystem *js) {
    atomic_store(&js->running, false);
    WaitForMultipleObjects(js->worker_count, (HANDLE *)js->threads, TRUE, INFINITE);
    for (u32 i = 0; i < js->worker_count; i++)
        CloseHandle(js->threads[i]);
    free(js->threads);
    free(js->deques);
}

void job_system_submit(JobSystem *js, u32 worker_hint, JobFn fn, void *data) {
    u32 idx = worker_hint % js->worker_count;
    deque_push(&js->deques[idx], (Job){ .fn = fn, .data = data });
}