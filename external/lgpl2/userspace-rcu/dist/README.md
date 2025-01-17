<!--
SPDX-FileCopyrightText: 2023 EfficiOS Inc.

SPDX-License-Identifier: CC-BY-4.0
-->

Userspace RCU Implementation
============================

by Mathieu Desnoyers and Paul E. McKenney


Building
--------

    ./bootstrap # skip if using tarball
    ./configure
    make
    make install
    ldconfig

Hints:

  - Forcing 32-bit build:

        CFLAGS="-m32 -g -O2" ./configure

  - Forcing 64-bit build:

        CFLAGS="-m64 -g -O2" ./configure

  - Forcing a 32-bit build with 386 backward compatibility:

        CFLAGS="-m32 -g -O2" ./configure --host=i386-pc-linux-gnu

  - Forcing a 32-bit build for Sparcv9 (typical for Sparc v9)

        CFLAGS="-m32 -Wa,-Av9a -g -O2" ./configure


Architectures supported
-----------------------

Currently, the following architectures are supported:

  - x86 (i386, i486, i586, i686)
  - amd64 / x86\_64
  - PowerPC 32/64
  - S390, S390x
  - ARM 32/64
  - MIPS
  - NIOS2
  - Alpha
  - ia64
  - Sparcv9 32/64
  - Tilera
  - hppa/PA-RISC
  - m68k
  - RISC-V
  - LoongArch

Tested on:

  - Linux all architectures
  - FreeBSD 13 i386/amd64
  - Cygwin i386/amd64
  - MacOS amd64/arm64

Should also work on:

  - Android
  - NetBSD 5
  - OpenBSD
  - Solaris

(more testing needed before claiming support for these OS).


Toolchain support
-----------------

The C compiler used needs to support at least C99. The C++ compiler used needs
to support at least C++11. The oldest GCC version officialy supported and
tested is 4.8.

Older GCC versions might still work with the following exceptions:

  - GCC 3.3 and 3.4 have a bug that prevents them from generating volatile
    accesses to offsets in a TLS structure on 32-bit x86. These versions are
    therefore not compatible with `liburcu` on x86 32-bit
    (i386, i486, i586, i686).
    The problem has been reported to the GCC community:
    <http://www.mail-archive.com/gcc-bugs@gcc.gnu.org/msg281255.html>
  - GCC 3.3 cannot match the "xchg" instruction on 32-bit x86 build.
    See <http://kerneltrap.org/node/7507>
  - Alpha, ia64 and ARM architectures depend on GCC 4.x with atomic builtins
    support. For ARM this was introduced with GCC 4.4:
    <http://gcc.gnu.org/gcc-4.4/changes.html>.
  - Linux aarch64 depends on GCC 5.1 or better because prior versions
    perform unsafe access to deallocated stack.

Clang version 3.0 (based on LLVM 3.0) is supported.

Glibc >= 2.4 should work but the older version we test against is
currently 2.17.


Build system
------------

For developers using the Git tree:

This source tree is based on the autotools suite from GNU to simplify
portability. Here are some things you should have on your system in order to
compile the git repository tree :

  - GNU autotools (automake >=1.12, autoconf >=2.69)
    (make sure your system wide `automake` points to a recent version!)
  - GNU Libtool >=2.2
    (for more information, go to <http://www.gnu.org/software/autoconf/>)

If you get the tree from the repository, you will need to use the `bootstrap`
script in the root of the tree. It calls all the GNU tools needed to prepare
the tree configuration.

Test scripts provided in the `tests/` directory of the source tree depend
on `bash` and the `seq` program.


API
---

See the relevant API documentation files in `doc/`. The APIs provided by
Userspace RCU are, by prefix:

  - `rcu_`: Read-Copy Update (see [`doc/rcu-api.md`](doc/rcu-api.md))
  - `cmm_`: Concurrent Memory Model
  - `caa_`: Concurrent Architecture Abstraction
  - `cds_`: Concurrent Data Structures
    (see [`doc/cds-api.md`](doc/cds-api.md))
  - `uatomic_`: Userspace Atomic
    (see [`doc/uatomic-api.md`](doc/uatomic-api.md))


Quick start guide
-----------------

### Usage of all urcu libraries:

  - Define `_LGPL_SOURCE` (only) if your code is LGPL or GPL compatible
    before including the `urcu.h` or `urcu-qsbr.h` header. If your application
    is distributed under another license, function calls will be generated
    instead of inlines, so your application can link with the library.
  - Linking with one of the libraries below is always necessary even for
    LGPL and GPL applications.
  - Define `URCU_INLINE_SMALL_FUNCTIONS` before including Userspace RCU
    headers if you want Userspace RCU to inline small functions (10
    lines or less) into the application. It can be used by applications
    distributed under any kind of license, and does *not* make the
    application a derived work of Userspace RCU.

Those small inlined functions are guaranteed to match the library
content as long as the library major version is unchanged.
Therefore, the application *must* be compiled with headers matching
the library major version number. Applications using
`URCU_INLINE_SMALL_FUNCTIONS` may be unable to use debugging
features of Userspace RCU without being recompiled.

There are multiple flavors of liburcu available:

  - `memb`,
  - `qsbr`,
  - `mb`,
  - `bp`.

The API members start with the prefix `urcu_<flavor>_`, where
`<flavor>` is the chosen flavor name.


### Usage of `liburcu-memb`

  1. `#include <urcu/urcu-memb.h>`
  2. Link the application with `-lurcu-memb`

This is the preferred version of the library, in terms of
grace-period detection speed, read-side speed and flexibility.
Dynamically detects kernel support for `sys_membarrier()`. Falls back
on `urcu-mb` scheme if support is not present, which has slower
read-side. Use the `--disable-sys-membarrier-fallback` configure option
to disable the fall back, thus requiring `sys_membarrier()` to be
available. This gives a small speedup when `sys_membarrier()` is
supported by the kernel, and aborts in the library constructor if not
supported.


### Usage of `liburcu-qsbr`

  1. `#include <urcu/urcu-qsbr.h>`
  2. Link with `-lurcu-qsbr`

The QSBR flavor of RCU needs to have each reader thread executing
`rcu_quiescent_state()` periodically to progress. `rcu_thread_online()`
and `rcu_thread_offline()` can be used to mark long periods for which
the threads are not active. It provides the fastest read-side at the
expense of more intrusiveness in the application code.


### Usage of `liburcu-mb`

  1. `#include <urcu/urcu-mb.h>`
  2. Link with `-lurcu-mb`

This version of the urcu library uses memory barriers on the writer
and reader sides. This results in faster grace-period detection, but
results in slower reads.


### Usage of `liburcu-bp`

  1. `#include <urcu/urcu-bp.h>`
  2. Link with `-lurcu-bp`

The BP library flavor stands for "bulletproof". It is specifically
designed to help tracing library to hook on applications without
requiring to modify these applications. `urcu_bp_init()`, and
`urcu_bp_unregister_thread()` all become nops, whereas calling
`urcu_bp_register_thread()` becomes optional. The state is dealt with by
the library internally at the expense of read-side and write-side
performance.


### Initialization

Each thread that has reader critical sections (that uses
`urcu_<flavor>_read_lock()`/`urcu_<flavor>_read_unlock()` must first
register to the URCU library. This is done by calling
`urcu_<flavor>_register_thread()`. Unregistration must be performed
before exiting the thread by using `urcu_<flavor>_unregister_thread()`.


### Reading

Reader critical sections must be protected by locating them between
calls to `urcu_<flavor>_read_lock()` and `urcu_<flavor>_read_unlock()`.
Inside that lock, `rcu_dereference()` may be called to read an RCU
protected pointer.


### Writing

`rcu_assign_pointer()` and `rcu_xchg_pointer()` may be called anywhere.
After, `urcu_<flavor>_synchronize_rcu()` must be called. When it
returns, the old values are not in usage anymore.

As an alternative to `urcu_<flavor>_synchronize_rcu()`,
it is also possible to use the urcu polling mechanism to wait for a
grace period to elapse. This can be done by using
`urcu_<flavor>_start_poll_synchronize_rcu()`
to start the grace period polling, and then invoke
`urcu_<flavor>_poll_state_synchronize_rcu()`, which returns true if
the grace period has completed, false otherwise.


### Usage of `liburcu-defer`

  - Follow instructions for either `liburcu-memb`, `liburcu-qsbr`,
    `liburcu-mb`, or `liburcu-bp` above.
    The `liburcu-defer` functionality is pulled into each of
    those library modules.
  - Provides `urcu_<flavor>_defer_rcu()` primitive to enqueue delayed
    callbacks. Queued callbacks are executed in batch periodically after
    a grace period.  Do _not_ use `urcu_<flavor>_defer_rcu()` within a
    read-side critical section, because it may call
    `urcu_<flavor>_synchronize_rcu()` if the thread queue is full.  This
    can lead to deadlock or worse.
  - Requires that `urcu_<flavor>_defer_barrier()` must be called in
    library destructor if a library queues callbacks and is expected to
    be unloaded with `dlclose()`.

Its API is currently experimental. It may change in future library releases.


### Usage of `urcu-call-rcu`

  - Follow instructions for either `liburcu-memb`, `liburcu-qsbr`,
    `liburcu-mb`, or `liburcu-bp` above.
    The `urcu-call-rcu` functionality is pulled into each of
    those library modules.
  - Provides the `urcu_<flavor>_call_rcu()` primitive to enqueue delayed
    callbacks in a manner similar to `urcu_<flavor>_defer_rcu()`, but
    without ever delaying for a grace period.  On the other hand,
    `urcu_<flavor>_call_rcu()`'s best-case overhead is not quite as good
    as that of `urcu_<flavor>_defer_rcu()`.
  - Provides `urcu_<flavor>_call_rcu()` to allow asynchronous handling
    of RCU grace periods.  A number of additional functions are provided
    to manage the helper threads used by `urcu_<flavor>_call_rcu()`, but
    reasonable defaults are used if these additional functions are not
    invoked. See [`doc/rcu-api.md`](doc/rcu-api.md) in userspace-rcu
    documentation for more details.


### Being careful with signals

Read-side critical sections are allowed in a signal handler,
except those setup with `sigaltstack(2)`, with `liburcu-memb` and
`liburcu-mb`. Be careful, however, to disable these signals
between thread creation and calls to `urcu_<flavor>_register_thread()`,
because a signal handler nesting on an unregistered thread would not be
allowed to call `urcu_<flavor>_read_lock()`.

Read-side critical sections are _not_ allowed in a signal handler with
`liburcu-qsbr`, unless signals are disabled explicitly around each
`urcu_qsbr_quiescent_state()` calls, when threads are put offline and around
calls to `urcu_qsbr_synchronize_rcu()`. Even then, we do not recommend it.


### Interaction with mutexes

One must be careful to do not cause deadlocks due to interaction of
`urcu_<flavor>_synchronize_rcu()` and RCU read-side with mutexes. If
`urcu_<flavor>_synchronize_rcu()` is called with a mutex held, this
mutex (or any mutex which has this mutex in its dependency chain) should
not be acquired from within a RCU read-side critical section.

This is especially important to understand in the context of the
QSBR flavor: a registered reader thread being "online" by
default should be considered as within a RCU read-side critical
section unless explicitly put "offline". Therefore, if
`urcu_qsbr_synchronize_rcu()` is called with a mutex held, this mutex,
as well as any mutex which has this mutex in its dependency chain should
only be taken when the RCU reader thread is "offline" (this can be
performed by calling `urcu_qsbr_thread_offline()`).


### Interaction with `fork()`

Special care must be taken for applications performing `fork()` without
any following `exec()`. This is caused by the fact that Linux only clones
the thread calling `fork()`, and thus never replicates any of the other
parent thread into the child process. Most `liburcu` implementations
require that all registrations (as reader, `defer_rcu` and `call_rcu`
threads) should be released before a `fork()` is performed, except for the
rather common scenario where `fork()` is immediately followed by `exec()` in
the child process. The only implementation not subject to that rule is
`liburcu-bp`, which is designed to handle `fork()` by calling
`urcu_bp_before_fork`, `urcu_bp_after_fork_parent` and
`urcu_bp_after_fork_child`.

Applications that use `urcu_<flavor>_call_rcu()` and that `fork()`
without doing an immediate `exec()` must take special action.  The
parent must invoke `urcu_<flavor>_call_rcu_before_fork()` before the
`fork()` and `urcu_<flavor>_call_rcu_after_fork_parent()` after the
`fork()`. The child process must invoke
`urcu_<flavor>_call_rcu_after_fork_child()`.  Even though these three
APIs are suitable for passing to `pthread_atfork()`, use of
`pthread_atfork()` is **STRONGLY DISCOURAGED** for programs calling the
glibc memory allocator (`malloc()`, `calloc()`, `free()`, ...) within
`urcu_<flavor>_call_rcu` callbacks.  This is due to limitations in the
way glibc memory allocator handles calls to the memory allocator from
concurrent threads while the `pthread_atfork()` handlers are executing.

Combining e.g.:

  - call to `free()` from callbacks executed within
    `urcu_<flavor>_call_rcu` worker threads,
  - executing `urcu_<flavor>_call_rcu` atfork handlers within the glibc
    pthread atfork mechanism,

will sometimes trigger interesting process hangs. This usually
hangs on a memory allocator lock within glibc.


### Thread Local Storage (TLS)

Userspace RCU can fall back on `pthread_getspecific()` to emulate
TLS variables on systems where it is not available. This behavior
can be forced by specifying `--disable-compiler-tls` as configure
argument.


### Usage of `DEBUG_RCU` & `--enable-rcu-debug`

By default the library is configured with internal debugging
self-checks disabled.

For always-on debugging self-checks:

    ./configure --enable-rcu-debug

For fine grained enabling of debugging self-checks, build
userspace-rcu with `DEBUG_RCU` defined and compile dependent
applications with `DEBUG_RCU` defined when necessary.

Warning: Enabling this feature result in a performance penalty.


### Usage of `DEBUG_YIELD`

`DEBUG_YIELD` is used to add random delays in the code for testing
purposes.


### SMP support

By default the library is configured to use synchronization primitives
adequate for SMP systems. On uniprocessor systems, support for SMP
systems can be disabled with:

    ./configure --disable-smp-support

theoretically yielding slightly better performance.


### Usage of `--enable-cds-lfht-iter-debug`

By default the library is configured with extra debugging checks for
lock-free hash table iterator traversal disabled.

Building liburcu with `--enable-cds-lfht-iter-debug` and rebuilding
application to match the ABI change allows finding cases where the hash
table iterator is re-purposed to be used on a different hash table while
still being used to iterate on a hash table.

This option alters the rculfhash ABI. Make sure to compile both library
and application with matching configuration.

### Usage of `--enable-compiler-atomic-builtins`

Building liburcu with `--enable-compiler-atomic-builtins` implements the uatomic
API with the compiler atomic builtins if supported.

Make targets
------------

In addition to the usual `make check` target, Userspace RCU features
`make regtest`, `make short_bench` and `make long_bench` targets:

  - `make check`: short tests, meant to be run when rebuilding or
    porting Userspace RCU.
  - `make regtest`: long (many hours) test, meant to be run when
    modifying Userspace RCU or porting it to a new architecture or
    operating system.
  - `make short_bench`: short benchmarks, 3 seconds per test.
  - `make long_bench`: long (many hours) benchmarks, 30 seconds per test.


Known issues
------------

There is an application vs library compatibility issue between
applications built using Userspace RCU 0.10 headers linked against
Userspace RCU 0.11 or 0.12 shared objects. The problem occurs as
follows:

  - An application executable is built with `_LGPL_SOURCE` defined, includes
    any of the Userspace RCU 0.10 urcu flavor headers, and is built
    without the `-fpic` compiler option.

  - The Userspace RCU 0.10 library shared objects are updated to 0.11
    or 0.12 without rebuilding the application.

  - The application will hang, typically when RCU grace period
    (synchronize_rcu) is invoked.

Some possible work-arounds for this are:

  - Rebuild the application against Userspace RCU 0.11+.

  - Rebuild the application with `-fpic`.

  - Upgrade Userspace RCU to 0.13+ without installing 0.11 nor 0.12.


Contacts
--------

You can contact the maintainers on the following mailing list:
`lttng-dev@lists.lttng.org`.

IRC channel: [#lttng](irc://irc.oftc.net/lttng) on the OFTC network

Bug tracker: [Userspace RCU bug tracker](https://bugs.lttng.org/projects/urcu)

Code review: [_userspace-rcu_ project](https://review.lttng.org/q/project:userspace-rcu) on LTTng Review

Continuous integration: [Userspace RCU](https://ci.lttng.org/view/Liburcu/) on LTTng's CI

GitHub mirror: [urcu/userspace-rcu](https://github.com/urcu/userspace-rcu)

Patches are principally submitted and reviewed on [LTTng Review](https://review.lttng.org),
but may also be submitted to the [mailing list](mailto:lttng-dev@lists.lttng.org)
with the subject prefix `PATCH urcu` or by pull request on the
[GitHub mirror](https://github.com/urcu/userspace-rcu).
