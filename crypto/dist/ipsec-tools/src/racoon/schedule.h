/*	$NetBSD: schedule.h,v 1.6 2008/09/19 11:01:08 tteras Exp $	*/

/* Id: schedule.h,v 1.5 2006/05/03 21:53:42 vanhu Exp */

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
 * Copyright (C) 2008 Timo Teras.
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
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _SCHEDULE_H
#define _SCHEDULE_H

#include <sys/queue.h>
#include "gnuc.h"

#ifndef offsetof
#ifdef __compiler_offsetof
#define offsetof(TYPE,MEMBER) __compiler_offsetof(TYPE,MEMBER)
#else
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif
#endif

#ifndef container_of
#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})
#endif


/* scheduling table */
/* the head is the nearest event. */
struct sched {
	time_t xtime;		/* event time which is as time(3). */
				/*
				 * if defined FIXY2038PROBLEM, this time
				 * is from the time when called sched_init().
				 */
	void (*func) __P((struct sched *)); /* call this function when timeout. */

	long id;		/* for debug */
	time_t created;		/* for debug */
	time_t tick;		/* for debug */

	TAILQ_ENTRY(sched) chain;
};

#define SCHED_INITIALIZER() { 0, NULL, }

struct scheddump {
	time_t xtime;
	long id;
	time_t created;
	time_t tick;
};

struct timeval *schedular __P((void));
void sched_schedule __P((struct sched *, time_t,
			 void (*func) __P((struct sched *))));
void sched_cancel __P((struct sched *));

int sched_dump __P((caddr_t *, int *));
void sched_init __P((void));

#endif /* _SCHEDULE_H */
