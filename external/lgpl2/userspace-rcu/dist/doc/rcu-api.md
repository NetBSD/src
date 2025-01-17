<!--
SPDX-FileCopyrightText: 2023 EfficiOS Inc.

SPDX-License-Identifier: CC-BY-4.0
-->

Userspace RCU API
=================

by Mathieu Desnoyers and Paul E. McKenney


API
---

```c
void rcu_init(void);
```

This must be called before any of the following functions
are invoked.


```c
void rcu_read_lock(void);
```

Begin an RCU read-side critical section. These critical
sections may be nested.


```c
void rcu_read_unlock(void);
```

End an RCU read-side critical section.


```c
void rcu_register_thread(void);
```

Each thread must invoke this function before its first call to
`rcu_read_lock()`. Threads that never call `rcu_read_lock()` need
not invoke this function. In addition, `rcu-bp` ("bullet proof"
RCU) does not require any thread to invoke `rcu_register_thread()`.


```c
void rcu_unregister_thread(void);
```

Each thread that invokes `rcu_register_thread()` must invoke
`rcu_unregister_thread()` before `invoking pthread_exit()`
or before returning from its top-level function.


```c
void synchronize_rcu(void);
```

Wait until every pre-existing RCU read-side critical section
has completed. Note that this primitive will not necessarily
wait for RCU read-side critical sections that have not yet
started: this is not a reader-writer lock. The duration
actually waited is called an RCU grace period.


```c
struct urcu_gp_poll_state start_poll_synchronize_rcu(void);
```

Provides a handle for checking if a new grace period has started
and completed since the handle was obtained. It returns a
`struct urcu_gp_poll_state` handle that can be used with
`poll_state_synchronize_rcu` to check, by polling, if the
associated grace period has completed.

`start_poll_synchronize_rcu` must only be called from
registered RCU read-side threads. For the QSBR flavor, the
caller must be online.


```c
bool poll_state_synchronize_rcu(struct urcu_gp_poll_state state);
```

Checks if the grace period associated with the
`struct urcu_gp_poll_state` handle has completed. If the grace
period has completed, the function returns true. Otherwise,
it returns false.


```c
void call_rcu(struct rcu_head *head,
              void (*func)(struct rcu_head *head));
```

Registers the callback indicated by "head". This means
that `func` will be invoked after the end of a future
RCU grace period. The `rcu_head` structure referenced
by `head` will normally be a field in a larger RCU-protected
structure. A typical implementation of `func` is as
follows:

```c
void func(struct rcu_head *head)
{
    struct foo *p = container_of(head, struct foo, rcu);

    free(p);
}
```

This RCU callback function can be registered as follows
given a pointer `p` to the enclosing structure:

```c
call_rcu(&p->rcu, func);
```

`call_rcu` should be called from registered RCU read-side threads.
For the QSBR flavor, the caller should be online.


```c
void rcu_barrier(void);
```

Wait for all `call_rcu()` work initiated prior to `rcu_barrier()` by
_any_ thread on the system to have completed before `rcu_barrier()`
returns. `rcu_barrier()` should never be called from a `call_rcu()`
thread. This function can be used, for instance, to ensure that
all memory reclaim involving a shared object has completed
before allowing `dlclose()` of this shared object to complete.


```c
struct call_rcu_data *create_call_rcu_data(unsigned long flags,
                                           int cpu_affinity);
```

Returns a handle that can be passed to the following
primitives. The `flags` argument can be zero, or can be
`URCU_CALL_RCU_RT` if the worker threads associated with the
new helper thread are to get real-time response. The argument
`cpu_affinity` specifies a CPU on which the `call_rcu` thread should
be affined to. It is ignored if negative.


```c
void call_rcu_data_free(struct call_rcu_data *crdp);
```

Terminates a `call_rcu()` helper thread and frees its associated
data. The caller must have ensured that this thread is no longer
in use, for example, by passing `NULL` to `set_thread_call_rcu_data()`
and `set_cpu_call_rcu_data()` as required.


```c
struct call_rcu_data *get_default_call_rcu_data(void);
```

Returns the handle for the default `call_rcu()` helper thread.
Creates it if necessary.


```c
struct call_rcu_data *get_cpu_call_rcu_data(int cpu);
```

Returns the handle for the current CPU's `call_rcu()` helper
thread, or `NULL` if the current CPU has no helper thread
currently assigned. The call to this function and use of the
returned `call_rcu_data` should be protected by RCU read-side
lock.


```c
struct call_rcu_data *get_thread_call_rcu_data(void);
```

Returns the handle for the current thread's hard-assigned
`call_rcu()` helper thread, or `NULL` if the current thread is
instead using a per-CPU or the default helper thread.


```c
struct call_rcu_data *get_call_rcu_data(void);
```

Returns the handle for the current thread's `call_rcu()` helper
thread, which is either, in increasing order of preference:
per-thread hard-assigned helper thread, per-CPU helper thread,
or default helper thread. `get_call_rcu_data` should be called
from registered RCU read-side threads. For the QSBR flavor, the
caller should be online.


```c
pthread_t get_call_rcu_thread(struct call_rcu_data *crdp);
```

Returns the helper thread's pthread identifier linked to a call
rcu helper thread data.


```c
void set_thread_call_rcu_data(struct call_rcu_data *crdp);
```

Sets the current thread's hard-assigned `call_rcu()` helper to the
handle specified by `crdp`. Note that `crdp` can be `NULL` to
disassociate this thread from its helper. Once a thread is
disassociated from its helper, further `call_rcu()` invocations
use the current CPU's helper if there is one and the default
helper otherwise.


```c
int set_cpu_call_rcu_data(int cpu, struct call_rcu_data *crdp);
```

Sets the specified CPU's `call_rcu()` helper to the handle
specified by `crdp`. Again, `crdp` can be `NULL` to disassociate
this CPU from its helper thread. Once a CPU has been
disassociated from its helper, further `call_rcu()` invocations
that would otherwise have used this CPU's helper will instead
use the default helper.

The caller must wait for a grace-period to pass between return from
`set_cpu_call_rcu_data()` and call to `call_rcu_data_free()` passing the
previous call rcu data as argument.


```c
int create_all_cpu_call_rcu_data(unsigned long flags);
```

Creates a separate `call_rcu()` helper thread for each CPU.
After this primitive is invoked, the global default `call_rcu()`
helper thread will not be called.

The `set_thread_call_rcu_data()`, `set_cpu_call_rcu_data()`, and
`create_all_cpu_call_rcu_data()` functions may be combined to set up
pretty much any desired association between worker and `call_rcu()`
helper threads. If a given executable calls only `call_rcu()`,
then that executable will have only the single global default
`call_rcu()` helper thread. This will suffice in most cases.


```c
void free_all_cpu_call_rcu_data(void);
```

Clean up all the per-CPU `call_rcu` threads. Should be paired with
`create_all_cpu_call_rcu_data()` to perform teardown. Note that
this function invokes `synchronize_rcu()` internally, so the
caller should be careful not to hold mutexes (or mutexes within a
dependency chain) that are also taken within a RCU read-side
critical section, or in a section where QSBR threads are online.


```c
void call_rcu_before_fork_parent(void);
void call_rcu_after_fork_parent(void);
void call_rcu_after_fork_child(void);
```

Should be used as `pthread_atfork()` handler for programs using
`call_rcu` and performing `fork()` or `clone()` without a following
`exec()`.
