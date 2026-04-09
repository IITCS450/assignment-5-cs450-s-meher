#include "types.h"
#include "stat.h"
#include "user.h"
#include "uthread.h"

#define MAXTHREADS 16
#define STACK_SIZE 4096

enum tstate {
  T_FREE = 0,
  T_RUNNABLE,
  T_RUNNING,
  T_ZOMBIE
};

struct context {
  uint edi;
  uint esi;
  uint ebx;
  uint ebp;
  uint eip;
};

struct thread {
  tid_t tid;
  int state;
  void *stack;
  struct context *context;
  void (*fn)(void *);
  void *arg;
};

static struct thread threads[MAXTHREADS];
static int current_thread = 0;
static int initialized = 0;

static void
thread_stub(void)
{
  struct thread *t;

  t = &threads[current_thread];
  t->fn(t->arg);

  // Thread finished running.
  t->state = T_ZOMBIE;

  // Give another thread a chance to run.
  thread_yield();

  // Should never get here, but don't keep going if we do.
  exit();
}

static int
pick_next(void)
{
  int i;
  int idx;

  for(i = 1; i < MAXTHREADS; i++){
    idx = (current_thread + i) % MAXTHREADS;
    if(threads[idx].state == T_RUNNABLE)
      return idx;
  }

  return -1;
}

void
thread_init(void)
{
  int i;

  for(i = 0; i < MAXTHREADS; i++){
    threads[i].tid = i;
    threads[i].state = T_FREE;
    threads[i].stack = 0;
    threads[i].context = 0;
    threads[i].fn = 0;
    threads[i].arg = 0;
  }

  // Main thread is thread 0.
  threads[0].state = T_RUNNING;
  current_thread = 0;
  initialized = 1;
}

tid_t
thread_create(void (*fn)(void*), void *arg)
{
  int i;
  char *stack;
  struct context *ctx;

  if(!initialized)
    thread_init();

  for(i = 1; i < MAXTHREADS; i++){
    if(threads[i].state == T_FREE)
      break;
  }

  if(i == MAXTHREADS)
    return -1;

  stack = malloc(STACK_SIZE);
  if(stack == 0)
    return -1;

  // Build an initial saved context at the top of the new stack.
  ctx = (struct context *)(stack + STACK_SIZE - sizeof(struct context));
  memset(ctx, 0, sizeof(struct context));
  ctx->eip = (uint)thread_stub;

  threads[i].stack = stack;
  threads[i].context = ctx;
  threads[i].fn = fn;
  threads[i].arg = arg;
  threads[i].state = T_RUNNABLE;

  return threads[i].tid;
}

void
thread_yield(void)
{
  int prev;
  int next;

  if(!initialized)
    thread_init();

  prev = current_thread;
  next = pick_next();

  if(next < 0)
    return;

  if(threads[prev].state == T_RUNNING)
    threads[prev].state = T_RUNNABLE;

  threads[next].state = T_RUNNING;
  current_thread = next;

  uswtch(&threads[prev].context, threads[next].context);
}

int
thread_join(tid_t tid)
{
  if(tid <= 0 || tid >= MAXTHREADS)
    return -1;

  while(threads[tid].state != T_ZOMBIE){
    if(threads[tid].state == T_FREE)
      return -1;
    thread_yield();
  }

  free(threads[tid].stack);
  threads[tid].stack = 0;
  threads[tid].context = 0;
  threads[tid].fn = 0;
  threads[tid].arg = 0;
  threads[tid].state = T_FREE;

  return 0;
}