/*	$NetBSD: pthread_debug.h,v 1.5 2003/02/15 04:33:45 nathanw Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nathan J. Williams.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _LIB_PTHREAD_DEBUG_H
#define _LIB_PTHREAD_DEBUG_H

#define PTHREADD_CREATE		0
#define PTHREADD_IDLE		1
#define PTHREADD_UPCALLS	2
#define PTHREADD_UP_NEW		3
#define PTHREADD_UP_BLOCK       4
#define PTHREADD_UP_UNBLOCK	5
#define PTHREADD_UP_PREEMPT	6
#define PTHREADD_UP_SIGNAL	7
#define PTHREADD_UP_SIGEV	8
#define PTHREADD_SPINLOCKS	9
#define PTHREADD_SPINUNLOCKS	10
#define PTHREADD_SPINPREEMPT	11
#define PTHREADD_RESOLVELOCKS	12
#define PTHREADD_SWITCHTO	13
#define PTHREADD_MUTEX_LOCK	14
#define PTHREADD_MUTEX_LOCK_SLOW	15
#define PTHREADD_MUTEX_TRYLOCK	16
#define PTHREADD_MUTEX_UNLOCK	17
#define PTHREADD_MUTEX_UNLOCK_UNBLOCK	18
#define PTHREADD_COND_WAIT	19
#define PTHREADD_COND_TIMEDWAIT	20
#define PTHREADD_COND_SIGNAL	21
#define PTHREADD_COND_BROADCAST	22
#define PTHREADD_COND_WOKEUP	23
#define PTHREADD_NCOUNTERS	24

#define PTHREADD_INITCOUNTERNAMES { \
	"pthread_create()",	/*  0 CREATE */		\
	"pthread_idle()",	/*  1 IDLE */		\
	"upcall",		/*  2 UPCALLS */	\
	"upcall: new",		/*  3 UP_NEW */		\
	"upcall: block",	/*  4 UP_BLOCK */	\
	"upcall: unblock",	/*  5 UP_UNBLOCK */	\
	"upcall: preempt",	/*  6 UP_PREEMPT */	\
	"upcall: signal",	/*  7 UP_SIGNAL */	\
	"upcall: sigev",	/*  8 UP_SIGEV */	\
	"spinlock",		/*  9 SPINLOCKS */	\
	"spinunlock",		/* 10 SPINUNLOCKS */	\
	"spin preemption",     	/* 11 SPINPREEMPT */	\
	"resolvelocks",		/* 12 RESOLVELOCKS */	\
	"switchto",		/* 13 SPINUNLOCKS */	\
	"mutex lock",		/* 14 MUTEX_LOCK */	\
	"mutex lock (slow)",   	/* 15 MUTEX_LOCK_SLOW */	\
	"mutex trylock",	/* 16 MUTEX_TRYLOCK */	\
	"mutex unlock",		/* 17 MUTEX_UNLOCK */	\
	"mutex unlock (wake)",  /* 18 MUTEX_UNLOCK_UNBLOCK */	\
	"cond wait",		/* 19 COND_WAIT */	\
	"cond timedwait",      	/* 20 COND_TIMEDWAIT */	\
	"cond broadcast",	/* 21 COND_BROADCAST */	\
	"cond signal",		/* 22 COND_SIGNAL */	\
	"cond wokeup",		/* 23 COND_WOKEUP */	\
}

#define PTHREAD__DEBUG_SHMKEY	(0x000f)
#define PTHREAD__DEBUG_SHMSIZE	(1<<18)

struct	pthread_msgbuf {
#define BUF_MAGIC	0x090976
	int	msg_magic;
	size_t	msg_bufw;
	size_t	msg_bufr;
	size_t	msg_bufs;
	char	msg_bufc[1];
};

void pthread__debug_init(void);
struct pthread_msgbuf* pthread__debuglog_init(int force);
void pthread__debuglog_printf(const char *fmt, ...);

#ifdef PTHREAD__DEBUG

#undef	PTHREAD_ALARM_DEBUG
#undef	PTHREAD_COND_DEBUG
#define PTHREAD_MAIN_DEBUG
#undef	PTHREAD_RUN_DEBUG
#define PTHREAD_SA_DEBUG
#undef	PTHREAD_SIG_DEBUG
#define PTHREAD_SPIN_DEBUG
#undef	PTHREAD_SPIN_DEBUG_PRINT

extern int pthread__debug_counters[PTHREADD_NCOUNTERS];

extern int pthread__dbg; /* Set by libpthread_dbg */

#define PTHREADD_ADD(x) (pthread__debug_counters[(x)]++)

#define DPRINTF(x) pthread__debuglog_printf x
#else /* PTHREAD_DEBUG */

#define PTHREADD_ADD(x)
#define DPRINTF(x)

#endif /* PTHREAD_DEBUG */

#endif /* _LIB_PTHREAD_DEBUG_H */
