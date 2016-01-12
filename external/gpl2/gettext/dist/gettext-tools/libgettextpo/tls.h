/* Thread-local storage in multithreaded situations.
   Copyright (C) 2005 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

/* Written by Bruno Haible <bruno@clisp.org>, 2005.  */

/* This file contains thread-local storage primitives for use with a given
   thread library.  It does not contain primitives for creating threads or
   for other multithreading primitives.

   Type:                      gl_tls_key_t
   Initialization:            gl_tls_key_init (name, destructor);
   Getting per-thread value:  gl_tls_get (name)
   Setting per-thread value:  gl_tls_set (name, pointer);
   De-initialization:         gl_tls_key_destroy (name);

   A per-thread value is of type 'void *'.

   A destructor is a function pointer of type 'void (*) (void *)', called
   when a thread exits, and taking the last per-thread value as argument.  It
   is unspecified whether the destructor function is called when the last
   per-thread value is NULL.  On some platforms, the destructor function is
   not called at all.
*/


#ifndef _TLS_H
#define _TLS_H

/* ========================================================================= */

#if USE_POSIX_THREADS

/* Use the POSIX threads library.  */

# include <pthread.h>
# include <stdlib.h>

# if PTHREAD_IN_USE_DETECTION_HARD

/* The pthread_in_use() detection needs to be done at runtime.  */
#  define pthread_in_use() \
     glthread_in_use ()
extern int glthread_in_use (void);

# endif

# if USE_POSIX_THREADS_WEAK

/* Use weak references to the POSIX threads library.  */

#  pragma weak pthread_key_create
#  pragma weak pthread_getspecific
#  pragma weak pthread_setspecific
#  pragma weak pthread_key_delete
#  ifndef pthread_self
#   pragma weak pthread_self
#  endif

#  if !PTHREAD_IN_USE_DETECTION_HARD
#   pragma weak pthread_cancel
#   define pthread_in_use() (pthread_cancel != NULL)
#  endif

# else

#  if !PTHREAD_IN_USE_DETECTION_HARD
#   define pthread_in_use() 1
#  endif

# endif

/* ------------------------- gl_tls_key_t datatype ------------------------- */

typedef union
        {
          void *singlethread_value;
          pthread_key_t key;
        }
        gl_tls_key_t;
# define gl_tls_key_init(NAME, DESTRUCTOR) \
    do                                                             \
      {                                                            \
        if (pthread_in_use ())                                     \
          {                                                        \
            if (pthread_key_create (&(NAME).key, DESTRUCTOR) != 0) \
              abort ();                                            \
          }                                                        \
        else                                                       \
          (NAME).singlethread_value = NULL;                        \
      }                                                            \
    while (0)
# define gl_tls_get(NAME) \
    (pthread_in_use ()                  \
     ? pthread_getspecific ((NAME).key) \
     : (NAME).singlethread_value)
# define gl_tls_set(NAME, POINTER) \
    do                                                            \
      {                                                           \
        if (pthread_in_use ())                                    \
          {                                                       \
            if (pthread_setspecific ((NAME).key, (POINTER)) != 0) \
              abort ();                                           \
          }                                                       \
        else                                                      \
          (NAME).singlethread_value = (POINTER);                  \
      }                                                           \
    while (0)
# define gl_tls_key_destroy(NAME) \
    if (pthread_in_use () && pthread_key_delete ((NAME).key) != 0) \
      abort ()

#endif

/* ========================================================================= */

#if USE_PTH_THREADS

/* Use the GNU Pth threads library.  */

# include <pth.h>
# include <stdlib.h>

# if USE_PTH_THREADS_WEAK

/* Use weak references to the GNU Pth threads library.  */

#  pragma weak pth_key_create
#  pragma weak pth_key_getdata
#  pragma weak pth_key_setdata
#  pragma weak pth_key_delete

#  pragma weak pth_cancel
#  define pth_in_use() (pth_cancel != NULL)

# else

#  define pth_in_use() 1

# endif

/* ------------------------- gl_tls_key_t datatype ------------------------- */

typedef union
        {
          void *singlethread_value;
          pth_key_t key;
        }
        gl_tls_key_t;
# define gl_tls_key_init(NAME, DESTRUCTOR) \
    do                                                     \
      {                                                    \
        if (pth_in_use ())                                 \
          {                                                \
            if (!pth_key_create (&(NAME).key, DESTRUCTOR)) \
              abort ();                                    \
          }                                                \
        else                                               \
          (NAME).singlethread_value = NULL;                \
      }                                                    \
    while (0)
# define gl_tls_get(NAME) \
    (pth_in_use ()                  \
     ? pth_key_getdata ((NAME).key) \
     : (NAME).singlethread_value)
# define gl_tls_set(NAME, POINTER) \
    do                                                    \
      {                                                   \
        if (pth_in_use ())                                \
          {                                               \
            if (!pth_key_setdata ((NAME).key, (POINTER))) \
              abort ();                                   \
          }                                               \
        else                                              \
          (NAME).singlethread_value = (POINTER);          \
      }                                                   \
    while (0)
# define gl_tls_key_destroy(NAME) \
    if (pth_in_use () && !pth_key_delete ((NAME).key)) \
      abort ()

#endif

/* ========================================================================= */

#if USE_SOLARIS_THREADS

/* Use the old Solaris threads library.  */

# include <thread.h>
# include <stdlib.h>

# if USE_SOLARIS_THREADS_WEAK

/* Use weak references to the old Solaris threads library.  */

#  pragma weak thr_keycreate
#  pragma weak thr_getspecific
#  pragma weak thr_setspecific

#  pragma weak thr_suspend
#  define thread_in_use() (thr_suspend != NULL)

# else

#  define thread_in_use() 1

# endif

/* ------------------------- gl_tls_key_t datatype ------------------------- */

typedef union
        {
          void *singlethread_value;
          thread_key_t key;
        }
        gl_tls_key_t;
# define gl_tls_key_init(NAME, DESTRUCTOR) \
    do                                                        \
      {                                                       \
        if (thread_in_use ())                                 \
          {                                                   \
            if (thr_keycreate (&(NAME).key, DESTRUCTOR) != 0) \
              abort ();                                       \
          }                                                   \
        else                                                  \
          (NAME).singlethread_value = NULL;                   \
      }                                                       \
    while (0)
# define gl_tls_get(NAME) \
    (thread_in_use ()                \
     ? glthread_tls_get ((NAME).key) \
     : (NAME).singlethread_value)
extern void *glthread_tls_get (thread_key_t key);
# define gl_tls_set(NAME, POINTER) \
    do                                                        \
      {                                                       \
        if (thread_in_use ())                                 \
          {                                                   \
            if (thr_setspecific ((NAME).key, (POINTER)) != 0) \
              abort ();                                       \
          }                                                   \
        else                                                  \
          (NAME).singlethread_value = (POINTER);              \
      }                                                       \
    while (0)
# define gl_tls_key_destroy(NAME) \
    /* Unsupported.  */ \
    (void)0

#endif

/* ========================================================================= */

#if USE_WIN32_THREADS

# include <windows.h>

/* ------------------------- gl_tls_key_t datatype ------------------------- */

typedef DWORD gl_tls_key_t;
# define gl_tls_key_init(NAME, DESTRUCTOR) \
    /* The destructor is unsupported.  */    \
    if (((NAME) = TlsAlloc ()) == (DWORD)-1) \
      abort ()
# define gl_tls_get(NAME) \
    TlsGetValue (NAME)
# define gl_tls_set(NAME, POINTER) \
    if (!TlsSetValue (NAME, POINTER)) \
      abort ()
# define gl_tls_key_destroy(NAME) \
    if (!TlsFree (NAME)) \
      abort ()

#endif

/* ========================================================================= */

#if !(USE_POSIX_THREADS || USE_PTH_THREADS || USE_SOLARIS_THREADS || USE_WIN32_THREADS)

/* Provide dummy implementation if threads are not supported.  */

/* ------------------------- gl_tls_key_t datatype ------------------------- */

typedef struct
        {
          void *singlethread_value;
        }
        gl_tls_key_t;
# define gl_tls_key_init(NAME, DESTRUCTOR) \
    (NAME).singlethread_value = NULL
# define gl_tls_get(NAME) \
    (NAME).singlethread_value
# define gl_tls_set(NAME, POINTER) \
    (NAME).singlethread_value = (POINTER)
# define gl_tls_key_destroy(NAME) \
    (void)0

#endif

/* ========================================================================= */

#endif /* _TLS_H */
