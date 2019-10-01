/* Plugin for NVPTX execution.

   Copyright (C) 2013-2018 Free Software Foundation, Inc.

   Contributed by Mentor Embedded.

   This file is part of the GNU Offloading and Multi Processing Library
   (libgomp).

   Libgomp is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   Libgomp is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
   FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
   more details.

   Under Section 7 of GPL version 3, you are granted additional
   permissions described in the GCC Runtime Library Exception, version
   3.1, as published by the Free Software Foundation.

   You should have received a copy of the GNU General Public License and
   a copy of the GCC Runtime Library Exception along with this program;
   see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
   <http://www.gnu.org/licenses/>.  */

/* Nvidia PTX-specific parts of OpenACC support.  The cuda driver
   library appears to hold some implicit state, but the documentation
   is not clear as to what that state might be.  Or how one might
   propagate it from one thread to another.  */

#include "openacc.h"
#include "config.h"
#include "libgomp-plugin.h"
#include "oacc-plugin.h"
#include "gomp-constants.h"

#include <pthread.h>
#include <cuda.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>

#if PLUGIN_NVPTX_DYNAMIC
# include <dlfcn.h>

# define CUDA_CALLS \
CUDA_ONE_CALL (cuCtxCreate)		\
CUDA_ONE_CALL (cuCtxDestroy)		\
CUDA_ONE_CALL (cuCtxGetCurrent)		\
CUDA_ONE_CALL (cuCtxGetDevice)		\
CUDA_ONE_CALL (cuCtxPopCurrent)		\
CUDA_ONE_CALL (cuCtxPushCurrent)	\
CUDA_ONE_CALL (cuCtxSynchronize)	\
CUDA_ONE_CALL (cuDeviceGet)		\
CUDA_ONE_CALL (cuDeviceGetAttribute)	\
CUDA_ONE_CALL (cuDeviceGetCount)	\
CUDA_ONE_CALL (cuEventCreate)		\
CUDA_ONE_CALL (cuEventDestroy)		\
CUDA_ONE_CALL (cuEventElapsedTime)	\
CUDA_ONE_CALL (cuEventQuery)		\
CUDA_ONE_CALL (cuEventRecord)		\
CUDA_ONE_CALL (cuEventSynchronize)	\
CUDA_ONE_CALL (cuFuncGetAttribute)	\
CUDA_ONE_CALL (cuGetErrorString)	\
CUDA_ONE_CALL (cuInit)			\
CUDA_ONE_CALL (cuLaunchKernel)		\
CUDA_ONE_CALL (cuLinkAddData)		\
CUDA_ONE_CALL (cuLinkComplete)		\
CUDA_ONE_CALL (cuLinkCreate)		\
CUDA_ONE_CALL (cuLinkDestroy)		\
CUDA_ONE_CALL (cuMemAlloc)		\
CUDA_ONE_CALL (cuMemAllocHost)		\
CUDA_ONE_CALL (cuMemcpy)		\
CUDA_ONE_CALL (cuMemcpyDtoDAsync)	\
CUDA_ONE_CALL (cuMemcpyDtoH)		\
CUDA_ONE_CALL (cuMemcpyDtoHAsync)	\
CUDA_ONE_CALL (cuMemcpyHtoD)		\
CUDA_ONE_CALL (cuMemcpyHtoDAsync)	\
CUDA_ONE_CALL (cuMemFree)		\
CUDA_ONE_CALL (cuMemFreeHost)		\
CUDA_ONE_CALL (cuMemGetAddressRange)	\
CUDA_ONE_CALL (cuMemHostGetDevicePointer)\
CUDA_ONE_CALL (cuModuleGetFunction)	\
CUDA_ONE_CALL (cuModuleGetGlobal)	\
CUDA_ONE_CALL (cuModuleLoad)		\
CUDA_ONE_CALL (cuModuleLoadData)	\
CUDA_ONE_CALL (cuModuleUnload)		\
CUDA_ONE_CALL (cuStreamCreate)		\
CUDA_ONE_CALL (cuStreamDestroy)		\
CUDA_ONE_CALL (cuStreamQuery)		\
CUDA_ONE_CALL (cuStreamSynchronize)	\
CUDA_ONE_CALL (cuStreamWaitEvent)
# define CUDA_ONE_CALL(call) \
  __typeof (call) *call;
struct cuda_lib_s {
  CUDA_CALLS
} cuda_lib;

/* -1 if init_cuda_lib has not been called yet, false
   if it has been and failed, true if it has been and succeeded.  */
static signed char cuda_lib_inited = -1;

/* Dynamically load the CUDA runtime library and initialize function
   pointers, return false if unsuccessful, true if successful.  */
static bool
init_cuda_lib (void)
{
  if (cuda_lib_inited != -1)
    return cuda_lib_inited;
  const char *cuda_runtime_lib = "libcuda.so.1";
  void *h = dlopen (cuda_runtime_lib, RTLD_LAZY);
  cuda_lib_inited = false;
  if (h == NULL)
    return false;
# undef CUDA_ONE_CALL
# define CUDA_ONE_CALL(call) CUDA_ONE_CALL_1 (call)
# define CUDA_ONE_CALL_1(call) \
  cuda_lib.call = dlsym (h, #call);	\
  if (cuda_lib.call == NULL)		\
    return false;
  CUDA_CALLS
  cuda_lib_inited = true;
  return true;
}
# undef CUDA_ONE_CALL
# undef CUDA_ONE_CALL_1
# define CUDA_CALL_PREFIX cuda_lib.
#else
# define CUDA_CALL_PREFIX
# define init_cuda_lib() true
#endif

/* Convenience macros for the frequently used CUDA library call and
   error handling sequence as well as CUDA library calls that
   do the error checking themselves or don't do it at all.  */

#define CUDA_CALL_ERET(ERET, FN, ...)		\
  do {						\
    unsigned __r				\
      = CUDA_CALL_PREFIX FN (__VA_ARGS__);	\
    if (__r != CUDA_SUCCESS)			\
      {						\
	GOMP_PLUGIN_error (#FN " error: %s",	\
			   cuda_error (__r));	\
	return ERET;				\
      }						\
  } while (0)

#define CUDA_CALL(FN, ...)			\
  CUDA_CALL_ERET (false, FN, __VA_ARGS__)

#define CUDA_CALL_ASSERT(FN, ...)		\
  do {						\
    unsigned __r				\
      = CUDA_CALL_PREFIX FN (__VA_ARGS__);	\
    if (__r != CUDA_SUCCESS)			\
      {						\
	GOMP_PLUGIN_fatal (#FN " error: %s",	\
			   cuda_error (__r));	\
      }						\
  } while (0)

#define CUDA_CALL_NOCHECK(FN, ...)		\
  CUDA_CALL_PREFIX FN (__VA_ARGS__)

static const char *
cuda_error (CUresult r)
{
#if CUDA_VERSION < 7000
  /* Specified in documentation and present in library from at least
     5.5.  Not declared in header file prior to 7.0.  */
  extern CUresult cuGetErrorString (CUresult, const char **);
#endif
  const char *desc;

  r = CUDA_CALL_NOCHECK (cuGetErrorString, r, &desc);
  if (r != CUDA_SUCCESS)
    desc = "unknown cuda error";

  return desc;
}

static unsigned int instantiated_devices = 0;
static pthread_mutex_t ptx_dev_lock = PTHREAD_MUTEX_INITIALIZER;

struct ptx_stream
{
  CUstream stream;
  pthread_t host_thread;
  bool multithreaded;

  CUdeviceptr d;
  void *h;
  void *h_begin;
  void *h_end;
  void *h_next;
  void *h_prev;
  void *h_tail;

  struct ptx_stream *next;
};

/* Thread-specific data for PTX.  */

struct nvptx_thread
{
  struct ptx_stream *current_stream;
  struct ptx_device *ptx_dev;
};

struct map
{
  int     async;
  size_t  size;
  char    mappings[0];
};

static bool
map_init (struct ptx_stream *s)
{
  int size = getpagesize ();

  assert (s);
  assert (!s->d);
  assert (!s->h);

  CUDA_CALL (cuMemAllocHost, &s->h, size);
  CUDA_CALL (cuMemHostGetDevicePointer, &s->d, s->h, 0);

  assert (s->h);

  s->h_begin = s->h;
  s->h_end = s->h_begin + size;
  s->h_next = s->h_prev = s->h_tail = s->h_begin;

  assert (s->h_next);
  assert (s->h_end);
  return true;
}

static bool
map_fini (struct ptx_stream *s)
{
  CUDA_CALL (cuMemFreeHost, s->h);
  return true;
}

static void
map_pop (struct ptx_stream *s)
{
  struct map *m;

  assert (s != NULL);
  assert (s->h_next);
  assert (s->h_prev);
  assert (s->h_tail);

  m = s->h_tail;

  s->h_tail += m->size;

  if (s->h_tail >= s->h_end)
    s->h_tail = s->h_begin + (int) (s->h_tail - s->h_end);

  if (s->h_next == s->h_tail)
    s->h_prev = s->h_next;

  assert (s->h_next >= s->h_begin);
  assert (s->h_tail >= s->h_begin);
  assert (s->h_prev >= s->h_begin);

  assert (s->h_next <= s->h_end);
  assert (s->h_tail <= s->h_end);
  assert (s->h_prev <= s->h_end);
}

static void
map_push (struct ptx_stream *s, int async, size_t size, void **h, void **d)
{
  int left;
  int offset;
  struct map *m;

  assert (s != NULL);

  left = s->h_end - s->h_next;
  size += sizeof (struct map);

  assert (s->h_prev);
  assert (s->h_next);

  if (size >= left)
    {
      m = s->h_prev;
      m->size += left;
      s->h_next = s->h_begin;

      if (s->h_next + size > s->h_end)
	GOMP_PLUGIN_fatal ("unable to push map");
    }

  assert (s->h_next);

  m = s->h_next;
  m->async = async;
  m->size = size;

  offset = (void *)&m->mappings[0] - s->h;

  *d = (void *)(s->d + offset);
  *h = (void *)(s->h + offset);

  s->h_prev = s->h_next;
  s->h_next += size;

  assert (s->h_prev);
  assert (s->h_next);

  assert (s->h_next >= s->h_begin);
  assert (s->h_tail >= s->h_begin);
  assert (s->h_prev >= s->h_begin);
  assert (s->h_next <= s->h_end);
  assert (s->h_tail <= s->h_end);
  assert (s->h_prev <= s->h_end);

  return;
}

/* Target data function launch information.  */

struct targ_fn_launch
{
  const char *fn;
  unsigned short dim[GOMP_DIM_MAX];
};

/* Target PTX object information.  */

struct targ_ptx_obj
{
  const char *code;
  size_t size;
};

/* Target data image information.  */

typedef struct nvptx_tdata
{
  const struct targ_ptx_obj *ptx_objs;
  unsigned ptx_num;

  const char *const *var_names;
  unsigned var_num;

  const struct targ_fn_launch *fn_descs;
  unsigned fn_num;
} nvptx_tdata_t;

/* Descriptor of a loaded function.  */

struct targ_fn_descriptor
{
  CUfunction fn;
  const struct targ_fn_launch *launch;
  int regs_per_thread;
  int max_threads_per_block;
};

/* A loaded PTX image.  */
struct ptx_image_data
{
  const void *target_data;
  CUmodule module;

  struct targ_fn_descriptor *fns;  /* Array of functions.  */
  
  struct ptx_image_data *next;
};

struct ptx_device
{
  CUcontext ctx;
  bool ctx_shared;
  CUdevice dev;
  struct ptx_stream *null_stream;
  /* All non-null streams associated with this device (actually context),
     either created implicitly or passed in from the user (via
     acc_set_cuda_stream).  */
  struct ptx_stream *active_streams;
  struct {
    struct ptx_stream **arr;
    int size;
  } async_streams;
  /* A lock for use when manipulating the above stream list and array.  */
  pthread_mutex_t stream_lock;
  int ord;
  bool overlap;
  bool map;
  bool concur;
  bool mkern;
  int  mode;
  int clock_khz;
  int num_sms;
  int regs_per_block;
  int regs_per_sm;

  struct ptx_image_data *images;  /* Images loaded on device.  */
  pthread_mutex_t image_lock;     /* Lock for above list.  */
  
  struct ptx_device *next;
};

enum ptx_event_type
{
  PTX_EVT_MEM,
  PTX_EVT_KNL,
  PTX_EVT_SYNC,
  PTX_EVT_ASYNC_CLEANUP
};

struct ptx_event
{
  CUevent *evt;
  int type;
  void *addr;
  int ord;
  int val;

  struct ptx_event *next;
};

static pthread_mutex_t ptx_event_lock;
static struct ptx_event *ptx_events;

static struct ptx_device **ptx_devices;

static inline struct nvptx_thread *
nvptx_thread (void)
{
  return (struct nvptx_thread *) GOMP_PLUGIN_acc_thread ();
}

static bool
init_streams_for_device (struct ptx_device *ptx_dev, int concurrency)
{
  int i;
  struct ptx_stream *null_stream
    = GOMP_PLUGIN_malloc (sizeof (struct ptx_stream));

  null_stream->stream = NULL;
  null_stream->host_thread = pthread_self ();
  null_stream->multithreaded = true;
  null_stream->d = (CUdeviceptr) NULL;
  null_stream->h = NULL;
  if (!map_init (null_stream))
    return false;

  ptx_dev->null_stream = null_stream;
  ptx_dev->active_streams = NULL;
  pthread_mutex_init (&ptx_dev->stream_lock, NULL);

  if (concurrency < 1)
    concurrency = 1;

  /* This is just a guess -- make space for as many async streams as the
     current device is capable of concurrently executing.  This can grow
     later as necessary.  No streams are created yet.  */
  ptx_dev->async_streams.arr
    = GOMP_PLUGIN_malloc (concurrency * sizeof (struct ptx_stream *));
  ptx_dev->async_streams.size = concurrency;

  for (i = 0; i < concurrency; i++)
    ptx_dev->async_streams.arr[i] = NULL;

  return true;
}

static bool
fini_streams_for_device (struct ptx_device *ptx_dev)
{
  free (ptx_dev->async_streams.arr);

  bool ret = true;
  while (ptx_dev->active_streams != NULL)
    {
      struct ptx_stream *s = ptx_dev->active_streams;
      ptx_dev->active_streams = ptx_dev->active_streams->next;

      ret &= map_fini (s);

      CUresult r = CUDA_CALL_NOCHECK (cuStreamDestroy, s->stream);
      if (r != CUDA_SUCCESS)
	{
	  GOMP_PLUGIN_error ("cuStreamDestroy error: %s", cuda_error (r));
	  ret = false;
	}
      free (s);
    }

  ret &= map_fini (ptx_dev->null_stream);
  free (ptx_dev->null_stream);
  return ret;
}

/* Select a stream for (OpenACC-semantics) ASYNC argument for the current
   thread THREAD (and also current device/context).  If CREATE is true, create
   the stream if it does not exist (or use EXISTING if it is non-NULL), and
   associate the stream with the same thread argument.  Returns stream to use
   as result.  */

static struct ptx_stream *
select_stream_for_async (int async, pthread_t thread, bool create,
			 CUstream existing)
{
  struct nvptx_thread *nvthd = nvptx_thread ();
  /* Local copy of TLS variable.  */
  struct ptx_device *ptx_dev = nvthd->ptx_dev;
  struct ptx_stream *stream = NULL;
  int orig_async = async;

  /* The special value acc_async_noval (-1) maps (for now) to an
     implicitly-created stream, which is then handled the same as any other
     numbered async stream.  Other options are available, e.g. using the null
     stream for anonymous async operations, or choosing an idle stream from an
     active set.  But, stick with this for now.  */
  if (async > acc_async_sync)
    async++;

  if (create)
    pthread_mutex_lock (&ptx_dev->stream_lock);

  /* NOTE: AFAICT there's no particular need for acc_async_sync to map to the
     null stream, and in fact better performance may be obtainable if it doesn't
     (because the null stream enforces overly-strict synchronisation with
     respect to other streams for legacy reasons, and that's probably not
     needed with OpenACC).  Maybe investigate later.  */
  if (async == acc_async_sync)
    stream = ptx_dev->null_stream;
  else if (async >= 0 && async < ptx_dev->async_streams.size
	   && ptx_dev->async_streams.arr[async] && !(create && existing))
    stream = ptx_dev->async_streams.arr[async];
  else if (async >= 0 && create)
    {
      if (async >= ptx_dev->async_streams.size)
	{
	  int i, newsize = ptx_dev->async_streams.size * 2;

	  if (async >= newsize)
	    newsize = async + 1;

	  ptx_dev->async_streams.arr
	    = GOMP_PLUGIN_realloc (ptx_dev->async_streams.arr,
				   newsize * sizeof (struct ptx_stream *));

	  for (i = ptx_dev->async_streams.size; i < newsize; i++)
	    ptx_dev->async_streams.arr[i] = NULL;

	  ptx_dev->async_streams.size = newsize;
	}

      /* Create a new stream on-demand if there isn't one already, or if we're
	 setting a particular async value to an existing (externally-provided)
	 stream.  */
      if (!ptx_dev->async_streams.arr[async] || existing)
        {
	  CUresult r;
	  struct ptx_stream *s
	    = GOMP_PLUGIN_malloc (sizeof (struct ptx_stream));

	  if (existing)
	    s->stream = existing;
	  else
	    {
	      r = CUDA_CALL_NOCHECK (cuStreamCreate, &s->stream,
				     CU_STREAM_DEFAULT);
	      if (r != CUDA_SUCCESS)
		{
		  pthread_mutex_unlock (&ptx_dev->stream_lock);
		  GOMP_PLUGIN_fatal ("cuStreamCreate error: %s",
				     cuda_error (r));
		}
	    }

	  /* If CREATE is true, we're going to be queueing some work on this
	     stream.  Associate it with the current host thread.  */
	  s->host_thread = thread;
	  s->multithreaded = false;

	  s->d = (CUdeviceptr) NULL;
	  s->h = NULL;
	  if (!map_init (s))
	    {
	      pthread_mutex_unlock (&ptx_dev->stream_lock);
	      GOMP_PLUGIN_fatal ("map_init fail");
	    }

	  s->next = ptx_dev->active_streams;
	  ptx_dev->active_streams = s;
	  ptx_dev->async_streams.arr[async] = s;
	}

      stream = ptx_dev->async_streams.arr[async];
    }
  else if (async < 0)
    {
      if (create)
	pthread_mutex_unlock (&ptx_dev->stream_lock);
      GOMP_PLUGIN_fatal ("bad async %d", async);
    }

  if (create)
    {
      assert (stream != NULL);

      /* If we're trying to use the same stream from different threads
	 simultaneously, set stream->multithreaded to true.  This affects the
	 behaviour of acc_async_test_all and acc_wait_all, which are supposed to
	 only wait for asynchronous launches from the same host thread they are
	 invoked on.  If multiple threads use the same async value, we make note
	 of that here and fall back to testing/waiting for all threads in those
	 functions.  */
      if (thread != stream->host_thread)
        stream->multithreaded = true;

      pthread_mutex_unlock (&ptx_dev->stream_lock);
    }
  else if (stream && !stream->multithreaded
	   && !pthread_equal (stream->host_thread, thread))
    GOMP_PLUGIN_fatal ("async %d used on wrong thread", orig_async);

  return stream;
}

/* Initialize the device.  Return TRUE on success, else FALSE.  PTX_DEV_LOCK
   should be locked on entry and remains locked on exit.  */

static bool
nvptx_init (void)
{
  int ndevs;

  if (instantiated_devices != 0)
    return true;

  ptx_events = NULL;
  pthread_mutex_init (&ptx_event_lock, NULL);

  if (!init_cuda_lib ())
    return false;

  CUDA_CALL (cuInit, 0);

  CUDA_CALL (cuDeviceGetCount, &ndevs);
  ptx_devices = GOMP_PLUGIN_malloc_cleared (sizeof (struct ptx_device *)
					    * ndevs);
  return true;
}

/* Select the N'th PTX device for the current host thread.  The device must
   have been previously opened before calling this function.  */

static bool
nvptx_attach_host_thread_to_device (int n)
{
  CUdevice dev;
  CUresult r;
  struct ptx_device *ptx_dev;
  CUcontext thd_ctx;

  r = CUDA_CALL_NOCHECK (cuCtxGetDevice, &dev);
  if (r != CUDA_SUCCESS && r != CUDA_ERROR_INVALID_CONTEXT)
    {
      GOMP_PLUGIN_error ("cuCtxGetDevice error: %s", cuda_error (r));
      return false;
    }

  if (r != CUDA_ERROR_INVALID_CONTEXT && dev == n)
    return true;
  else
    {
      CUcontext old_ctx;

      ptx_dev = ptx_devices[n];
      if (!ptx_dev)
	{
	  GOMP_PLUGIN_error ("device %d not found", n);
	  return false;
	}

      CUDA_CALL (cuCtxGetCurrent, &thd_ctx);

      /* We don't necessarily have a current context (e.g. if it has been
         destroyed.  Pop it if we do though.  */
      if (thd_ctx != NULL)
	CUDA_CALL (cuCtxPopCurrent, &old_ctx);

      CUDA_CALL (cuCtxPushCurrent, ptx_dev->ctx);
    }
  return true;
}

static struct ptx_device *
nvptx_open_device (int n)
{
  struct ptx_device *ptx_dev;
  CUdevice dev, ctx_dev;
  CUresult r;
  int async_engines, pi;

  CUDA_CALL_ERET (NULL, cuDeviceGet, &dev, n);

  ptx_dev = GOMP_PLUGIN_malloc (sizeof (struct ptx_device));

  ptx_dev->ord = n;
  ptx_dev->dev = dev;
  ptx_dev->ctx_shared = false;

  r = CUDA_CALL_NOCHECK (cuCtxGetDevice, &ctx_dev);
  if (r != CUDA_SUCCESS && r != CUDA_ERROR_INVALID_CONTEXT)
    {
      GOMP_PLUGIN_error ("cuCtxGetDevice error: %s", cuda_error (r));
      return NULL;
    }
  
  if (r != CUDA_ERROR_INVALID_CONTEXT && ctx_dev != dev)
    {
      /* The current host thread has an active context for a different device.
         Detach it.  */
      CUcontext old_ctx;
      CUDA_CALL_ERET (NULL, cuCtxPopCurrent, &old_ctx);
    }

  CUDA_CALL_ERET (NULL, cuCtxGetCurrent, &ptx_dev->ctx);

  if (!ptx_dev->ctx)
    CUDA_CALL_ERET (NULL, cuCtxCreate, &ptx_dev->ctx, CU_CTX_SCHED_AUTO, dev);
  else
    ptx_dev->ctx_shared = true;

  CUDA_CALL_ERET (NULL, cuDeviceGetAttribute,
		  &pi, CU_DEVICE_ATTRIBUTE_GPU_OVERLAP, dev);
  ptx_dev->overlap = pi;

  CUDA_CALL_ERET (NULL, cuDeviceGetAttribute,
		  &pi, CU_DEVICE_ATTRIBUTE_CAN_MAP_HOST_MEMORY, dev);
  ptx_dev->map = pi;

  CUDA_CALL_ERET (NULL, cuDeviceGetAttribute,
		  &pi, CU_DEVICE_ATTRIBUTE_CONCURRENT_KERNELS, dev);
  ptx_dev->concur = pi;

  CUDA_CALL_ERET (NULL, cuDeviceGetAttribute,
		  &pi, CU_DEVICE_ATTRIBUTE_COMPUTE_MODE, dev);
  ptx_dev->mode = pi;

  CUDA_CALL_ERET (NULL, cuDeviceGetAttribute,
		  &pi, CU_DEVICE_ATTRIBUTE_INTEGRATED, dev);
  ptx_dev->mkern = pi;

  CUDA_CALL_ERET (NULL, cuDeviceGetAttribute,
		  &pi, CU_DEVICE_ATTRIBUTE_CLOCK_RATE, dev);
  ptx_dev->clock_khz = pi;

  CUDA_CALL_ERET (NULL, cuDeviceGetAttribute,
		  &pi, CU_DEVICE_ATTRIBUTE_MULTIPROCESSOR_COUNT, dev);
  ptx_dev->num_sms = pi;

  CUDA_CALL_ERET (NULL, cuDeviceGetAttribute,
		  &pi, CU_DEVICE_ATTRIBUTE_MAX_REGISTERS_PER_BLOCK, dev);
  ptx_dev->regs_per_block = pi;

  /* CU_DEVICE_ATTRIBUTE_MAX_REGISTERS_PER_MULTIPROCESSOR = 82 is defined only
     in CUDA 6.0 and newer.  */
  r = CUDA_CALL_NOCHECK (cuDeviceGetAttribute, &pi, 82, dev);
  /* Fallback: use limit of registers per block, which is usually equal.  */
  if (r == CUDA_ERROR_INVALID_VALUE)
    pi = ptx_dev->regs_per_block;
  else if (r != CUDA_SUCCESS)
    {
      GOMP_PLUGIN_error ("cuDeviceGetAttribute error: %s", cuda_error (r));
      return NULL;
    }
  ptx_dev->regs_per_sm = pi;

  CUDA_CALL_ERET (NULL, cuDeviceGetAttribute,
		  &pi, CU_DEVICE_ATTRIBUTE_WARP_SIZE, dev);
  if (pi != 32)
    {
      GOMP_PLUGIN_error ("Only warp size 32 is supported");
      return NULL;
    }

  r = CUDA_CALL_NOCHECK (cuDeviceGetAttribute, &async_engines,
			 CU_DEVICE_ATTRIBUTE_ASYNC_ENGINE_COUNT, dev);
  if (r != CUDA_SUCCESS)
    async_engines = 1;

  ptx_dev->images = NULL;
  pthread_mutex_init (&ptx_dev->image_lock, NULL);

  if (!init_streams_for_device (ptx_dev, async_engines))
    return NULL;

  return ptx_dev;
}

static bool
nvptx_close_device (struct ptx_device *ptx_dev)
{
  if (!ptx_dev)
    return true;

  if (!fini_streams_for_device (ptx_dev))
    return false;
  
  pthread_mutex_destroy (&ptx_dev->image_lock);

  if (!ptx_dev->ctx_shared)
    CUDA_CALL (cuCtxDestroy, ptx_dev->ctx);

  free (ptx_dev);
  return true;
}

static int
nvptx_get_num_devices (void)
{
  int n;

  /* PR libgomp/65099: Currently, we only support offloading in 64-bit
     configurations.  */
  if (sizeof (void *) != 8)
    {
      GOMP_PLUGIN_debug (0, "Disabling nvptx offloading;"
			 " only 64-bit configurations are supported\n");
      return 0;
    }

  /* This function will be called before the plugin has been initialized in
     order to enumerate available devices, but CUDA API routines can't be used
     until cuInit has been called.  Just call it now (but don't yet do any
     further initialization).  */
  if (instantiated_devices == 0)
    {
      if (!init_cuda_lib ())
	return 0;
      CUresult r = CUDA_CALL_NOCHECK (cuInit, 0);
      /* This is not an error: e.g. we may have CUDA libraries installed but
         no devices available.  */
      if (r != CUDA_SUCCESS)
	{
	  GOMP_PLUGIN_debug (0, "Disabling nvptx offloading; cuInit: %s\n",
			     cuda_error (r));
	  return 0;
	}
    }

  CUDA_CALL_ERET (-1, cuDeviceGetCount, &n);
  return n;
}

static void
notify_var (const char *var_name, const char *env_var)
{
  if (env_var == NULL)
    GOMP_PLUGIN_debug (0, "%s: <Not defined>\n", var_name);
  else
    GOMP_PLUGIN_debug (0, "%s: '%s'\n", var_name, env_var);
}

static bool
link_ptx (CUmodule *module, const struct targ_ptx_obj *ptx_objs,
	  unsigned num_objs)
{
  CUjit_option opts[6];
  void *optvals[6];
  float elapsed = 0.0;
  char elog[1024];
  char ilog[16384];
  CUlinkState linkstate;
  CUresult r;
  void *linkout;
  size_t linkoutsize __attribute__ ((unused));

  opts[0] = CU_JIT_WALL_TIME;
  optvals[0] = &elapsed;

  opts[1] = CU_JIT_INFO_LOG_BUFFER;
  optvals[1] = &ilog[0];

  opts[2] = CU_JIT_INFO_LOG_BUFFER_SIZE_BYTES;
  optvals[2] = (void *) sizeof ilog;

  opts[3] = CU_JIT_ERROR_LOG_BUFFER;
  optvals[3] = &elog[0];

  opts[4] = CU_JIT_ERROR_LOG_BUFFER_SIZE_BYTES;
  optvals[4] = (void *) sizeof elog;

  opts[5] = CU_JIT_LOG_VERBOSE;
  optvals[5] = (void *) 1;

  CUDA_CALL (cuLinkCreate, 6, opts, optvals, &linkstate);

  for (; num_objs--; ptx_objs++)
    {
      /* cuLinkAddData's 'data' argument erroneously omits the const
	 qualifier.  */
      GOMP_PLUGIN_debug (0, "Loading:\n---\n%s\n---\n", ptx_objs->code);
      r = CUDA_CALL_NOCHECK (cuLinkAddData, linkstate, CU_JIT_INPUT_PTX,
			     (char *) ptx_objs->code, ptx_objs->size,
			     0, 0, 0, 0);
      if (r != CUDA_SUCCESS)
	{
	  GOMP_PLUGIN_error ("Link error log %s\n", &elog[0]);
	  GOMP_PLUGIN_error ("cuLinkAddData (ptx_code) error: %s",
			     cuda_error (r));
	  return false;
	}
    }

  GOMP_PLUGIN_debug (0, "Linking\n");
  r = CUDA_CALL_NOCHECK (cuLinkComplete, linkstate, &linkout, &linkoutsize);

  GOMP_PLUGIN_debug (0, "Link complete: %fms\n", elapsed);
  GOMP_PLUGIN_debug (0, "Link log %s\n", &ilog[0]);

  if (r != CUDA_SUCCESS)
    {
      GOMP_PLUGIN_error ("cuLinkComplete error: %s", cuda_error (r));
      return false;
    }

  CUDA_CALL (cuModuleLoadData, module, linkout);
  CUDA_CALL (cuLinkDestroy, linkstate);
  return true;
}

static void
event_gc (bool memmap_lockable)
{
  struct ptx_event *ptx_event = ptx_events;
  struct ptx_event *async_cleanups = NULL;
  struct nvptx_thread *nvthd = nvptx_thread ();

  pthread_mutex_lock (&ptx_event_lock);

  while (ptx_event != NULL)
    {
      CUresult r;
      struct ptx_event *e = ptx_event;

      ptx_event = ptx_event->next;

      if (e->ord != nvthd->ptx_dev->ord)
	continue;

      r = CUDA_CALL_NOCHECK (cuEventQuery, *e->evt);
      if (r == CUDA_SUCCESS)
	{
	  bool append_async = false;
	  CUevent *te;

	  te = e->evt;

	  switch (e->type)
	    {
	    case PTX_EVT_MEM:
	    case PTX_EVT_SYNC:
	      break;

	    case PTX_EVT_KNL:
	      map_pop (e->addr);
	      break;

	    case PTX_EVT_ASYNC_CLEANUP:
	      {
		/* The function gomp_plugin_async_unmap_vars needs to claim the
		   memory-map splay tree lock for the current device, so we
		   can't call it when one of our callers has already claimed
		   the lock.  In that case, just delay the GC for this event
		   until later.  */
		if (!memmap_lockable)
		  continue;

		append_async = true;
	      }
	      break;
	    }

	  CUDA_CALL_NOCHECK (cuEventDestroy, *te);
	  free ((void *)te);

	  /* Unlink 'e' from ptx_events list.  */
	  if (ptx_events == e)
	    ptx_events = ptx_events->next;
	  else
	    {
	      struct ptx_event *e_ = ptx_events;
	      while (e_->next != e)
		e_ = e_->next;
	      e_->next = e_->next->next;
	    }

	  if (append_async)
	    {
	      e->next = async_cleanups;
	      async_cleanups = e;
	    }
	  else
	    free (e);
	}
    }

  pthread_mutex_unlock (&ptx_event_lock);

  /* We have to do these here, after ptx_event_lock is released.  */
  while (async_cleanups)
    {
      struct ptx_event *e = async_cleanups;
      async_cleanups = async_cleanups->next;

      GOMP_PLUGIN_async_unmap_vars (e->addr, e->val);
      free (e);
    }
}

static void
event_add (enum ptx_event_type type, CUevent *e, void *h, int val)
{
  struct ptx_event *ptx_event;
  struct nvptx_thread *nvthd = nvptx_thread ();

  assert (type == PTX_EVT_MEM || type == PTX_EVT_KNL || type == PTX_EVT_SYNC
	  || type == PTX_EVT_ASYNC_CLEANUP);

  ptx_event = GOMP_PLUGIN_malloc (sizeof (struct ptx_event));
  ptx_event->type = type;
  ptx_event->evt = e;
  ptx_event->addr = h;
  ptx_event->ord = nvthd->ptx_dev->ord;
  ptx_event->val = val;

  pthread_mutex_lock (&ptx_event_lock);

  ptx_event->next = ptx_events;
  ptx_events = ptx_event;

  pthread_mutex_unlock (&ptx_event_lock);
}

static void
nvptx_exec (void (*fn), size_t mapnum, void **hostaddrs, void **devaddrs,
	    int async, unsigned *dims, void *targ_mem_desc)
{
  struct targ_fn_descriptor *targ_fn = (struct targ_fn_descriptor *) fn;
  CUfunction function;
  CUresult r;
  int i;
  struct ptx_stream *dev_str;
  void *kargs[1];
  void *hp, *dp;
  struct nvptx_thread *nvthd = nvptx_thread ();
  const char *maybe_abort_msg = "(perhaps abort was called)";

  function = targ_fn->fn;

  dev_str = select_stream_for_async (async, pthread_self (), false, NULL);
  assert (dev_str == nvthd->current_stream);

  /* Initialize the launch dimensions.  Typically this is constant,
     provided by the device compiler, but we must permit runtime
     values.  */
  int seen_zero = 0;
  for (i = 0; i != GOMP_DIM_MAX; i++)
    {
      if (targ_fn->launch->dim[i])
       dims[i] = targ_fn->launch->dim[i];
      if (!dims[i])
       seen_zero = 1;
    }

  if (seen_zero)
    {
      /* See if the user provided GOMP_OPENACC_DIM environment
	 variable to specify runtime defaults. */
      static int default_dims[GOMP_DIM_MAX];

      pthread_mutex_lock (&ptx_dev_lock);
      if (!default_dims[0])
	{
	  const char *var_name = "GOMP_OPENACC_DIM";
	  /* We only read the environment variable once.  You can't
	     change it in the middle of execution.  The syntax  is
	     the same as for the -fopenacc-dim compilation option.  */
	  const char *env_var = getenv (var_name);
	  notify_var (var_name, env_var);
	  if (env_var)
	    {
	      const char *pos = env_var;

	      for (i = 0; *pos && i != GOMP_DIM_MAX; i++)
		{
		  if (i && *pos++ != ':')
		    break;
		  if (*pos != ':')
		    {
		      const char *eptr;

		      errno = 0;
		      long val = strtol (pos, (char **)&eptr, 10);
		      if (errno || val < 0 || (unsigned)val != val)
			break;
		      default_dims[i] = (int)val;
		      pos = eptr;
		    }
		}
	    }

	  int warp_size, block_size, dev_size, cpu_size;
	  CUdevice dev = nvptx_thread()->ptx_dev->dev;
	  /* 32 is the default for known hardware.  */
	  int gang = 0, worker = 32, vector = 32;
	  CUdevice_attribute cu_tpb, cu_ws, cu_mpc, cu_tpm;

	  cu_tpb = CU_DEVICE_ATTRIBUTE_MAX_THREADS_PER_BLOCK;
	  cu_ws = CU_DEVICE_ATTRIBUTE_WARP_SIZE;
	  cu_mpc = CU_DEVICE_ATTRIBUTE_MULTIPROCESSOR_COUNT;
	  cu_tpm  = CU_DEVICE_ATTRIBUTE_MAX_THREADS_PER_MULTIPROCESSOR;

	  if (CUDA_CALL_NOCHECK (cuDeviceGetAttribute, &block_size, cu_tpb,
				 dev) == CUDA_SUCCESS
	      && CUDA_CALL_NOCHECK (cuDeviceGetAttribute, &warp_size, cu_ws,
				    dev) == CUDA_SUCCESS
	      && CUDA_CALL_NOCHECK (cuDeviceGetAttribute, &dev_size, cu_mpc,
				    dev) == CUDA_SUCCESS
	      && CUDA_CALL_NOCHECK (cuDeviceGetAttribute, &cpu_size, cu_tpm,
				    dev) == CUDA_SUCCESS)
	    {
	      GOMP_PLUGIN_debug (0, " warp_size=%d, block_size=%d,"
				 " dev_size=%d, cpu_size=%d\n",
				 warp_size, block_size, dev_size, cpu_size);
	      gang = (cpu_size / block_size) * dev_size;
	      worker = block_size / warp_size;
	      vector = warp_size;
	    }

	  /* There is no upper bound on the gang size.  The best size
	     matches the hardware configuration.  Logical gangs are
	     scheduled onto physical hardware.  To maximize usage, we
	     should guess a large number.  */
	  if (default_dims[GOMP_DIM_GANG] < 1)
	    default_dims[GOMP_DIM_GANG] = gang ? gang : 1024;
	  /* The worker size must not exceed the hardware.  */
	  if (default_dims[GOMP_DIM_WORKER] < 1
	      || (default_dims[GOMP_DIM_WORKER] > worker && gang))
	    default_dims[GOMP_DIM_WORKER] = worker;
	  /* The vector size must exactly match the hardware.  */
	  if (default_dims[GOMP_DIM_VECTOR] < 1
	      || (default_dims[GOMP_DIM_VECTOR] != vector && gang))
	    default_dims[GOMP_DIM_VECTOR] = vector;

	  GOMP_PLUGIN_debug (0, " default dimensions [%d,%d,%d]\n",
			     default_dims[GOMP_DIM_GANG],
			     default_dims[GOMP_DIM_WORKER],
			     default_dims[GOMP_DIM_VECTOR]);
	}
      pthread_mutex_unlock (&ptx_dev_lock);

      for (i = 0; i != GOMP_DIM_MAX; i++)
	if (!dims[i])
	  dims[i] = default_dims[i];
    }

  /* This reserves a chunk of a pre-allocated page of memory mapped on both
     the host and the device. HP is a host pointer to the new chunk, and DP is
     the corresponding device pointer.  */
  map_push (dev_str, async, mapnum * sizeof (void *), &hp, &dp);

  GOMP_PLUGIN_debug (0, "  %s: prepare mappings\n", __FUNCTION__);

  /* Copy the array of arguments to the mapped page.  */
  for (i = 0; i < mapnum; i++)
    ((void **) hp)[i] = devaddrs[i];

  /* Copy the (device) pointers to arguments to the device (dp and hp might in
     fact have the same value on a unified-memory system).  */
  CUDA_CALL_ASSERT (cuMemcpy, (CUdeviceptr) dp, (CUdeviceptr) hp,
		    mapnum * sizeof (void *));
  GOMP_PLUGIN_debug (0, "  %s: kernel %s: launch"
		     " gangs=%u, workers=%u, vectors=%u\n",
		     __FUNCTION__, targ_fn->launch->fn, dims[GOMP_DIM_GANG],
		     dims[GOMP_DIM_WORKER], dims[GOMP_DIM_VECTOR]);

  // OpenACC		CUDA
  //
  // num_gangs		nctaid.x
  // num_workers	ntid.y
  // vector length	ntid.x

  kargs[0] = &dp;
  CUDA_CALL_ASSERT (cuLaunchKernel, function,
		    dims[GOMP_DIM_GANG], 1, 1,
		    dims[GOMP_DIM_VECTOR], dims[GOMP_DIM_WORKER], 1,
		    0, dev_str->stream, kargs, 0);

#ifndef DISABLE_ASYNC
  if (async < acc_async_noval)
    {
      r = CUDA_CALL_NOCHECK (cuStreamSynchronize, dev_str->stream);
      if (r == CUDA_ERROR_LAUNCH_FAILED)
	GOMP_PLUGIN_fatal ("cuStreamSynchronize error: %s %s\n", cuda_error (r),
			   maybe_abort_msg);
      else if (r != CUDA_SUCCESS)
        GOMP_PLUGIN_fatal ("cuStreamSynchronize error: %s", cuda_error (r));
    }
  else
    {
      CUevent *e;

      e = (CUevent *)GOMP_PLUGIN_malloc (sizeof (CUevent));

      r = CUDA_CALL_NOCHECK (cuEventCreate, e, CU_EVENT_DISABLE_TIMING);
      if (r == CUDA_ERROR_LAUNCH_FAILED)
	GOMP_PLUGIN_fatal ("cuEventCreate error: %s %s\n", cuda_error (r),
			   maybe_abort_msg);
      else if (r != CUDA_SUCCESS)
        GOMP_PLUGIN_fatal ("cuEventCreate error: %s", cuda_error (r));

      event_gc (true);

      CUDA_CALL_ASSERT (cuEventRecord, *e, dev_str->stream);

      event_add (PTX_EVT_KNL, e, (void *)dev_str, 0);
    }
#else
  r = CUDA_CALL_NOCHECK (cuCtxSynchronize, );
  if (r == CUDA_ERROR_LAUNCH_FAILED)
    GOMP_PLUGIN_fatal ("cuCtxSynchronize error: %s %s\n", cuda_error (r),
		       maybe_abort_msg);
  else if (r != CUDA_SUCCESS)
    GOMP_PLUGIN_fatal ("cuCtxSynchronize error: %s", cuda_error (r));
#endif

  GOMP_PLUGIN_debug (0, "  %s: kernel %s: finished\n", __FUNCTION__,
		     targ_fn->launch->fn);

#ifndef DISABLE_ASYNC
  if (async < acc_async_noval)
#endif
    map_pop (dev_str);
}

void * openacc_get_current_cuda_context (void);

static void *
nvptx_alloc (size_t s)
{
  CUdeviceptr d;

  CUDA_CALL_ERET (NULL, cuMemAlloc, &d, s);
  return (void *) d;
}

static bool
nvptx_free (void *p)
{
  CUdeviceptr pb;
  size_t ps;

  CUDA_CALL (cuMemGetAddressRange, &pb, &ps, (CUdeviceptr) p);
  if ((CUdeviceptr) p != pb)
    {
      GOMP_PLUGIN_error ("invalid device address");
      return false;
    }

  CUDA_CALL (cuMemFree, (CUdeviceptr) p);
  return true;
}


static bool
nvptx_host2dev (void *d, const void *h, size_t s)
{
  CUdeviceptr pb;
  size_t ps;
  struct nvptx_thread *nvthd = nvptx_thread ();

  if (!s)
    return true;
  if (!d)
    {
      GOMP_PLUGIN_error ("invalid device address");
      return false;
    }

  CUDA_CALL (cuMemGetAddressRange, &pb, &ps, (CUdeviceptr) d);

  if (!pb)
    {
      GOMP_PLUGIN_error ("invalid device address");
      return false;
    }
  if (!h)
    {
      GOMP_PLUGIN_error ("invalid host address");
      return false;
    }
  if (d == h)
    {
      GOMP_PLUGIN_error ("invalid host or device address");
      return false;
    }
  if ((void *)(d + s) > (void *)(pb + ps))
    {
      GOMP_PLUGIN_error ("invalid size");
      return false;
    }

#ifndef DISABLE_ASYNC
  if (nvthd && nvthd->current_stream != nvthd->ptx_dev->null_stream)
    {
      CUevent *e = (CUevent *)GOMP_PLUGIN_malloc (sizeof (CUevent));
      CUDA_CALL (cuEventCreate, e, CU_EVENT_DISABLE_TIMING);
      event_gc (false);
      CUDA_CALL (cuMemcpyHtoDAsync,
		 (CUdeviceptr) d, h, s, nvthd->current_stream->stream);
      CUDA_CALL (cuEventRecord, *e, nvthd->current_stream->stream);
      event_add (PTX_EVT_MEM, e, (void *)h, 0);
    }
  else
#endif
    CUDA_CALL (cuMemcpyHtoD, (CUdeviceptr) d, h, s);

  return true;
}

static bool
nvptx_dev2host (void *h, const void *d, size_t s)
{
  CUdeviceptr pb;
  size_t ps;
  struct nvptx_thread *nvthd = nvptx_thread ();

  if (!s)
    return true;
  if (!d)
    {
      GOMP_PLUGIN_error ("invalid device address");
      return false;
    }

  CUDA_CALL (cuMemGetAddressRange, &pb, &ps, (CUdeviceptr) d);

  if (!pb)
    {
      GOMP_PLUGIN_error ("invalid device address");
      return false;
    }
  if (!h)
    {
      GOMP_PLUGIN_error ("invalid host address");
      return false;
    }
  if (d == h)
    {
      GOMP_PLUGIN_error ("invalid host or device address");
      return false;
    }
  if ((void *)(d + s) > (void *)(pb + ps))
    {
      GOMP_PLUGIN_error ("invalid size");
      return false;
    }

#ifndef DISABLE_ASYNC
  if (nvthd && nvthd->current_stream != nvthd->ptx_dev->null_stream)
    {
      CUevent *e = (CUevent *) GOMP_PLUGIN_malloc (sizeof (CUevent));
      CUDA_CALL (cuEventCreate, e, CU_EVENT_DISABLE_TIMING);
      event_gc (false);
      CUDA_CALL (cuMemcpyDtoHAsync,
		 h, (CUdeviceptr) d, s, nvthd->current_stream->stream);
      CUDA_CALL (cuEventRecord, *e, nvthd->current_stream->stream);
      event_add (PTX_EVT_MEM, e, (void *)h, 0);
    }
  else
#endif
    CUDA_CALL (cuMemcpyDtoH, h, (CUdeviceptr) d, s);

  return true;
}

static void
nvptx_set_async (int async)
{
  struct nvptx_thread *nvthd = nvptx_thread ();
  nvthd->current_stream
    = select_stream_for_async (async, pthread_self (), true, NULL);
}

static int
nvptx_async_test (int async)
{
  CUresult r;
  struct ptx_stream *s;

  s = select_stream_for_async (async, pthread_self (), false, NULL);

  if (!s)
    GOMP_PLUGIN_fatal ("unknown async %d", async);

  r = CUDA_CALL_NOCHECK (cuStreamQuery, s->stream);
  if (r == CUDA_SUCCESS)
    {
      /* The oacc-parallel.c:goacc_wait function calls this hook to determine
	 whether all work has completed on this stream, and if so omits the call
	 to the wait hook.  If that happens, event_gc might not get called
	 (which prevents variables from getting unmapped and their associated
	 device storage freed), so call it here.  */
      event_gc (true);
      return 1;
    }
  else if (r == CUDA_ERROR_NOT_READY)
    return 0;

  GOMP_PLUGIN_fatal ("cuStreamQuery error: %s", cuda_error (r));

  return 0;
}

static int
nvptx_async_test_all (void)
{
  struct ptx_stream *s;
  pthread_t self = pthread_self ();
  struct nvptx_thread *nvthd = nvptx_thread ();

  pthread_mutex_lock (&nvthd->ptx_dev->stream_lock);

  for (s = nvthd->ptx_dev->active_streams; s != NULL; s = s->next)
    {
      if ((s->multithreaded || pthread_equal (s->host_thread, self))
	  && CUDA_CALL_NOCHECK (cuStreamQuery,
				s->stream) == CUDA_ERROR_NOT_READY)
	{
	  pthread_mutex_unlock (&nvthd->ptx_dev->stream_lock);
	  return 0;
	}
    }

  pthread_mutex_unlock (&nvthd->ptx_dev->stream_lock);

  event_gc (true);

  return 1;
}

static void
nvptx_wait (int async)
{
  struct ptx_stream *s;

  s = select_stream_for_async (async, pthread_self (), false, NULL);
  if (!s)
    GOMP_PLUGIN_fatal ("unknown async %d", async);

  CUDA_CALL_ASSERT (cuStreamSynchronize, s->stream);

  event_gc (true);
}

static void
nvptx_wait_async (int async1, int async2)
{
  CUevent *e;
  struct ptx_stream *s1, *s2;
  pthread_t self = pthread_self ();

  /* The stream that is waiting (rather than being waited for) doesn't
     necessarily have to exist already.  */
  s2 = select_stream_for_async (async2, self, true, NULL);

  s1 = select_stream_for_async (async1, self, false, NULL);
  if (!s1)
    GOMP_PLUGIN_fatal ("invalid async 1\n");

  if (s1 == s2)
    GOMP_PLUGIN_fatal ("identical parameters");

  e = (CUevent *) GOMP_PLUGIN_malloc (sizeof (CUevent));

  CUDA_CALL_ASSERT (cuEventCreate, e, CU_EVENT_DISABLE_TIMING);

  event_gc (true);

  CUDA_CALL_ASSERT (cuEventRecord, *e, s1->stream);

  event_add (PTX_EVT_SYNC, e, NULL, 0);

  CUDA_CALL_ASSERT (cuStreamWaitEvent, s2->stream, *e, 0);
}

static void
nvptx_wait_all (void)
{
  CUresult r;
  struct ptx_stream *s;
  pthread_t self = pthread_self ();
  struct nvptx_thread *nvthd = nvptx_thread ();

  pthread_mutex_lock (&nvthd->ptx_dev->stream_lock);

  /* Wait for active streams initiated by this thread (or by multiple threads)
     to complete.  */
  for (s = nvthd->ptx_dev->active_streams; s != NULL; s = s->next)
    {
      if (s->multithreaded || pthread_equal (s->host_thread, self))
	{
	  r = CUDA_CALL_NOCHECK (cuStreamQuery, s->stream);
	  if (r == CUDA_SUCCESS)
	    continue;
	  else if (r != CUDA_ERROR_NOT_READY)
	    GOMP_PLUGIN_fatal ("cuStreamQuery error: %s", cuda_error (r));

	  CUDA_CALL_ASSERT (cuStreamSynchronize, s->stream);
	}
    }

  pthread_mutex_unlock (&nvthd->ptx_dev->stream_lock);

  event_gc (true);
}

static void
nvptx_wait_all_async (int async)
{
  struct ptx_stream *waiting_stream, *other_stream;
  CUevent *e;
  struct nvptx_thread *nvthd = nvptx_thread ();
  pthread_t self = pthread_self ();

  /* The stream doing the waiting.  This could be the first mention of the
     stream, so create it if necessary.  */
  waiting_stream
    = select_stream_for_async (async, pthread_self (), true, NULL);

  /* Launches on the null stream already block on other streams in the
     context.  */
  if (!waiting_stream || waiting_stream == nvthd->ptx_dev->null_stream)
    return;

  event_gc (true);

  pthread_mutex_lock (&nvthd->ptx_dev->stream_lock);

  for (other_stream = nvthd->ptx_dev->active_streams;
       other_stream != NULL;
       other_stream = other_stream->next)
    {
      if (!other_stream->multithreaded
	  && !pthread_equal (other_stream->host_thread, self))
	continue;

      e = (CUevent *) GOMP_PLUGIN_malloc (sizeof (CUevent));

      CUDA_CALL_ASSERT (cuEventCreate, e, CU_EVENT_DISABLE_TIMING);

      /* Record an event on the waited-for stream.  */
      CUDA_CALL_ASSERT (cuEventRecord, *e, other_stream->stream);

      event_add (PTX_EVT_SYNC, e, NULL, 0);

      CUDA_CALL_ASSERT (cuStreamWaitEvent, waiting_stream->stream, *e, 0);
   }

  pthread_mutex_unlock (&nvthd->ptx_dev->stream_lock);
}

static void *
nvptx_get_current_cuda_device (void)
{
  struct nvptx_thread *nvthd = nvptx_thread ();

  if (!nvthd || !nvthd->ptx_dev)
    return NULL;

  return &nvthd->ptx_dev->dev;
}

static void *
nvptx_get_current_cuda_context (void)
{
  struct nvptx_thread *nvthd = nvptx_thread ();

  if (!nvthd || !nvthd->ptx_dev)
    return NULL;

  return nvthd->ptx_dev->ctx;
}

static void *
nvptx_get_cuda_stream (int async)
{
  struct ptx_stream *s;
  struct nvptx_thread *nvthd = nvptx_thread ();

  if (!nvthd || !nvthd->ptx_dev)
    return NULL;

  s = select_stream_for_async (async, pthread_self (), false, NULL);

  return s ? s->stream : NULL;
}

static int
nvptx_set_cuda_stream (int async, void *stream)
{
  struct ptx_stream *oldstream;
  pthread_t self = pthread_self ();
  struct nvptx_thread *nvthd = nvptx_thread ();

  if (async < 0)
    GOMP_PLUGIN_fatal ("bad async %d", async);

  pthread_mutex_lock (&nvthd->ptx_dev->stream_lock);

  /* We have a list of active streams and an array mapping async values to
     entries of that list.  We need to take "ownership" of the passed-in stream,
     and add it to our list, removing the previous entry also (if there was one)
     in order to prevent resource leaks.  Note the potential for surprise
     here: maybe we should keep track of passed-in streams and leave it up to
     the user to tidy those up, but that doesn't work for stream handles
     returned from acc_get_cuda_stream above...  */

  oldstream = select_stream_for_async (async, self, false, NULL);

  if (oldstream)
    {
      if (nvthd->ptx_dev->active_streams == oldstream)
	nvthd->ptx_dev->active_streams = nvthd->ptx_dev->active_streams->next;
      else
	{
	  struct ptx_stream *s = nvthd->ptx_dev->active_streams;
	  while (s->next != oldstream)
	    s = s->next;
	  s->next = s->next->next;
	}

      CUDA_CALL_ASSERT (cuStreamDestroy, oldstream->stream);

      if (!map_fini (oldstream))
	GOMP_PLUGIN_fatal ("error when freeing host memory");

      free (oldstream);
    }

  pthread_mutex_unlock (&nvthd->ptx_dev->stream_lock);

  (void) select_stream_for_async (async, self, true, (CUstream) stream);

  return 1;
}

/* Plugin entry points.  */

const char *
GOMP_OFFLOAD_get_name (void)
{
  return "nvptx";
}

unsigned int
GOMP_OFFLOAD_get_caps (void)
{
  return GOMP_OFFLOAD_CAP_OPENACC_200 | GOMP_OFFLOAD_CAP_OPENMP_400;
}

int
GOMP_OFFLOAD_get_type (void)
{
  return OFFLOAD_TARGET_TYPE_NVIDIA_PTX;
}

int
GOMP_OFFLOAD_get_num_devices (void)
{
  return nvptx_get_num_devices ();
}

bool
GOMP_OFFLOAD_init_device (int n)
{
  struct ptx_device *dev;

  pthread_mutex_lock (&ptx_dev_lock);

  if (!nvptx_init () || ptx_devices[n] != NULL)
    {
      pthread_mutex_unlock (&ptx_dev_lock);
      return false;
    }

  dev = nvptx_open_device (n);
  if (dev)
    {
      ptx_devices[n] = dev;
      instantiated_devices++;
    }

  pthread_mutex_unlock (&ptx_dev_lock);

  return dev != NULL;
}

bool
GOMP_OFFLOAD_fini_device (int n)
{
  pthread_mutex_lock (&ptx_dev_lock);

  if (ptx_devices[n] != NULL)
    {
      if (!nvptx_attach_host_thread_to_device (n)
	  || !nvptx_close_device (ptx_devices[n]))
	{
	  pthread_mutex_unlock (&ptx_dev_lock);
	  return false;
	}
      ptx_devices[n] = NULL;
      instantiated_devices--;
    }

  pthread_mutex_unlock (&ptx_dev_lock);
  return true;
}

/* Return the libgomp version number we're compatible with.  There is
   no requirement for cross-version compatibility.  */

unsigned
GOMP_OFFLOAD_version (void)
{
  return GOMP_VERSION;
}

/* Initialize __nvptx_clocktick, if present in MODULE.  */

static void
nvptx_set_clocktick (CUmodule module, struct ptx_device *dev)
{
  CUdeviceptr dptr;
  CUresult r = CUDA_CALL_NOCHECK (cuModuleGetGlobal, &dptr, NULL,
				  module, "__nvptx_clocktick");
  if (r == CUDA_ERROR_NOT_FOUND)
    return;
  if (r != CUDA_SUCCESS)
    GOMP_PLUGIN_fatal ("cuModuleGetGlobal error: %s", cuda_error (r));
  double __nvptx_clocktick = 1e-3 / dev->clock_khz;
  r = CUDA_CALL_NOCHECK (cuMemcpyHtoD, dptr, &__nvptx_clocktick,
			 sizeof (__nvptx_clocktick));
  if (r != CUDA_SUCCESS)
    GOMP_PLUGIN_fatal ("cuMemcpyHtoD error: %s", cuda_error (r));
}

/* Load the (partial) program described by TARGET_DATA to device
   number ORD.  Allocate and return TARGET_TABLE.  */

int
GOMP_OFFLOAD_load_image (int ord, unsigned version, const void *target_data,
			 struct addr_pair **target_table)
{
  CUmodule module;
  const char *const *var_names;
  const struct targ_fn_launch *fn_descs;
  unsigned int fn_entries, var_entries, i, j;
  struct targ_fn_descriptor *targ_fns;
  struct addr_pair *targ_tbl;
  const nvptx_tdata_t *img_header = (const nvptx_tdata_t *) target_data;
  struct ptx_image_data *new_image;
  struct ptx_device *dev;

  if (GOMP_VERSION_DEV (version) > GOMP_VERSION_NVIDIA_PTX)
    {
      GOMP_PLUGIN_error ("Offload data incompatible with PTX plugin"
			 " (expected %u, received %u)",
			 GOMP_VERSION_NVIDIA_PTX, GOMP_VERSION_DEV (version));
      return -1;
    }

  if (!nvptx_attach_host_thread_to_device (ord)
      || !link_ptx (&module, img_header->ptx_objs, img_header->ptx_num))
    return -1;

  dev = ptx_devices[ord];

  /* The mkoffload utility emits a struct of pointers/integers at the
     start of each offload image.  The array of kernel names and the
     functions addresses form a one-to-one correspondence.  */

  var_entries = img_header->var_num;
  var_names = img_header->var_names;
  fn_entries = img_header->fn_num;
  fn_descs = img_header->fn_descs;

  targ_tbl = GOMP_PLUGIN_malloc (sizeof (struct addr_pair)
				 * (fn_entries + var_entries));
  targ_fns = GOMP_PLUGIN_malloc (sizeof (struct targ_fn_descriptor)
				 * fn_entries);

  *target_table = targ_tbl;

  new_image = GOMP_PLUGIN_malloc (sizeof (struct ptx_image_data));
  new_image->target_data = target_data;
  new_image->module = module;
  new_image->fns = targ_fns;

  pthread_mutex_lock (&dev->image_lock);
  new_image->next = dev->images;
  dev->images = new_image;
  pthread_mutex_unlock (&dev->image_lock);

  for (i = 0; i < fn_entries; i++, targ_fns++, targ_tbl++)
    {
      CUfunction function;
      int nregs, mthrs;

      CUDA_CALL_ERET (-1, cuModuleGetFunction, &function, module,
		      fn_descs[i].fn);
      CUDA_CALL_ERET (-1, cuFuncGetAttribute, &nregs,
		      CU_FUNC_ATTRIBUTE_NUM_REGS, function);
      CUDA_CALL_ERET (-1, cuFuncGetAttribute, &mthrs,
		      CU_FUNC_ATTRIBUTE_MAX_THREADS_PER_BLOCK, function);

      targ_fns->fn = function;
      targ_fns->launch = &fn_descs[i];
      targ_fns->regs_per_thread = nregs;
      targ_fns->max_threads_per_block = mthrs;

      targ_tbl->start = (uintptr_t) targ_fns;
      targ_tbl->end = targ_tbl->start + 1;
    }

  for (j = 0; j < var_entries; j++, targ_tbl++)
    {
      CUdeviceptr var;
      size_t bytes;

      CUDA_CALL_ERET (-1, cuModuleGetGlobal,
		      &var, &bytes, module, var_names[j]);

      targ_tbl->start = (uintptr_t) var;
      targ_tbl->end = targ_tbl->start + bytes;
    }

  nvptx_set_clocktick (module, dev);

  return fn_entries + var_entries;
}

/* Unload the program described by TARGET_DATA.  DEV_DATA is the
   function descriptors allocated by G_O_load_image.  */

bool
GOMP_OFFLOAD_unload_image (int ord, unsigned version, const void *target_data)
{
  struct ptx_image_data *image, **prev_p;
  struct ptx_device *dev = ptx_devices[ord];

  if (GOMP_VERSION_DEV (version) > GOMP_VERSION_NVIDIA_PTX)
    {
      GOMP_PLUGIN_error ("Offload data incompatible with PTX plugin"
			 " (expected %u, received %u)",
			 GOMP_VERSION_NVIDIA_PTX, GOMP_VERSION_DEV (version));
      return false;
    }

  bool ret = true;
  pthread_mutex_lock (&dev->image_lock);
  for (prev_p = &dev->images; (image = *prev_p) != 0; prev_p = &image->next)
    if (image->target_data == target_data)
      {
	*prev_p = image->next;
	if (CUDA_CALL_NOCHECK (cuModuleUnload, image->module) != CUDA_SUCCESS)
	  ret = false;
	free (image->fns);
	free (image);
	break;
      }
  pthread_mutex_unlock (&dev->image_lock);
  return ret;
}

void *
GOMP_OFFLOAD_alloc (int ord, size_t size)
{
  if (!nvptx_attach_host_thread_to_device (ord))
    return NULL;
  return nvptx_alloc (size);
}

bool
GOMP_OFFLOAD_free (int ord, void *ptr)
{
  return (nvptx_attach_host_thread_to_device (ord)
	  && nvptx_free (ptr));
}

bool
GOMP_OFFLOAD_dev2host (int ord, void *dst, const void *src, size_t n)
{
  return (nvptx_attach_host_thread_to_device (ord)
	  && nvptx_dev2host (dst, src, n));
}

bool
GOMP_OFFLOAD_host2dev (int ord, void *dst, const void *src, size_t n)
{
  return (nvptx_attach_host_thread_to_device (ord)
	  && nvptx_host2dev (dst, src, n));
}

bool
GOMP_OFFLOAD_dev2dev (int ord, void *dst, const void *src, size_t n)
{
  struct ptx_device *ptx_dev = ptx_devices[ord];
  CUDA_CALL (cuMemcpyDtoDAsync, (CUdeviceptr) dst, (CUdeviceptr) src, n,
				ptx_dev->null_stream->stream);
  return true;
}

void (*device_run) (int n, void *fn_ptr, void *vars) = NULL;

void
GOMP_OFFLOAD_openacc_exec (void (*fn) (void *), size_t mapnum,
			   void **hostaddrs, void **devaddrs,
			   int async, unsigned *dims, void *targ_mem_desc)
{
  nvptx_exec (fn, mapnum, hostaddrs, devaddrs, async, dims, targ_mem_desc);
}

void
GOMP_OFFLOAD_openacc_register_async_cleanup (void *targ_mem_desc, int async)
{
  struct nvptx_thread *nvthd = nvptx_thread ();
  CUevent *e = (CUevent *) GOMP_PLUGIN_malloc (sizeof (CUevent));

  CUDA_CALL_ASSERT (cuEventCreate, e, CU_EVENT_DISABLE_TIMING);
  CUDA_CALL_ASSERT (cuEventRecord, *e, nvthd->current_stream->stream);
  event_add (PTX_EVT_ASYNC_CLEANUP, e, targ_mem_desc, async);
}

int
GOMP_OFFLOAD_openacc_async_test (int async)
{
  return nvptx_async_test (async);
}

int
GOMP_OFFLOAD_openacc_async_test_all (void)
{
  return nvptx_async_test_all ();
}

void
GOMP_OFFLOAD_openacc_async_wait (int async)
{
  nvptx_wait (async);
}

void
GOMP_OFFLOAD_openacc_async_wait_async (int async1, int async2)
{
  nvptx_wait_async (async1, async2);
}

void
GOMP_OFFLOAD_openacc_async_wait_all (void)
{
  nvptx_wait_all ();
}

void
GOMP_OFFLOAD_openacc_async_wait_all_async (int async)
{
  nvptx_wait_all_async (async);
}

void
GOMP_OFFLOAD_openacc_async_set_async (int async)
{
  nvptx_set_async (async);
}

void *
GOMP_OFFLOAD_openacc_create_thread_data (int ord)
{
  struct ptx_device *ptx_dev;
  struct nvptx_thread *nvthd
    = GOMP_PLUGIN_malloc (sizeof (struct nvptx_thread));
  CUcontext thd_ctx;

  ptx_dev = ptx_devices[ord];

  assert (ptx_dev);

  CUDA_CALL_ASSERT (cuCtxGetCurrent, &thd_ctx);

  assert (ptx_dev->ctx);

  if (!thd_ctx)
    CUDA_CALL_ASSERT (cuCtxPushCurrent, ptx_dev->ctx);

  nvthd->current_stream = ptx_dev->null_stream;
  nvthd->ptx_dev = ptx_dev;

  return (void *) nvthd;
}

void
GOMP_OFFLOAD_openacc_destroy_thread_data (void *data)
{
  free (data);
}

void *
GOMP_OFFLOAD_openacc_cuda_get_current_device (void)
{
  return nvptx_get_current_cuda_device ();
}

void *
GOMP_OFFLOAD_openacc_cuda_get_current_context (void)
{
  return nvptx_get_current_cuda_context ();
}

/* NOTE: This returns a CUstream, not a ptx_stream pointer.  */

void *
GOMP_OFFLOAD_openacc_cuda_get_stream (int async)
{
  return nvptx_get_cuda_stream (async);
}

/* NOTE: This takes a CUstream, not a ptx_stream pointer.  */

int
GOMP_OFFLOAD_openacc_cuda_set_stream (int async, void *stream)
{
  return nvptx_set_cuda_stream (async, stream);
}

/* Adjust launch dimensions: pick good values for number of blocks and warps
   and ensure that number of warps does not exceed CUDA limits as well as GCC's
   own limits.  */

static void
nvptx_adjust_launch_bounds (struct targ_fn_descriptor *fn,
			    struct ptx_device *ptx_dev,
			    int *teams_p, int *threads_p)
{
  int max_warps_block = fn->max_threads_per_block / 32;
  /* Maximum 32 warps per block is an implementation limit in NVPTX backend
     and libgcc, which matches documented limit of all GPUs as of 2015.  */
  if (max_warps_block > 32)
    max_warps_block = 32;
  if (*threads_p <= 0)
    *threads_p = 8;
  if (*threads_p > max_warps_block)
    *threads_p = max_warps_block;

  int regs_per_block = fn->regs_per_thread * 32 * *threads_p;
  /* This is an estimate of how many blocks the device can host simultaneously.
     Actual limit, which may be lower, can be queried with "occupancy control"
     driver interface (since CUDA 6.0).  */
  int max_blocks = ptx_dev->regs_per_sm / regs_per_block * ptx_dev->num_sms;
  if (*teams_p <= 0 || *teams_p > max_blocks)
    *teams_p = max_blocks;
}

/* Return the size of per-warp stacks (see gcc -msoft-stack) to use for OpenMP
   target regions.  */

static size_t
nvptx_stacks_size ()
{
  return 128 * 1024;
}

/* Return contiguous storage for NUM stacks, each SIZE bytes.  */

static void *
nvptx_stacks_alloc (size_t size, int num)
{
  CUdeviceptr stacks;
  CUresult r = CUDA_CALL_NOCHECK (cuMemAlloc, &stacks, size * num);
  if (r != CUDA_SUCCESS)
    GOMP_PLUGIN_fatal ("cuMemAlloc error: %s", cuda_error (r));
  return (void *) stacks;
}

/* Release storage previously allocated by nvptx_stacks_alloc.  */

static void
nvptx_stacks_free (void *p, int num)
{
  CUresult r = CUDA_CALL_NOCHECK (cuMemFree, (CUdeviceptr) p);
  if (r != CUDA_SUCCESS)
    GOMP_PLUGIN_fatal ("cuMemFree error: %s", cuda_error (r));
}

void
GOMP_OFFLOAD_run (int ord, void *tgt_fn, void *tgt_vars, void **args)
{
  CUfunction function = ((struct targ_fn_descriptor *) tgt_fn)->fn;
  CUresult r;
  struct ptx_device *ptx_dev = ptx_devices[ord];
  const char *maybe_abort_msg = "(perhaps abort was called)";
  int teams = 0, threads = 0;

  if (!args)
    GOMP_PLUGIN_fatal ("No target arguments provided");
  while (*args)
    {
      intptr_t id = (intptr_t) *args++, val;
      if (id & GOMP_TARGET_ARG_SUBSEQUENT_PARAM)
	val = (intptr_t) *args++;
      else
        val = id >> GOMP_TARGET_ARG_VALUE_SHIFT;
      if ((id & GOMP_TARGET_ARG_DEVICE_MASK) != GOMP_TARGET_ARG_DEVICE_ALL)
	continue;
      val = val > INT_MAX ? INT_MAX : val;
      id &= GOMP_TARGET_ARG_ID_MASK;
      if (id == GOMP_TARGET_ARG_NUM_TEAMS)
	teams = val;
      else if (id == GOMP_TARGET_ARG_THREAD_LIMIT)
	threads = val;
    }
  nvptx_adjust_launch_bounds (tgt_fn, ptx_dev, &teams, &threads);

  size_t stack_size = nvptx_stacks_size ();
  void *stacks = nvptx_stacks_alloc (stack_size, teams * threads);
  void *fn_args[] = {tgt_vars, stacks, (void *) stack_size};
  size_t fn_args_size = sizeof fn_args;
  void *config[] = {
    CU_LAUNCH_PARAM_BUFFER_POINTER, fn_args,
    CU_LAUNCH_PARAM_BUFFER_SIZE, &fn_args_size,
    CU_LAUNCH_PARAM_END
  };
  r = CUDA_CALL_NOCHECK (cuLaunchKernel, function, teams, 1, 1,
			 32, threads, 1, 0, ptx_dev->null_stream->stream,
			 NULL, config);
  if (r != CUDA_SUCCESS)
    GOMP_PLUGIN_fatal ("cuLaunchKernel error: %s", cuda_error (r));

  r = CUDA_CALL_NOCHECK (cuCtxSynchronize, );
  if (r == CUDA_ERROR_LAUNCH_FAILED)
    GOMP_PLUGIN_fatal ("cuCtxSynchronize error: %s %s\n", cuda_error (r),
		       maybe_abort_msg);
  else if (r != CUDA_SUCCESS)
    GOMP_PLUGIN_fatal ("cuCtxSynchronize error: %s", cuda_error (r));
  nvptx_stacks_free (stacks, teams * threads);
}

void
GOMP_OFFLOAD_async_run (int ord, void *tgt_fn, void *tgt_vars, void **args,
			void *async_data)
{
  GOMP_PLUGIN_fatal ("GOMP_OFFLOAD_async_run unimplemented");
}
