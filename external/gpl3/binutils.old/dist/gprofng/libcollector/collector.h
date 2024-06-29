/* Copyright (C) 2021 Free Software Foundation, Inc.
   Contributed by Oracle.

   This file is part of GNU Binutils.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

#ifndef _COLLECTOR_H
#define _COLLECTOR_H

#include <ucontext.h>
#include <signal.h>

#include "gp-defs.h"
#include "data_pckts.h"
#include "libcol_util.h"
#include "collector_module.h"

#define GETRELTIME()    (__collector_gethrtime() - __collector_start_time)

extern hrtime_t __collector_start_time;

/* ========================================================== */
/* -------  internal function prototypes ----------------- */
/* These will not be exported from libcollector.so */
struct DataHandle;
struct Heap;
extern struct DataHandle *__collector_create_handle (char*);
extern void __collector_delete_handle (struct DataHandle*);
extern int __collector_write_record (struct DataHandle*, Common_packet*);
extern int __collector_write_packet (struct DataHandle*, CM_Packet*);
extern int __collector_write_string (struct DataHandle*, char*, int);
extern FrameInfo __collector_get_frame_info (hrtime_t, int, void *);
extern FrameInfo __collector_getUID (CM_Array *arg, FrameInfo uid);
extern int __collector_getStackTrace (void *buf, int size, void *bptr, void *eptr, void *arg);
extern void *__collector_ext_return_address (unsigned level);
extern void __collector_mmap_fork_child_cleanup ();

extern int __collector_ext_mmap_install (int);
extern int __collector_ext_mmap_deinstall (int);
extern int __collector_ext_update_map_segments (void);
extern int __collector_check_segment (unsigned long addr,
				      unsigned long *base,
				      unsigned long *end, int maxnretries);
extern int __collector_check_readable_segment (unsigned long addr,
					       unsigned long *base,
					       unsigned long *end, int maxnretries);
extern int __collector_ext_line_init (int * pfollow_this_experiment,
				      const char * progspec,
				      const char *progname);
extern int __collector_ext_line_install (char *, const char *);
extern void __collector_ext_line_close ();
extern void __collector_ext_unwind_init (int);
extern void __collector_ext_unwind_close ();
extern int __collector_ext_jstack_unwind (char*, int, ucontext_t *);
extern void __collector_ext_dispatcher_fork_child_cleanup ();
extern void __collector_ext_unwind_key_init (int isPthread, void * stack);
extern void __collector_ext_dispatcher_tsd_create_key ();
extern void __collector_ext_dispatcher_thread_timer_suspend ();
extern int __collector_ext_dispatcher_thread_timer_resume ();
extern int __collector_ext_dispatcher_install ();
extern void __collector_ext_dispatcher_suspend ();
extern void __collector_ext_dispatcher_restart ();
extern void __collector_ext_dispatcher_deinstall ();
extern void __collector_ext_usage_sample (Smpl_type type, char *name);
extern void __collector_ext_profile_handler (siginfo_t *, ucontext_t *);
extern int __collector_ext_clone_pthread (int (*fn)(void *), void *child_stack, int flags, void *arg,
					  va_list va /* pid_t *ptid, struct user_desc *tlspid_t *" ctid" */);

/* D-light related functions */
extern int __collector_sigprof_install ();
extern int __collector_ext_hwc_active ();
extern void __collector_ext_hwc_check (siginfo_t *, ucontext_t *);
extern int __collector_ext_hwc_lwp_init ();
extern void __collector_ext_hwc_lwp_fini ();
extern int __collector_ext_hwc_lwp_suspend ();
extern int __collector_ext_hwc_lwp_resume ();
extern int (*__collector_VM_ReadByteInstruction)(unsigned char *);
extern int (*__collector_omp_stack_trace)(char*, int, hrtime_t, void*);
extern hrtime_t (*__collector_gethrtime)();
extern int (*__collector_mpi_stack_trace)(char*, int, hrtime_t);
extern int __collector_open_experiment (const char *exp, const char *par, sp_origin_t origin);
extern void __collector_suspend_experiment (char *why);
extern void __collector_resume_experiment ();
extern void __collector_clean_state ();
extern void __collector_close_experiment ();
extern void __collector_terminate_expt ();
extern void __collector_terminate_hook ();
extern void __collector_sample (char *name);
extern void __collector_pause ();
extern void __collector_pause_m ();
extern void __collector_resume ();
extern int collector_sigemt_sigaction (const struct sigaction*,
				       struct sigaction*);
extern int collector_sigchld_sigaction (const struct sigaction*,
					struct sigaction*);

extern int
__collector_log_write (char *format, ...) __attribute__ ((format (printf, 1, 2)));

/* -------  internal global data ----------------- */
/* These will not be exported from libcollector.so */
extern struct Heap *__collector_heap;

/* experiment state flag  */
typedef enum
{
  EXP_INIT, EXP_OPEN, EXP_PAUSED, EXP_CLOSED
} sp_state_t;
extern volatile sp_state_t __collector_expstate;

/* global flag, defines whether target is threaded or not
 *   if set, put _lwp_self() for thread id instead of thr_self()
 *	in output packets; should be set before any data packets
 *	are written, i.e., before signal handlers are installed.
 */
extern int __collector_no_threads;
extern int __collector_libthread_T1; /* T1 or not T1 */
extern int __collector_sample_sig; /* set to signal used to trigger a sample */
extern int __collector_sample_sig_warn; /* if 1, warning given on target use */
extern int __collector_pause_sig; /* set to signal used to toggle pause-resume */
extern int __collector_pause_sig_warn; /* if 1, warning given on target use */
extern hrtime_t __collector_delay_start;
extern int __collector_exp_active;

/* global hrtime_t for next periodic sample */
extern hrtime_t __collector_next_sample;
extern int __collector_sample_period;

/* global hrtime_t for experiment termination (-t) */
extern hrtime_t __collector_terminate_time;
extern int __collector_terminate_duration;
extern char __collector_exp_dir_name[];
extern int __collector_java_mode;
extern int __collector_java_asyncgetcalltrace_loaded;
extern int __collector_jprofile_start_attach ();

/* --------- information controlling debug tracing ------------- */

/* global flag, defines level of trace information */
extern void __collector_dlog (int, int, char *, ...) __attribute__ ((format (printf, 3, 4)));

#define STR(x)  ((x) ? (x) : "NULL")

// To set collector_debug_opt use:
//   SP_COLLECTOR_DEBUG=4 ; export SP_COLLECTOR_DEBUG ; collect ...
enum
{
  SP_DUMP_TIME      = 1,
  SP_DUMP_FLAG      = 2,
  SP_DUMP_JAVA      = 4,
  SP_DUMP_NOHEADER  = 8,
  SP_DUMP_UNWIND    = 16,
  SP_DUMP_STACK     = 32,
};

#ifndef DEBUG
#define DprintfT(flag, ...)
#define tprintf(...)
#define Tprintf(...)
#define TprintfT(...)

#else
#define DprintfT(flag, ...)  __collector_dlog(SP_DUMP_FLAG | (flag), 0, __VA_ARGS__ )
#define tprintf(...)  __collector_dlog( SP_DUMP_NOHEADER, __VA_ARGS__ )
#define Tprintf(...)  __collector_dlog( 0, __VA_ARGS__ )
#define TprintfT(...) __collector_dlog( SP_DUMP_TIME, __VA_ARGS__ )

#endif /* DEBUG */

// To find the glibc version:
//   objdump -T /lib*/*so /lib*/*/*.so | grep popen
// IMPORTANT: The GLIBC_* versions below must match those in mapfile.<variant>
 #if ARCH(Aarch64)
  #define SYS_LIBC_NAME                 "libc.so.6"
  #define SYS_PTHREAD_CREATE_VERSION    "GLIBC_2.17"
  #define SYS_DLOPEN_VERSION            "GLIBC_2.17"
  #define SYS_POPEN_VERSION             "GLIBC_2.17"
  #define SYS_FOPEN_X_VERSION           "GLIBC_2.17"
  #define SYS_FGETPOS_X_VERSION         "GLIBC_2.17"

#elif ARCH(Intel)
  #define SYS_LIBC_NAME                 "libc.so.6"
  #define SYS_POSIX_SPAWN_VERSION       "GLIBC_2.15"
  #if WSIZE(32)
   #define SYS_PTHREAD_CREATE_VERSION   "GLIBC_2.1"
   #define SYS_DLOPEN_VERSION           "GLIBC_2.1"
   #define SYS_POPEN_VERSION            "GLIBC_2.1"
   #define SYS_TIMER_X_VERSION          "GLIBC_2.2"
   #define SYS_FOPEN_X_VERSION          "GLIBC_2.1"
   #define SYS_FGETPOS_X_VERSION        "GLIBC_2.2"
   #define SYS_FGETPOS64_X_VERSION      "GLIBC_2.2"
   #define SYS_OPEN64_X_VERSION         "GLIBC_2.2"
   #define SYS_PREAD_X_VERSION          "GLIBC_2.2"
   #define SYS_PWRITE_X_VERSION         "GLIBC_2.2"
   #define SYS_PWRITE64_X_VERSION       "GLIBC_2.2"
  #else /* WSIZE(64) */
   #define SYS_PTHREAD_CREATE_VERSION   "GLIBC_2.2.5"
   #define SYS_DLOPEN_VERSION           "GLIBC_2.2.5"
   #define SYS_POPEN_VERSION            "GLIBC_2.2.5"
   #define SYS_TIMER_X_VERSION          "GLIBC_2.3.3"
   #define SYS_FOPEN_X_VERSION          "GLIBC_2.2.5"
   #define SYS_FGETPOS_X_VERSION        "GLIBC_2.2.5"
  #endif

 #elif ARCH(SPARC)
  #define SYS_LIBC_NAME                 "libc.so.6"
  #define SYS_DLOPEN_VERSION            "GLIBC_2.1"
  #if WSIZE(32)
   #define SYS_PTHREAD_CREATE_VERSION   "GLIBC_2.1"
   #define SYS_POPEN_VERSION            "GLIBC_2.1"
   #define SYS_FOPEN_X_VERSION          "GLIBC_2.1"
   #define SYS_FGETPOS_X_VERSION        "GLIBC_2.2"
  #else /* WSIZE(64) */
   #define SYS_PTHREAD_CREATE_VERSION   "GLIBC_2.2"
   #define SYS_POPEN_VERSION            "GLIBC_2.2"
   #define SYS_TIMER_X_VERSION          "GLIBC_2.3.3"
   #define SYS_FOPEN_X_VERSION          "GLIBC_2.2"
   #define SYS_FGETPOS_X_VERSION        "GLIBC_2.2"
  #endif
 #endif

#endif
