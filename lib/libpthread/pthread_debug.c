/*	$NetBSD: pthread_debug.c,v 1.14 2007/11/13 15:57:11 ad Exp $	*/

/*-
 * Copyright (c) 2001, 2006, 2007 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__RCSID("$NetBSD: pthread_debug.c,v 1.14 2007/11/13 15:57:11 ad Exp $");

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
static struct pthread_msgbuf *debugbuf;
#define	MAXLINELEN 1000
struct linebuf {
	char buf[MAXLINELEN];
	int len;
};
static struct linebuf *linebuf;

static void pthread__debug_printcounters(void);
static const char *pthread__counternames[] = PTHREADD_INITCOUNTERNAMES;

extern int pthread__started;

void
pthread__debug_init(void)
{
	time_t t;

	if (pthread__getenv("PTHREAD_DEBUGCOUNTERS") != NULL)
		atexit(pthread__debug_printcounters);

	if (pthread__getenv("PTHREAD_DEBUGLOG") != NULL) {
		t = time(NULL);
		debugbuf = pthread__debuglog_init(0);
		linebuf = calloc(1000, sizeof(struct linebuf));
		if (linebuf == NULL)
			err(1, "Couldn't allocate linebuf");

		DPRINTF(("Started debugging %s (pid %d) at %s\n", 
		    getprogname(), getpid(), ctime(&t)));
	}
}

static void
pthread__debug_printcounters(void)
{
	int i;
	int counts[PTHREADD_NCOUNTERS];

	/*
	 * Copy the counters before printing anything to so that we don't see
	 * the effect of printing the counters.
	 * There will still be one more mutex lock than unlock, because
	 * atexit() handling itself will call us back with a mutex locked.
	 */
	for (i = 0; i < PTHREADD_NCOUNTERS; i++)
		counts[i] = pthread__debug_counters[i];

	printf("Pthread event counters\n");
	for (i = 0; i < PTHREADD_NCOUNTERS; i++)
		printf("%2d %-20s %9d\n", i, pthread__counternames[i], 
		    counts[i]);
	printf("\n");
}

struct pthread_msgbuf *
pthread__debuglog_init(int force)
{
	int debugshmid;
	void *debuglog;
	struct pthread_msgbuf* buf;

	debugshmid = shmget((key_t)PTHREAD__DEBUG_SHMKEY,
	    PTHREAD__DEBUG_SHMSIZE, IPC_CREAT | S_IRWXU | S_IRWXG | S_IRWXO);
	
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
	char *tmpbuf;
	size_t len, cplen, r, w, s;
	long diff1, diff2;
	int vpid;
	va_list ap;

	if (debugbuf == NULL || linebuf == NULL) 
		return;

	vpid = (int)_lwp_self();
	tmpbuf = linebuf[vpid].buf;
	len = linebuf[vpid].len;

	if (len == 0)
		len += sprintf(tmpbuf, "[%05d.%05d]", getpid(), vpid);

	va_start(ap, fmt);
	len += vsnprintf(tmpbuf + len, (unsigned int)(MAXLINELEN - len),
	    fmt, ap);
	va_end(ap);
	
	linebuf[vpid].len = len;

	if (tmpbuf[len - 1] != '\n')
		return;

	r = debugbuf->msg_bufr;
	w = debugbuf->msg_bufw;
	s = debugbuf->msg_bufs;
	diff1 = (long)w - (long)r;

	if (w + len >= s) {
		cplen = s - w;
		debugbuf->msg_bufw = len - cplen;
		(void)memcpy(&debugbuf->msg_bufc[w],
		    tmpbuf, cplen);
		(void)memcpy(&debugbuf->msg_bufc[0], tmpbuf + cplen,
		    len - cplen);
		w = len - cplen;

		/* Check for lapping the read pointer. */
		diff2 = (long)w - (long)r;
		if (((diff1 < 0) && (diff2 <= 0)) ||
		    ((diff1 > 0) && (diff2 >= 0)))
			debugbuf->msg_bufr = (w + 1) % s;
	} else {
		debugbuf->msg_bufw = w + len;
		(void)memcpy(&debugbuf->msg_bufc[w],
		    tmpbuf, len);
		w += len;

		/* Check for lapping the read pointer. */
		diff2 = (long)w - (long)r;
		if ((diff1 < 0) && (diff2 >= 0))
			debugbuf->msg_bufr = (w + 1) % s;
	}

	linebuf[vpid].len = 0;
}

int
pthread__debuglog_newline(void)
{
	int vpid;

	if (debugbuf == NULL) 
		return 1;

	vpid = (int)_lwp_self();
	return (linebuf[vpid].len == 0);
}

#else	/* PTHREAD__DEBUG */

void
pthread__debug_init(void)
{

}

#endif	/* PTHREAD__DEBUG */
