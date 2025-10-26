/*	$NetBSD: pthread.h,v 1.1 2025/10/26 20:48:28 christos Exp $	*/

#ifndef COMPAT_PTHREAD_H
#define COMPAT_PTHREAD_H

#include <sys/mutex.h>
#include <pthread_types.h>

int     __compat_pthread_setname_np(pthread_t, const char *, void *)
    __dso_hidden;
int     __pthread_setname_np120(pthread_t, const char *, ...);
#endif
