/*	$NetBSD: pthread_debug.c,v 1.1.2.10 2002/05/20 17:52:08 nathanw Exp $	*/

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

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/types.h>

#include "pthread.h"
#include "pthread_int.h"

int pthread__dbg;

#ifdef PTHREAD__DEBUG

int pthread__debug_counters[PTHREADD_NCOUNTERS];
static struct pthread_msgbuf* debugbuf;

static void pthread__debug_printcounters(void);

void pthread__debug_init(void)
{
	time_t t;

	if (getenv("PTHREAD_DEBUGCOUNTERS") != NULL)
		atexit(pthread__debug_printcounters);

	if (getenv("PTHREAD_DEBUGLOG") != NULL) {
		t = time(NULL);
		debugbuf = pthread__debuglog_init(0);
		DPRINTF(("Started debugging %s (pid %d) at %s\n", 
		    getprogname(), getpid(), ctime(&t)));
	}
}

static void
pthread__debug_printcounters(void)
{
	int i;

	if (getenv("PTHREAD_DEBUGCOUNTERS") == NULL)
		return;

	printf("Pthread event counters:\n");
	for (i = 0; i < PTHREADD_NCOUNTERS; i++)
		printf("Counter %2d: %9d\n", i, pthread__debug_counters[i]);
	printf("\n");
}

struct pthread_msgbuf *
pthread__debuglog_init(int force)
{
	int debugshmid;
	void *debuglog;
	struct pthread_msgbuf* buf;

	debugshmid = shmget(PTHREAD__DEBUG_SHMKEY, PTHREAD__DEBUG_SHMSIZE,
	    IPC_CREAT | S_IRWXU | S_IRWXG | S_IRWXO);
	
	if (debugshmid == -1)
		err(1, "Couldn't get shared debug log");

	debuglog = shmat(debugshmid, 0, 0);

	if (debuglog == (void *)-1)
		err(1, "Couldn't map shared debug log (ID %d)", debugshmid);

	buf = (struct pthread_msgbuf *)debuglog;

	if (force || buf->msg_magic != BUF_MAGIC) {
		/* Initialize */
		buf->msg_magic = BUF_MAGIC;
		buf->msg_bufw = 0;
		buf->msg_bufr = 0;
		buf->msg_bufs = PTHREAD__DEBUG_SHMSIZE - 
		    sizeof (struct pthread_msgbuf);
	}

	return buf;
}

void
pthread__debuglog_printf(const char *fmt, ...)
{
	char tmpbuf[200];
	long len, cplen, diff1, diff2;
	va_list ap;

	if (debugbuf == NULL) 
		return;

	va_start(ap, fmt);
	len = vsnprintf(tmpbuf, 200, fmt, ap);
	va_end(ap);

	diff1 = debugbuf->msg_bufw - debugbuf->msg_bufr;

	if (debugbuf->msg_bufw + len > debugbuf->msg_bufs) {
		cplen = debugbuf->msg_bufs - debugbuf->msg_bufw;
		memcpy(&debugbuf->msg_bufc[debugbuf->msg_bufw],
		    tmpbuf, cplen);
		memcpy(&debugbuf->msg_bufc[0], tmpbuf + cplen, len - cplen);
		debugbuf->msg_bufw = len - cplen;
	} else {
		memcpy(&debugbuf->msg_bufc[debugbuf->msg_bufw],
		    tmpbuf, len);
		debugbuf->msg_bufw += len;
	}

	diff2 = debugbuf->msg_bufw - debugbuf->msg_bufr;
	
	/* Check if we've lapped the read pointer and if so advance it. */
	if (((diff1 < 0) && (diff2 >= 0)) || ((diff1 >= 0) && (diff2 < 0)))
		debugbuf->msg_bufr = debugbuf->msg_bufw;

}
#endif
