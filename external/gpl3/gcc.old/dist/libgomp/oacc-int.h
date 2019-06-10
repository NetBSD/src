/* OpenACC Runtime - internal declarations

   Copyright (C) 2013-2016 Free Software Foundation, Inc.

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

/* This file contains data types and function declarations that are not
   part of the official OpenACC user interface.  There are declarations
   in here that are part of the GNU OpenACC ABI, in that the compiler is
   required to know about them and use them.

   The convention is that the all caps prefix "GOACC" is used group items
   that are part of the external ABI, and the lower case prefix "goacc"
   is used group items that are completely private to the library.  */

#ifndef OACC_INT_H
#define OACC_INT_H 1

#include "openacc.h"
#include "config.h"
#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>

#ifdef HAVE_ATTRIBUTE_VISIBILITY
# pragma GCC visibility push(hidden)
#endif

static inline enum acc_device_t
acc_device_type (enum offload_target_type type)
{
  return (enum acc_device_t) type;
}

struct goacc_thread
{
  /* The base device for the current thread.  */
  struct gomp_device_descr *base_dev;

  /* The device for the current thread.  */
  struct gomp_device_descr *dev;

  struct gomp_device_descr *saved_bound_dev;

  /* This is a linked list of data mapped by the "acc data" pragma, following
     strictly push/pop semantics according to lexical scope.  */
  struct target_mem_desc *mapped_data;

  /* These structures form a list: this is the next thread in that list.  */
  struct goacc_thread *next;

  /* Target-specific data (used by plugin).  */
  void *target_tls;
};

#if defined HAVE_TLS || defined USE_EMUTLS
extern __thread struct goacc_thread *goacc_tls_data;
static inline struct goacc_thread *
goacc_thread (void)
{
  return goacc_tls_data;
}
#else
extern pthread_key_t goacc_tls_key;
static inline struct goacc_thread *
goacc_thread (void)
{
  return pthread_getspecific (goacc_tls_key);
}
#endif

void goacc_register (struct gomp_device_descr *) __GOACC_NOTHROW;
void goacc_attach_host_thread_to_device (int);
void goacc_runtime_initialize (void);
void goacc_save_and_set_bind (acc_device_t);
void goacc_restore_bind (void);
void goacc_lazy_initialize (void);
void goacc_host_init (void);

#ifdef HAVE_ATTRIBUTE_VISIBILITY
# pragma GCC visibility pop
#endif

#endif
