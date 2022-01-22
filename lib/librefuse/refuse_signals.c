/* $NetBSD: refuse_signals.c,v 1.1 2022/01/22 07:53:06 pho Exp $ */

/*
 * Copyright (c) 2021 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#if !defined(lint)
__RCSID("$NetBSD: refuse_signals.c,v 1.1 2022/01/22 07:53:06 pho Exp $");
#endif /* !lint */

#include <assert.h>
#include <fuse_internal.h>
#if defined(MULTITHREADED_REFUSE)
#  include <pthread.h>
#endif
#include <signal.h>
#include <stdlib.h>
#include <string.h>

/* Signal handling routines
 *
 * FUSE only supports running a single filesystem per process. ReFUSE
 * is going to allow a process to run a filesystem per thread. In
 * order to support this, our implementation of
 * fuse_set_signal_handlers() installs a set of signal handlers which,
 * when invoked, terminates all the filesystems that called the
 * function. This means our fuse_remove_signal_handlers() must not
 * actually remove the signal handlers until the last thread calls the
 * function.
 *
 * FUSE installs a signal handler for a signal only if its sa_handler
 * is set to SIG_DFL. This obviously has a bad consequence: if the
 * caller process already has a non-default signal handler for SIGINT,
 * Ctrl-C will not stop the main loop of FUSE. See
 * https://stackoverflow.com/q/5044375/3571336
 *
 * Maybe we should do the same knowing it's bad, but it's probably
 * better to call our handler along with the old one. We may change
 * this behavior if this turns out to cause serious compatibility
 * issues.
 *
 * Also note that it is tempting to use puffs_unmountonsignal(3) but
 * we can't, because there is no way to revert its effect.
 */

#if defined(MULTITHREADED_REFUSE)
/* A mutex to protect the global state regarding signal handlers. When
 * a thread is going to lock this, it must block all the signals (with
 * pthread_sigmask(3)) that we install a handler for, or otherwise it
 * may deadlock for trying to acquire a lock that is already held by
 * itself. */
static pthread_mutex_t signal_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

/* Saved sigaction for each signal before we modify them. */
static struct sigaction* saved_actions[NSIG];

/* A linked list of "struct fuse*" which should be terminated upon
 * receiving a signal. */
struct refuse_obj_elem {
    struct fuse* fuse;
    struct refuse_obj_elem* next;
};
static struct refuse_obj_elem* fuse_head;

#if defined(MULTITHREADED_REFUSE)
static int
block_signals(sigset_t* oset) {
    sigset_t set;

    if (sigemptyset(&set) != 0)
        return -1;

    if (sigaddset(&set, SIGHUP) != 0)
        return -1;

    if (sigaddset(&set, SIGINT) != 0)
        return -1;

    if (sigaddset(&set, SIGTERM) != 0)
        return -1;

    return pthread_sigmask(SIG_BLOCK, &set, oset);
}

static int
unblock_signals(const sigset_t* oset) {
    return pthread_sigmask(SIG_SETMASK, oset, NULL);
}
#endif /* defined(MULTITHREADED_REFUSE) */

/* handler == NULL means the signal should be ignored. */
static int
set_signal_handler(int sig, void (*handler)(int, siginfo_t*, void*)) {
    struct sigaction* saved;
    struct sigaction act;

    saved = malloc(sizeof(*saved));
    if (!saved)
        return -1;

    if (sigaction(sig, NULL, saved) != 0) {
        free(saved);
        return -1;
    }

    saved_actions[sig] = saved;

    memset(&act, 0, sizeof(act));
    if (handler) {
        act.sa_sigaction = handler;
        act.sa_flags = SA_SIGINFO;
    }
    else {
        /* Ignore the signal only if the signal doesn't have a
         * handler. */
        if (!(saved->sa_flags & SA_SIGINFO) && saved->sa_handler == SIG_DFL)
            act.sa_handler = SIG_IGN;
        else
            return 0;
    }

    if (sigemptyset(&act.sa_mask) != 0) {
        free(saved);
        saved_actions[sig] = NULL;
        return -1;
    }

    return sigaction(sig, &act, NULL);
}

static int
restore_signal_handler(int sig, void (*handler)(int, siginfo_t*, void*)) {
    struct sigaction oact;
    struct sigaction* saved;

    saved = saved_actions[sig];
    assert(saved != NULL);

    if (sigaction(sig, NULL, &oact) != 0)
        return -1;

    /* Has the sigaction changed since we installed our handler? Do
     * nothing if so. */
    if (handler) {
        if (!(oact.sa_flags & SA_SIGINFO) || oact.sa_sigaction != handler)
            goto done;
    }
    else {
        if (oact.sa_handler != SIG_IGN)
            goto done;
    }

    if (sigaction(sig, saved, NULL) != 0)
        return -1;

  done:
    free(saved);
    saved_actions[sig] = NULL;
    return 0;
}

static void
exit_handler(int sig, siginfo_t* info, void* ctx) {
    struct refuse_obj_elem* elem;
    struct sigaction* saved;
#if defined(MULTITHREADED_REFUSE)
    int rv;

    /* pthread_mutex_lock(3) is NOT an async-signal-safe function. We
     * assume it's okay, as the thread running this handler shouldn't
     * be locking this mutex. */
    rv = pthread_mutex_lock(&signal_mutex);
    assert(rv == 0);
#endif

    for (elem = fuse_head; elem != NULL; elem = elem->next)
        fuse_exit(elem->fuse);

#if defined(MULTITHREADED_REFUSE)
    rv = pthread_mutex_unlock(&signal_mutex);
    assert(rv == 0);
#endif

    saved = saved_actions[sig];
    assert(saved != NULL);

    if (saved->sa_handler != SIG_DFL && saved->sa_handler != SIG_IGN) {
        if (saved->sa_flags & SA_SIGINFO)
            saved->sa_sigaction(sig, info, ctx);
        else
            saved->sa_handler(sig);
    }
}

/* The original function appeared on FUSE 2.5 takes a pointer to
 * "struct fuse_session" instead of "struct fuse". We have no such
 * things as fuse sessions.
 */
int
__fuse_set_signal_handlers(struct fuse* fuse) {
    int ret = 0;
    struct refuse_obj_elem* elem;
#if defined(MULTITHREADED_REFUSE)
    int rv;
    sigset_t oset;

    rv = block_signals(&oset);
    assert(rv == 0);

    rv = pthread_mutex_lock(&signal_mutex);
    assert(rv == 0);
#endif

    /* Have we already installed our signal handlers? If the list is
     * empty, it means we have not. */
    if (fuse_head == NULL) {
        if (set_signal_handler(SIGHUP, exit_handler) != 0 ||
            set_signal_handler(SIGINT, exit_handler) != 0 ||
            set_signal_handler(SIGTERM, exit_handler) != 0 ||
            set_signal_handler(SIGPIPE, NULL) != 0) {

            ret = -1;
            goto done;
        }
    }

    /* Add ourselves to the list of filesystems that want to be
     * terminated upon receiving a signal. But only if we aren't
     * already in the list. */
    for (elem = fuse_head; elem != NULL; elem = elem->next) {
        if (elem->fuse == fuse)
            goto done;
    }

    elem = malloc(sizeof(*elem));
    if (!elem) {
        ret = -1;
        goto done;
    }
    elem->fuse = fuse;
    elem->next = fuse_head;
    fuse_head  = elem;
  done:

#if defined(MULTITHREADED_REFUSE)
    rv = pthread_mutex_unlock(&signal_mutex);
    assert(rv == 0);

    rv = unblock_signals(&oset);
    assert(rv == 0);
#endif
    return ret;
}

int
__fuse_remove_signal_handlers(struct fuse* fuse) {
    int ret = 0;
    struct refuse_obj_elem* prev;
    struct refuse_obj_elem* elem;
#if defined(MULTITHREADED_REFUSE)
    int rv;
    sigset_t oset;

    rv = block_signals(&oset);
    assert(rv == 0);

    rv = pthread_mutex_lock(&signal_mutex);
    assert(rv == 0);
#endif

    /* Remove ourselves from the list. */
    for (prev = NULL, elem = fuse_head;
         elem != NULL;
         prev = elem, elem = elem->next) {

        if (elem->fuse == fuse) {
            if (prev)
                prev->next = elem->next;
            else
                fuse_head = elem->next;
            free(elem);
        }
    }

    /* Restore handlers if we were the last one. */
    if (fuse_head == NULL) {
        if (restore_signal_handler(SIGHUP, exit_handler) == -1 ||
            restore_signal_handler(SIGINT, exit_handler) == -1 ||
            restore_signal_handler(SIGTERM, exit_handler) == -1 ||
            restore_signal_handler(SIGPIPE, NULL) == -1) {

            ret = -1;
        }
    }

#if defined(MULTITHREADED_REFUSE)
    rv = pthread_mutex_unlock(&signal_mutex);
    assert(rv == 0);

    rv = unblock_signals(&oset);
    assert(rv == 0);
#endif
    return ret;
}
