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

/*
 *	Synchronization events
 */
#include "config.h"
#include <alloca.h>
#include <dlfcn.h>
#include <unistd.h>
#include <semaphore.h>		/* sem_wait() */
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <pthread.h>

#include "gp-defs.h"
#include "collector_module.h"
#include "gp-experiment.h"
#include "data_pckts.h"
#include "i18n.h"
#include "tsd.h"
#include "cc_libcollector.h"

/* TprintfT(<level>,...) definitions.  Adjust per module as needed */
#define DBG_LT0 0 // for high-level configuration, unexpected errors/warnings
#define DBG_LTT 0 // for interposition on GLIBC functions
#define DBG_LT1 1 // for configuration details, warnings
#define DBG_LT2 2
#define DBG_LT3 3

/* define the packet that will be written out */
typedef struct Sync_packet
{ /* Synchronization delay tracing packet */
  Common_packet comm;
  hrtime_t requested;       /* time of synchronization request */
  Vaddr_type objp;          /* vaddr of synchronization object */
} Sync_packet;

static int open_experiment (const char *);
static int start_data_collection (void);
static int stop_data_collection (void);
static int close_experiment (void);
static int detach_experiment (void);
static int init_thread_intf ();
static int sync_calibrate ();

static ModuleInterface module_interface ={
  SP_SYNCTRACE_FILE,        /* description */
  NULL,                     /* initInterface */
  open_experiment,          /* openExperiment */
  start_data_collection,    /* startDataCollection */
  stop_data_collection,     /* stopDataCollection */
  close_experiment,         /* closeExperiment */
  detach_experiment         /* detachExperiment (fork child) */
};

static CollectorInterface *collector_interface = NULL;
static int sync_mode = 0;
static long sync_scope = 0;
static int sync_native = 0;
static int sync_java = 0;
static CollectorModule sync_hndl = COLLECTOR_MODULE_ERR;
static unsigned sync_key = COLLECTOR_TSD_INVALID_KEY;
static long sync_threshold = -1; /* calibrate the value */
static int init_thread_intf_started = 0;
static int init_thread_intf_finished = 0;

#define CHCK_NREENTRANCE(x)     (!sync_native || !sync_mode || ((x) = collector_interface->getKey( sync_key )) == NULL || (*(x) != 0))
#define RECHCK_NREENTRANCE(x)   (!sync_native || !sync_mode || ((x) = collector_interface->getKey( sync_key )) == NULL || (*(x) == 0))
#define CHCK_JREENTRANCE(x)     (!sync_java || !sync_mode || ((x) = collector_interface->getKey( sync_key )) == NULL || (*(x) != 0))
#define RECHCK_JREENTRANCE(x)   (!sync_java || !sync_mode || ((x) = collector_interface->getKey( sync_key )) == NULL || (*(x) == 0))
#define PUSH_REENTRANCE(x)      ((*(x))++)
#define POP_REENTRANCE(x)       ((*(x))--)

#define CALL_REAL(x)            (*(int(*)())__real_##x)
#define NULL_PTR(x)             ( __real_##x == NULL )
#define gethrtime	collector_interface->getHiResTime

#ifdef DEBUG
#define Tprintf(...)   if (collector_interface) collector_interface->writeDebugInfo( 0, __VA_ARGS__ )
#define TprintfT(...)  if (collector_interface) collector_interface->writeDebugInfo( 1, __VA_ARGS__ )
#else
#define Tprintf(...)
#define TprintfT(...)
#endif

/*
 * In most cases, the functions which require interposition are implemented as
 * weak symbols corresponding to an associated internal function named with a
 * leading underscore: e.g., mutex_lock() is simply an alias for _mutex_lock().
 * For the wait functions, however, the published version (used by applications)
 * is distinct from the internal version (used by system libraries), i.e.,
 * cond_wait() is an alias for _cond_wait_cancel() rather than _cond_wait().
 */
static void *__real_strtol = NULL;
static void *__real_fprintf = NULL;
static void *__real___collector_jprofile_enable_synctrace = NULL;
static void *__real_pthread_mutex_lock = NULL;
static void *__real_pthread_mutex_unlock = NULL; /* not interposed, used in calibrate */
static void *__real_pthread_cond_wait = NULL;
static void *__real_pthread_cond_timedwait = NULL;
static void *__real_pthread_join = NULL;
static void *__real_sem_wait = NULL;
static void *__real_pthread_cond_wait_2_3_2 = NULL;
static void *__real_pthread_cond_timedwait_2_3_2 = NULL;

#if WSIZE(32)
static void *__real_sem_wait_2_1 = NULL;
static void *__real_sem_wait_2_0 = NULL;
static void *__real_pthread_cond_wait_2_0 = NULL;
static void *__real_pthread_cond_timedwait_2_0 = NULL;
#elif WSIZE(64)
#if ARCH(Intel)
static void *__real_pthread_cond_wait_2_2_5 = NULL;
static void *__real_pthread_cond_timedwait_2_2_5 = NULL;
#elif ARCH(SPARC)
static void *__real_pthread_cond_wait_2_2 = NULL;
static void *__real_pthread_cond_timedwait_2_2 = NULL;
#endif  /* ARCH() */
#endif /* WSIZE() */

static void
collector_memset (void *s, int c, size_t n)
{
  unsigned char *s1 = s;
  while (n--)
    *s1++ = (unsigned char) c;
}

void
__collector_module_init (CollectorInterface *_collector_interface)
{
  if (_collector_interface == NULL)
    return;
  collector_interface = _collector_interface;
  TprintfT (0, "synctrace: __collector_module_init\n");
  sync_hndl = collector_interface->registerModule (&module_interface);

  /* Initialize next module */
  ModuleInitFunc next_init = (ModuleInitFunc) dlsym (RTLD_NEXT, "__collector_module_init");
  if (next_init != NULL)
    next_init (_collector_interface);
}

static int
open_experiment (const char *exp)
{
  long thresh = 0;
  if (init_thread_intf_finished == 0)
    init_thread_intf ();
  if (collector_interface == NULL)
    {
      Tprintf (0, "synctrace: collector_interface is null.\n");
      return COL_ERROR_SYNCINIT;
    }
  if (sync_hndl == COLLECTOR_MODULE_ERR)
    {
      Tprintf (0, "synctrace: handle create failed.\n");
      collector_interface->writeLog ("<event kind=\"%s\" id=\"%d\">data handle not created</event>\n",
				     SP_JCMD_CERROR, COL_ERROR_SYNCINIT);
      return COL_ERROR_SYNCINIT;
    }
  TprintfT (0, "synctrace: open_experiment %s\n", exp);

  char *params = (char *) collector_interface->getParams ();
  while (params)
    {
      if ((params[0] == 's') && (params[1] == ':'))
	{
	  char *ptr = params + 2;
	  Tprintf (DBG_LT1, "synctrace: open_experiment s: parameter = %s\n", ptr);
	  while (*ptr != ',' && *ptr != ';')
	    ptr++;
	  sync_scope = 0;
	  if (*ptr == ',')
	    {
	      sync_scope = CALL_REAL (strtol) (ptr + 1, NULL, 0);
	      switch (sync_scope)
		{
		case 1:
		  sync_java = 0;
		  sync_native = 1;
		  break;
		case 2:
		  sync_java = 1;
		  sync_native = 0;
		  break;
		default:
		case 3:
		  sync_native = 1;
		  sync_java = 1;
		  break;
		}
	      Tprintf (0, "\tsynctrace: sync_scope found as %ld\n", sync_scope);
	    }
	  else
	    {
	      /* the old-style descriptor, without scope */
	      /* if there was no comma, use the old default */
	      sync_scope = 3;
	      sync_java = 1;
	      sync_native = 1;
	      Tprintf (0, "\tsynctrace: sync_scope not found set to %ld\n", sync_scope);
	    }
	  if (__real___collector_jprofile_enable_synctrace == NULL)
	    sync_java = 0;
	  thresh = CALL_REAL (strtol)(params + 2, NULL, 0);
	  break; /* from the loop to find the "s:thresh,scope" entry */
	}
      else
	params++;
    }
  if (params == NULL)  /* Sync data collection not specified */
    return COL_ERROR_SYNCINIT;
  if (thresh < 0)  /* calibrate the threshold, keep it as a negative number */
    thresh = -sync_calibrate ();

  sync_key = collector_interface->createKey (sizeof ( int), NULL, NULL);
  if (sync_key == (unsigned) - 1)
    {
      Tprintf (0, "synctrace: TSD key create failed.\n");
      collector_interface->writeLog ("<event kind=\"%s\" id=\"%d\">TSD key not created</event>\n",
				     SP_JCMD_CERROR, COL_ERROR_SYNCINIT);
      return COL_ERROR_SYNCINIT;
    }
  /* if Java synctrace was requested, tell the jprofile module */
  if (sync_java)
    {
      TprintfT (0, "synctrace: enabling Java synctrace\n");
      CALL_REAL (__collector_jprofile_enable_synctrace)();
    }
  collector_interface->writeLog ("<profile name=\"%s\" threshold=\"%ld\" scope=\"%ld\">\n",
				 SP_JCMD_SYNCTRACE, thresh, sync_scope);
  collector_interface->writeLog ("  <profdata fname=\"%s\"/>\n",
				 module_interface.description);
  /* Record Sync_packet description */
  Sync_packet *pp = NULL;
  collector_interface->writeLog ("  <profpckt kind=\"%d\" uname=\"Synchronization tracing data\">\n", SYNC_PCKT);
  collector_interface->writeLog ("    <field name=\"LWPID\" uname=\"Lightweight process id\" offset=\"%d\" type=\"%s\"/>\n",
				 &pp->comm.lwp_id, sizeof (pp->comm.lwp_id) == 4 ? "INT32" : "INT64");
  collector_interface->writeLog ("    <field name=\"THRID\" uname=\"Thread number\" offset=\"%d\" type=\"%s\"/>\n",
				 &pp->comm.thr_id, sizeof (pp->comm.thr_id) == 4 ? "INT32" : "INT64");
  collector_interface->writeLog ("    <field name=\"CPUID\" uname=\"CPU id\" offset=\"%d\" type=\"%s\"/>\n",
				 &pp->comm.cpu_id, sizeof (pp->comm.cpu_id) == 4 ? "INT32" : "INT64");
  collector_interface->writeLog ("    <field name=\"TSTAMP\" uname=\"High resolution timestamp\" offset=\"%d\" type=\"%s\"/>\n",
				 &pp->comm.tstamp, sizeof (pp->comm.tstamp) == 4 ? "INT32" : "INT64");
  collector_interface->writeLog ("    <field name=\"FRINFO\" offset=\"%d\" type=\"%s\"/>\n",
				 &pp->comm.frinfo, sizeof (pp->comm.frinfo) == 4 ? "INT32" : "INT64");
  collector_interface->writeLog ("    <field name=\"SRQST\" uname=\"Synchronization start time\" offset=\"%d\" type=\"%s\"/>\n",
				 &pp->requested, sizeof (pp->requested) == 4 ? "INT32" : "INT64");
  collector_interface->writeLog ("    <field name=\"SOBJ\" uname=\"Synchronization object address\" offset=\"%d\" type=\"%s\"/>\n",
				 &pp->objp, sizeof (pp->objp) == 4 ? "INT32" : "INT64");
  collector_interface->writeLog ("  </profpckt>\n");
  collector_interface->writeLog ("</profile>\n");

  /* Convert threshold from microsec to nanosec */
  sync_threshold = (thresh > 0 ? thresh : -thresh) * 1000;
  TprintfT (0, "synctrace: open_experiment complete %ld\n", sync_threshold);
  return COL_ERROR_NONE;
}

static int
start_data_collection (void)
{
  sync_mode = 1;
  TprintfT (0, "synctrace: start_data_collection\n");
  return 0;
}

static int
stop_data_collection (void)
{
  sync_mode = 0;
  TprintfT (0, "synctrace: stop_data_collection\n");
  return 0;
}

static int
close_experiment (void)
{
  sync_mode = 0;
  sync_threshold = -1;
  sync_key = COLLECTOR_TSD_INVALID_KEY;
  TprintfT (0, "synctrace: close_experiment\n");
  return 0;
}

/* fork child.  Clean up state but don't write to experiment */
static int
detach_experiment (void)
{
  sync_mode = 0;
  sync_threshold = -1;
  sync_key = COLLECTOR_TSD_INVALID_KEY;
  TprintfT (0, "synctrace: detach_experiment\n");
  return 0;
}

#define NUM_ITER    100     /* number of iterations in calibration */
#define NUM_WARMUP    3     /* number of warm up iterations */

static int
sync_calibrate ()
{
  pthread_mutex_t mt = PTHREAD_MUTEX_INITIALIZER;
  hrtime_t bt, at, delta;
  hrtime_t avg, max, min;
  int i;
  int ret;
  avg = (hrtime_t) 0;
  min = max = (hrtime_t) 0;
  for (i = 0; i < NUM_ITER + NUM_WARMUP; i++)
    {
      /* Here we simulate a real call */
      bt = gethrtime ();
      ret = CALL_REAL (pthread_mutex_lock)(&mt);
      at = gethrtime ();
      CALL_REAL (pthread_mutex_unlock)(&mt);
      if (i < NUM_WARMUP)   /* skip these iterations */
	continue;
      /* add the time of this one */
      delta = at - bt;
      avg += delta;
      if (min == 0)
	min = delta;
      if (delta < min)
	min = delta;
      if (delta > max)
	max = delta;
    }
  /* compute average time */
  avg = avg / NUM_ITER;

  /* pretty simple, let's see how it works */
  if (max < 6 * avg)
    max = 6 * avg;
  /* round up to the nearest microsecond */
  ret = (int) ((max + 999) / 1000);
  return ret;
}

static int
init_thread_intf ()
{
  void *dlflag = RTLD_NEXT;
  int err = 0;
  /* if we detect recursion/reentrance, SEGV so we can get a stack */
  init_thread_intf_started++;
  if (!init_thread_intf_finished && init_thread_intf_started >= 3)
    {
      /* pull the plug if recursion occurs... */
      abort ();
    }
  /* lookup fprint to print fatal error message */
  void *ptr = dlsym (RTLD_DEFAULT, "fprintf");
  if (ptr)
    {
      __real_fprintf = (void *) ptr;
    }
  else
    {
      abort ();
    }

  /* find the __collector_jprofile_enable_synctrace routine in jprofile module */
  ptr = dlsym (RTLD_DEFAULT, "__collector_jprofile_enable_synctrace");
  if (ptr)
    __real___collector_jprofile_enable_synctrace = (void *) ptr;
  else
    {
#if defined(GPROFNG_JAVA_PROFILING)
      CALL_REAL (fprintf)(stderr, "synctrace_init COL_ERROR_SYNCINIT __collector_jprofile_enable_synctrace\n");
      err = COL_ERROR_SYNCINIT;
#endif
      sync_java = 0;
    }

#if WSIZE(32)
  /* ########################################## begin WSIZE(32) */
  /* IMPORTANT!!  The GLIBC_* versions below must match those in the er_sync.*.mapfile ! */
  dlflag = RTLD_NEXT;
  ptr = dlvsym (dlflag, "pthread_mutex_lock", "GLIBC_2.0");
  if (ptr == NULL)
    {
      /* We are probably dlopened after libthread/libc,
       * try to search in the previously loaded objects
       */
      dlflag = RTLD_DEFAULT;
      ptr = dlvsym (dlflag, "pthread_mutex_lock", "GLIBC_2.0");
      if (ptr != NULL)
	{
	  __real_pthread_mutex_lock = ptr;
	  Tprintf (0, "synctrace: WARNING: init_thread_intf() using RTLD_DEFAULT for OS sync routines\n");
	}
      else
	{
	  CALL_REAL (fprintf)(stderr, "synctrace_init COL_ERROR_SYNCINIT pthread_mutex_lock\n");
	  err = COL_ERROR_SYNCINIT;
	}
    }
  else
    __real_pthread_mutex_lock = ptr;

  ptr = dlvsym (dlflag, "pthread_mutex_unlock", "GLIBC_2.0");
  if (ptr)
    __real_pthread_mutex_unlock = (void *) ptr;
  else
    {
      CALL_REAL (fprintf)(stderr, "synctrace_init COL_ERROR_SYNCINIT pthread_mutex_unlock\n");
      err = COL_ERROR_SYNCINIT;
    }
  ptr = dlvsym (dlflag, "pthread_cond_wait", "GLIBC_2.3.2");
  if (ptr)
    __real_pthread_cond_wait = (void *) ptr;
  else
    {
      CALL_REAL (fprintf)(stderr, "synctrace_init COL_ERROR_SYNCINIT pthread_cond_wait\n");
      err = COL_ERROR_SYNCINIT;
    }
  ptr = dlvsym (dlflag, "pthread_cond_timedwait", "GLIBC_2.3.2");
  if (ptr)
    __real_pthread_cond_timedwait = (void *) ptr;
  else
    {
      CALL_REAL (fprintf)(stderr, "synctrace_init COL_ERROR_SYNCINIT pthread_cond_timedwait\n");
      err = COL_ERROR_SYNCINIT;
    }
  ptr = dlvsym (dlflag, "pthread_join", "GLIBC_2.0");
  if (ptr)
    __real_pthread_join = (void *) ptr;
  else
    {
      CALL_REAL (fprintf)(stderr, "synctrace_init COL_ERROR_SYNCINIT pthread_join\n");
      err = COL_ERROR_SYNCINIT;
    }
  ptr = dlvsym (dlflag, "sem_wait", "GLIBC_2.1");
  if (ptr)
    __real_sem_wait = (void *) ptr;
  else
    {
      CALL_REAL (fprintf)(stderr, "synctrace_init COL_ERROR_SYNCINIT sem_wait\n");
      err = COL_ERROR_SYNCINIT;
    }

#if ARCH(Intel)
  /* ############## Intel specific additional pointers for 32-bits */
  ptr = __real_sem_wait_2_1 = __real_sem_wait;
  ptr = dlvsym (dlflag, "sem_wait", "GLIBC_2.0");
  if (ptr)
    __real_sem_wait_2_0 = (void *) ptr;
  else
    {
      CALL_REAL (fprintf)(stderr, "synctrace_init COL_ERROR_SYNCINIT sem_wait_2_0\n");
      err = COL_ERROR_SYNCINIT;
    }
  ptr = dlvsym (dlflag, "pthread_cond_wait", "GLIBC_2.0");
  if (ptr)
    __real_pthread_cond_wait_2_0 = (void *) ptr;
  else
    {
      CALL_REAL (fprintf)(stderr, "synctrace_init COL_ERROR_SYNCINIT pthread_cond_wait_2_0\n");
      err = COL_ERROR_SYNCINIT;
    }
  ptr = dlvsym (dlflag, "pthread_cond_timedwait", "GLIBC_2.0");
  if (ptr)
    __real_pthread_cond_timedwait_2_0 = (void *) ptr;
  else
    {
      CALL_REAL (fprintf)(stderr, "synctrace_init COL_ERROR_SYNCINIT __real_pthread_cond_timedwait_2_0\n");
      err = COL_ERROR_SYNCINIT;
    }
#endif /* ARCH(Intel) */

#else /* WSIZE(64) */
  /* # most versions are different between platforms	*/
  /* # the few that are common are set after the ARCH ifdef */
#if ARCH(Aarch64)
  dlflag = RTLD_NEXT;
#define GLIBC_N    "GLIBC_2.17"
  __real_pthread_mutex_lock = dlvsym(dlflag, "pthread_mutex_lock", GLIBC_N);
  __real_pthread_mutex_unlock = dlvsym(dlflag, "pthread_mutex_unlock", GLIBC_N);
  __real_pthread_cond_wait = dlvsym(dlflag, "pthread_cond_wait", GLIBC_N);
  __real_pthread_cond_timedwait = dlvsym(dlflag, "pthread_cond_timedwait", GLIBC_N);
  __real_pthread_join = dlvsym(dlflag, "pthread_join", GLIBC_N);
  __real_sem_wait = dlvsym(dlflag, "sem_wait", GLIBC_N);

#elif ARCH(Intel)
  dlflag = RTLD_NEXT;
  ptr = dlvsym (dlflag, "pthread_mutex_lock", "GLIBC_2.2.5");
  if (ptr == NULL)
    {
      /* We are probably dlopened after libthread/libc,
       * try to search in the previously loaded objects
       */
      dlflag = RTLD_DEFAULT;
      ptr = dlvsym (dlflag, "pthread_mutex_lock", "GLIBC_2.2.5");
      if (ptr != NULL)
	{
	  __real_pthread_mutex_lock = ptr;
	  Tprintf (0, "synctrace: WARNING: init_thread_intf() using RTLD_DEFAULT for Solaris sync routines\n");
	}
      else
	{
	  CALL_REAL (fprintf)(stderr, "synctrace_init COL_ERROR_SYNCINIT pthread_mutex_lock\n");
	  err = COL_ERROR_SYNCINIT;
	}
    }
  else
    __real_pthread_mutex_lock = ptr;
  ptr = dlvsym (dlflag, "pthread_mutex_unlock", "GLIBC_2.2.5");
  if (ptr)
    __real_pthread_mutex_unlock = (void *) ptr;
  else
    {
      CALL_REAL (fprintf)(stderr, "synctrace_init COL_ERROR_SYNCINIT pthread_mutex_unlock\n");
      err = COL_ERROR_SYNCINIT;
    }
  ptr = dlvsym (dlflag, "pthread_cond_wait", "GLIBC_2.3.2");
  if (ptr)
    __real_pthread_cond_wait = (void *) ptr;
  else
    {
      CALL_REAL (fprintf)(stderr, "synctrace_init COL_ERROR_SYNCINIT pthread_cond_wait\n");
      err = COL_ERROR_SYNCINIT;
    }
  ptr = dlvsym (dlflag, "pthread_cond_timedwait", "GLIBC_2.3.2");
  if (ptr)
    __real_pthread_cond_timedwait = (void *) ptr;
  else
    {
      CALL_REAL (fprintf)(stderr, "synctrace_init COL_ERROR_SYNCINIT pthread_cond_timedwait\n");
      err = COL_ERROR_SYNCINIT;
    }
  ptr = dlvsym (dlflag, "pthread_join", "GLIBC_2.2.5");
  if (ptr)
    __real_pthread_join = (void *) ptr;
  else
    {
      CALL_REAL (fprintf)(stderr, "synctrace_init COL_ERROR_SYNCINIT pthread_join\n");
      err = COL_ERROR_SYNCINIT;
    }
  ptr = dlvsym (dlflag, "sem_wait", "GLIBC_2.2.5");
  if (ptr)
    __real_sem_wait = (void *) ptr;
  else
    {
      CALL_REAL (fprintf)(stderr, "synctrace_init COL_ERROR_SYNCINIT sem_wait\n");
      err = COL_ERROR_SYNCINIT;
    }
  ptr = dlvsym (dlflag, "pthread_cond_wait", "GLIBC_2.2.5");
  if (ptr)
    __real_pthread_cond_wait_2_2_5 = (void *) ptr;
  else
    {
      CALL_REAL (fprintf)(stderr, "synctrace_init COL_ERROR_SYNCINIT pthread_cond_wait_2_2_5\n");
      err = COL_ERROR_SYNCINIT;
    }
  ptr = dlvsym (dlflag, "pthread_cond_timedwait", "GLIBC_2.2.5");
  if (ptr)
    __real_pthread_cond_timedwait_2_2_5 = (void *) ptr;
  else
    {
      CALL_REAL (fprintf)(stderr, "synctrace_init COL_ERROR_SYNCINIT pthread_cond_timedwait_2_2_5\n");
      err = COL_ERROR_SYNCINIT;
    }

#elif ARCH(SPARC)
  dlflag = RTLD_NEXT;
  ptr = dlvsym (dlflag, "pthread_mutex_lock", "GLIBC_2.2");
  if (ptr == NULL)
    {
      /* We are probably dlopened after libthread/libc,
       * try to search in the previously loaded objects
       */
      dlflag = RTLD_DEFAULT;
      ptr = dlvsym (dlflag, "pthread_mutex_lock", "GLIBC_2.2");
      if (ptr != NULL)
	{
	  __real_pthread_mutex_lock = ptr;
	  Tprintf (0, "synctrace: WARNING: init_thread_intf() using RTLD_DEFAULT for Solaris sync routines\n");
	}
      else
	{
	  CALL_REAL (fprintf)(stderr, "synctrace_init COL_ERROR_SYNCINIT mutex_lock\n");
	  err = COL_ERROR_SYNCINIT;
	}
    }
  else
    __real_pthread_mutex_lock = ptr;
  ptr = dlvsym (dlflag, "pthread_mutex_unlock", "GLIBC_2.2");
  if (ptr)
    __real_pthread_mutex_unlock = (void *) ptr;
  else
    {
      CALL_REAL (fprintf)(stderr, "synctrace_init COL_ERROR_SYNCINIT pthread_mutex_unlock\n");
      err = COL_ERROR_SYNCINIT;
    }
  ptr = dlvsym (dlflag, "pthread_cond_wait", "GLIBC_2.3.2");
  if (ptr)
    __real_pthread_cond_wait = (void *) ptr;
  else
    {
      CALL_REAL (fprintf)(stderr, "synctrace_init COL_ERROR_SYNCINIT pthread_cond_wait\n");
      err = COL_ERROR_SYNCINIT;
    }
  ptr = dlvsym (dlflag, "pthread_cond_timedwait", "GLIBC_2.3.2");
  if (ptr)
    __real_pthread_cond_timedwait = (void *) ptr;
  else
    {
      CALL_REAL (fprintf)(stderr, "synctrace_init COL_ERROR_SYNCINIT pthread_cond_timedwait\n");
      err = COL_ERROR_SYNCINIT;
    }
  ptr = dlvsym (dlflag, "pthread_join", "GLIBC_2.2");
  if (ptr)
    __real_pthread_join = (void *) ptr;
  else
    {
      CALL_REAL (fprintf)(stderr, "synctrace_init COL_ERROR_SYNCINIT pthread_join\n");
      err = COL_ERROR_SYNCINIT;
    }
  ptr = dlvsym (dlflag, "sem_wait", "GLIBC_2.2");
  if (ptr)
    __real_sem_wait = (void *) ptr;
  else
    {
      CALL_REAL (fprintf)(stderr, "synctrace_init COL_ERROR_SYNCINIT sem_wait\n");
      err = COL_ERROR_SYNCINIT;
    }
  ptr = dlvsym (dlflag, "pthread_cond_wait", "GLIBC_2.2");
  if (ptr)
    __real_pthread_cond_wait_2_2 = (void *) ptr;
  else
    {
      CALL_REAL (fprintf)(stderr, "synctrace_init COL_ERROR_SYNCINIT pthread_cond_wait_2_2_5\n");
      err = COL_ERROR_SYNCINIT;
    }
  ptr = dlvsym (dlflag, "pthread_cond_timedwait", "GLIBC_2.2");
  if (ptr)
    __real_pthread_cond_timedwait_2_2 = (void *) ptr;
  else
    {
      CALL_REAL (fprintf)(stderr, "synctrace_init COL_ERROR_SYNCINIT pthread_cond_timedwait_2_2\n");
      err = COL_ERROR_SYNCINIT;
    }
#endif /* ARCH() */
#endif /* WSIZE(64) */
  /*  the pointers that are common to 32- and 64-bits, and to SPARC and Intel */

  __real_pthread_cond_wait_2_3_2 = __real_pthread_cond_wait;
  __real_pthread_cond_timedwait_2_3_2 = __real_pthread_cond_timedwait;
  ptr = dlsym (dlflag, "strtol");
  if (ptr)
    __real_strtol = (void *) ptr;
  else
    {
      CALL_REAL (fprintf)(stderr, "synctrace_init COL_ERROR_SYNCINIT strtol\n");
      err = COL_ERROR_SYNCINIT;
    }
  init_thread_intf_finished++;
  TprintfT (0, "synctrace init_thread_intf complete\n");
  return err;
}

/* These next two routines are used from jprofile to record Java synctrace data */
void
__collector_jsync_begin ()
{
  int *guard;
  if (CHCK_JREENTRANCE (guard))
    {
      Tprintf (DBG_LT1, "__collector_jsync_begin: skipped\n");
      return;
    }
  Tprintf (DBG_LT1, "__collector_jsync_begin: start event\n");
  PUSH_REENTRANCE (guard);
}

void
__collector_jsync_end (hrtime_t reqt, void *object)
{
  int *guard;
  if (RECHCK_JREENTRANCE (guard))
    {
      Tprintf (DBG_LT1, "__collector_jsync_end: skipped\n");
      return;
    }
  hrtime_t grnt = gethrtime ();
  if (grnt - reqt >= sync_threshold)
    {
      Sync_packet spacket;
      collector_memset (&spacket, 0, sizeof (Sync_packet));
      spacket.comm.tsize = sizeof (Sync_packet);
      spacket.comm.tstamp = grnt;
      spacket.requested = reqt;
      spacket.objp = (intptr_t) object;
      spacket.comm.frinfo = collector_interface->getFrameInfo (sync_hndl, spacket.comm.tstamp, FRINFO_FROM_STACK_ARG, &spacket);
      collector_interface->writeDataRecord (sync_hndl, (Common_packet*) & spacket);
    }
  Tprintf (DBG_LT1, "__collector_jsync_begin: end event\n");
  POP_REENTRANCE (guard);
}

/*-------------------------------------------------------- pthread_mutex_lock */
int
pthread_mutex_lock (pthread_mutex_t *mp)
{
  int *guard;
  if (NULL_PTR (pthread_mutex_lock))
    init_thread_intf ();
  if (CHCK_NREENTRANCE (guard))
    return CALL_REAL (pthread_mutex_lock)(mp);
  PUSH_REENTRANCE (guard);
  hrtime_t reqt = gethrtime ();
  int ret = CALL_REAL (pthread_mutex_lock)(mp);
  if (RECHCK_NREENTRANCE (guard))
    {
      POP_REENTRANCE (guard);
      return ret;
    }
  hrtime_t grnt = gethrtime ();
  if (grnt - reqt >= sync_threshold)
    {
      Sync_packet spacket;
      collector_memset (&spacket, 0, sizeof (Sync_packet));
      spacket.comm.tsize = sizeof (Sync_packet);
      spacket.comm.tstamp = grnt;
      spacket.requested = reqt;
      spacket.objp = (intptr_t) mp;
      spacket.comm.frinfo = collector_interface->getFrameInfo (sync_hndl, spacket.comm.tstamp, FRINFO_FROM_STACK, &spacket);
      collector_interface->writeDataRecord (sync_hndl, (Common_packet*) & spacket);
    }
  POP_REENTRANCE (guard);
  return ret;
}


/*------------------------------------------------------------- pthread_cond_wait */
// map interposed symbol versions
static int
__collector_pthread_cond_wait_symver (int(real_pthread_cond_wait) (), pthread_cond_t *cond, pthread_mutex_t *mutex);

#if ARCH(Intel) || ARCH(SPARC)
SYMVER_ATTRIBUTE (__collector_pthread_cond_wait_2_3_2,
		  pthread_cond_wait@@GLIBC_2.3.2)
#endif
int
__collector_pthread_cond_wait_2_3_2 (pthread_cond_t *cond, pthread_mutex_t *mutex)
{
  if (NULL_PTR (pthread_cond_wait))
    init_thread_intf ();
  TprintfT (DBG_LTT, "linetrace: GLIBC: __collector_pthread_cond_wait_2_3_2@%p\n", CALL_REAL (pthread_cond_wait_2_3_2));
  return __collector_pthread_cond_wait_symver (CALL_REAL (pthread_cond_wait_2_3_2), cond, mutex);
}

#if WSIZE(32)

SYMVER_ATTRIBUTE (__collector_pthread_cond_wait_2_0,
		  pthread_cond_wait@GLIBC_2.0)
int
__collector_pthread_cond_wait_2_0 (pthread_cond_t *cond, pthread_mutex_t *mutex)
{
  if (NULL_PTR (pthread_cond_wait))
    init_thread_intf ();
  TprintfT (DBG_LTT, "linetrace: GLIBC: __collector_pthread_cond_wait_2_0@%p\n", CALL_REAL (pthread_cond_wait_2_0));
  return __collector_pthread_cond_wait_symver (CALL_REAL (pthread_cond_wait_2_0), cond, mutex);
}
#else // WSIZE(64)
#if ARCH(Intel)
SYMVER_ATTRIBUTE (__collector_pthread_cond_wait_2_2_5,
		  pthread_cond_wait@GLIBC_2.2.5)
int
__collector_pthread_cond_wait_2_2_5 (pthread_cond_t *cond, pthread_mutex_t *mutex)
{
  if (NULL_PTR (pthread_cond_wait))
    init_thread_intf ();
  TprintfT (DBG_LTT, "linetrace: GLIBC: __collector_pthread_cond_wait_2_2_5@%p\n", CALL_REAL (pthread_cond_wait_2_2_5));
  return __collector_pthread_cond_wait_symver (CALL_REAL (pthread_cond_wait_2_2_5), cond, mutex);
}
#elif ARCH(SPARC)

SYMVER_ATTRIBUTE (__collector_pthread_cond_wait_2_2,
		  pthread_cond_wait@GLIBC_2.2)
int
__collector_pthread_cond_wait_2_2 (pthread_cond_t *cond, pthread_mutex_t *mutex)
{
  if (NULL_PTR (pthread_cond_wait))
    init_thread_intf ();
  TprintfT (DBG_LTT, "linetrace: GLIBC: __collector_pthread_cond_wait_2_2@%p\n", CALL_REAL (pthread_cond_wait_2_2));
  return __collector_pthread_cond_wait_symver (CALL_REAL (pthread_cond_wait_2_2), cond, mutex);
}
#endif  // ARCH()
#endif  // WSIZE()

static int
__collector_pthread_cond_wait_symver (int(real_pthread_cond_wait) (), pthread_cond_t *cond, pthread_mutex_t *mutex)
{
  int *guard;
  if (NULL_PTR (pthread_cond_wait))
    init_thread_intf ();
  if (CHCK_NREENTRANCE (guard))
    return (real_pthread_cond_wait) (cond, mutex);
  PUSH_REENTRANCE (guard);
  hrtime_t reqt = gethrtime ();
  int ret = -1;
  ret = (real_pthread_cond_wait) (cond, mutex);
  if (RECHCK_NREENTRANCE (guard))
    {
      POP_REENTRANCE (guard);
      return ret;
    }
  hrtime_t grnt = gethrtime ();
  if (grnt - reqt >= sync_threshold)
    {
      Sync_packet spacket;
      collector_memset (&spacket, 0, sizeof (Sync_packet));
      spacket.comm.tsize = sizeof (Sync_packet);
      spacket.comm.tstamp = grnt;
      spacket.requested = reqt;
      spacket.objp = (intptr_t) mutex;
      spacket.comm.frinfo = collector_interface->getFrameInfo (sync_hndl, spacket.comm.tstamp, FRINFO_FROM_STACK_ARG, &spacket);
      collector_interface->writeDataRecord (sync_hndl, (Common_packet*) & spacket);
    }
  POP_REENTRANCE (guard);
  return ret;
}

/*---------------------------------------------------- pthread_cond_timedwait */
// map interposed symbol versions
static int
__collector_pthread_cond_timedwait_symver (int(real_pthread_cond_timedwait) (),
					   pthread_cond_t *cond,
					   pthread_mutex_t *mutex,
					   const struct timespec *abstime);

#if ARCH(Intel) || ARCH(SPARC)
SYMVER_ATTRIBUTE (__collector_pthread_cond_timedwait_2_3_2,
		  pthread_cond_timedwait@@GLIBC_2.3.2)
#endif  // ARCH()
int
__collector_pthread_cond_timedwait_2_3_2 (pthread_cond_t *cond,
					  pthread_mutex_t *mutex,
					  const struct timespec *abstime)
{
  if (NULL_PTR (pthread_cond_timedwait))
    init_thread_intf ();
  TprintfT (DBG_LTT, "linetrace: GLIBC: __collector_pthread_cond_timedwait_2_3_2@%p\n", CALL_REAL (pthread_cond_timedwait_2_3_2));
  return __collector_pthread_cond_timedwait_symver (CALL_REAL (pthread_cond_timedwait_2_3_2), cond, mutex, abstime);
}

#if WSIZE(32)
SYMVER_ATTRIBUTE (__collector_pthread_cond_timedwait_2_0,
		  pthread_cond_timedwait@GLIBC_2.0)
int
__collector_pthread_cond_timedwait_2_0 (pthread_cond_t *cond,
					pthread_mutex_t *mutex,
					const struct timespec *abstime)
{
  if (NULL_PTR (pthread_cond_timedwait))
    init_thread_intf ();
  TprintfT (DBG_LTT, "linetrace: GLIBC: __collector_pthread_cond_timedwait_2_0@%p\n", CALL_REAL (pthread_cond_timedwait_2_0));
  return __collector_pthread_cond_timedwait_symver (CALL_REAL (pthread_cond_timedwait_2_0), cond, mutex, abstime);
}
#else // WSIZE(64)
#if ARCH(Intel)
SYMVER_ATTRIBUTE (__collector_pthread_cond_timedwait_2_2_5,
		  pthread_cond_timedwait@GLIBC_2.2.5)
int
__collector_pthread_cond_timedwait_2_2_5 (pthread_cond_t *cond,
					  pthread_mutex_t *mutex,
					  const struct timespec *abstime)
{
  if (NULL_PTR (pthread_cond_timedwait))
    init_thread_intf ();
  TprintfT (DBG_LTT, "linetrace: GLIBC: __collector_pthread_cond_timedwait_2_2_5@%p\n", CALL_REAL (pthread_cond_timedwait_2_2_5));
  return __collector_pthread_cond_timedwait_symver (CALL_REAL (pthread_cond_timedwait_2_2_5), cond, mutex, abstime);
}
#elif ARCH(SPARC)

SYMVER_ATTRIBUTE (__collector_pthread_cond_timedwait_2_2,
		  pthread_cond_timedwait@GLIBC_2.2)
int
__collector_pthread_cond_timedwait_2_2 (pthread_cond_t *cond,
					pthread_mutex_t *mutex,
					const struct timespec *abstime)
{
  if (NULL_PTR (pthread_cond_timedwait))
    init_thread_intf ();
  TprintfT (DBG_LTT, "linetrace: GLIBC: __collector_pthread_cond_timedwait_2_2@%p\n", CALL_REAL (pthread_cond_timedwait_2_2));
  return __collector_pthread_cond_timedwait_symver (CALL_REAL (pthread_cond_timedwait_2_2), cond, mutex, abstime);
}
#endif  // ARCH()
#endif  // WSIZE()

static int
__collector_pthread_cond_timedwait_symver (int(real_pthread_cond_timedwait) (),
					   pthread_cond_t *cond,
					   pthread_mutex_t *mutex,
					   const struct timespec *abstime)
{
  int *guard;
  if (NULL_PTR (pthread_cond_timedwait))
    init_thread_intf ();
  if (CHCK_NREENTRANCE (guard))
    return (real_pthread_cond_timedwait) (cond, mutex, abstime);
  PUSH_REENTRANCE (guard);
  hrtime_t reqt = gethrtime ();
  int ret = -1;
  ret = (real_pthread_cond_timedwait) (cond, mutex, abstime);
  if (RECHCK_NREENTRANCE (guard))
    {
      POP_REENTRANCE (guard);
      return ret;
    }
  hrtime_t grnt = gethrtime ();
  if (grnt - reqt >= sync_threshold)
    {
      Sync_packet spacket;
      collector_memset (&spacket, 0, sizeof ( Sync_packet));
      spacket.comm.tsize = sizeof ( Sync_packet);
      spacket.comm.tstamp = grnt;
      spacket.requested = reqt;
      spacket.objp = (intptr_t) mutex;
      spacket.comm.frinfo = collector_interface->getFrameInfo (sync_hndl, spacket.comm.tstamp, FRINFO_FROM_STACK_ARG, &spacket);
      collector_interface->writeDataRecord (sync_hndl, (Common_packet*) & spacket);
    }
  POP_REENTRANCE (guard);
  return ret;
}

/*------------------------------------------------------------- pthread_join */
int
pthread_join (pthread_t target_thread, void **status)
{
  int *guard;
  if (NULL_PTR (pthread_join))
    init_thread_intf ();
  if (CHCK_NREENTRANCE (guard))
    return CALL_REAL (pthread_join)(target_thread, status);
  PUSH_REENTRANCE (guard);
  hrtime_t reqt = gethrtime ();
  int ret = CALL_REAL (pthread_join)(target_thread, status);
  if (RECHCK_NREENTRANCE (guard))
    {
      POP_REENTRANCE (guard);
      return ret;
    }
  hrtime_t grnt = gethrtime ();
  if (grnt - reqt >= sync_threshold)
    {
      Sync_packet spacket;
      collector_memset (&spacket, 0, sizeof ( Sync_packet));
      spacket.comm.tsize = sizeof ( Sync_packet);
      spacket.comm.tstamp = grnt;
      spacket.requested = reqt;
      spacket.objp = (Vaddr_type) target_thread;
      spacket.comm.frinfo = collector_interface->getFrameInfo (sync_hndl, spacket.comm.tstamp, FRINFO_FROM_STACK, &spacket);
      collector_interface->writeDataRecord (sync_hndl, (Common_packet*) & spacket);
    }
  POP_REENTRANCE (guard);
  return ret;
}

/*------------------------------------------------------------- sem_wait */
// map interposed symbol versions
#if ARCH(Intel) && WSIZE(32)
static int
__collector_sem_wait_symver (int(real_sem_wait) (), sem_t *sp);

SYMVER_ATTRIBUTE (__collector_sem_wait_2_1, sem_wait@@GLIBC_2.1)
int
__collector_sem_wait_2_1 (sem_t *sp)
{
  if (NULL_PTR (sem_wait))
    init_thread_intf ();
  TprintfT (DBG_LTT, "linetrace: GLIBC: __collector_sem_wait_2_1@%p\n", CALL_REAL (sem_wait_2_1));
  return __collector_sem_wait_symver (CALL_REAL (sem_wait_2_1), sp);
}

SYMVER_ATTRIBUTE (__collector_sem_wait_2_0, sem_wait@GLIBC_2.0)
int
__collector_sem_wait_2_0 (sem_t *sp)
{
  if (NULL_PTR (sem_wait))
    init_thread_intf ();
  TprintfT (DBG_LTT, "linetrace: GLIBC: __collector_sem_wait_2_0@%p\n", CALL_REAL (sem_wait_2_0));
  return __collector_sem_wait_symver (CALL_REAL (sem_wait_2_0), sp);
}
#endif

#if ARCH(Intel) && WSIZE(32)
static int
__collector_sem_wait_symver (int(real_sem_wait) (), sem_t *sp)
{
#else
int
sem_wait (sem_t *sp)
{
#endif
  int *guard;
  if (NULL_PTR (sem_wait))
    init_thread_intf ();
  if (CHCK_NREENTRANCE (guard))
    {
#if ARCH(Intel) && WSIZE(32)
      return (real_sem_wait) (sp);
#else
      return CALL_REAL (sem_wait)(sp);
#endif
    }
  PUSH_REENTRANCE (guard);
  hrtime_t reqt = gethrtime ();
  int ret = -1;
#if ARCH(Intel) && WSIZE(32)
  ret = (real_sem_wait) (sp);
#else
  ret = CALL_REAL (sem_wait)(sp);
#endif
  if (RECHCK_NREENTRANCE (guard))
    {
      POP_REENTRANCE (guard);
      return ret;
    }
  hrtime_t grnt = gethrtime ();
  if (grnt - reqt >= sync_threshold)
    {
      Sync_packet spacket;
      collector_memset (&spacket, 0, sizeof ( Sync_packet));
      spacket.comm.tsize = sizeof ( Sync_packet);
      spacket.comm.tstamp = grnt;
      spacket.requested = reqt;
      spacket.objp = (intptr_t) sp;

#if ARCH(Intel) && WSIZE(32)
      spacket.comm.frinfo = collector_interface->getFrameInfo (sync_hndl, spacket.comm.tstamp, FRINFO_FROM_STACK_ARG, &spacket);
#else
      spacket.comm.frinfo = collector_interface->getFrameInfo (sync_hndl, spacket.comm.tstamp, FRINFO_FROM_STACK, &spacket);
#endif
      collector_interface->writeDataRecord (sync_hndl, (Common_packet*) & spacket);
    }
  POP_REENTRANCE (guard);
  return ret;
}
