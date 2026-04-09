# RESULTS

## Context-switching approach
Each user thread has its own user-space stack. `thread_create()` allocates a stack and places an initial saved context at the top with `eip` pointing to `thread_stub`. `thread_yield()` performs cooperative round-robin scheduling among RUNNABLE threads and uses `uswtch()` to save the current thread’s stack/register context and restore the next thread’s context. When a thread function returns, `thread_stub` marks the thread as ZOMBIE and yields so another thread can run.

## Limitations
- Maximum threads supported: 16
- Stack size per thread: 4096 bytes
- Cooperative scheduling only, so threads must explicitly call `thread_yield()`
- Mutex implementation assumes cooperative user-level scheduling