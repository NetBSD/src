/*
 * Copyright (c) 1989 The Regents of the University of California.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
/*static char *sccsid = "from: @(#)sleep.c	5.6 (Berkeley) 2/23/91";*/
static char *rcsid = "$Id: sleep.c,v 1.4 1994/05/28 06:25:04 jtc Exp $";
#endif /* LIBC_SCCS and not lint */

#include <sys/time.h>
#include <signal.h>
#include <unistd.h>

unsigned int
sleep(seconds)
	unsigned int seconds;
{
	register struct itimerval *itp;
	struct itimerval itv, oitv;
	struct sigaction act, oact;
	struct timeval diff;
	sigset_t set, oset;
	static void sleephandler();

	itp = &itv;
	if (!seconds)
		return 0;

	sigemptyset (&set);
	sigaddset (&set, SIGALRM);
	sigprocmask(SIG_BLOCK, &set, &oset);

	act.sa_handler = sleephandler;
	act.sa_flags   = 0;
	sigemptyset(&act.sa_mask);
	sigaction(SIGALRM, &act, &oact);

	timerclear(&itp->it_interval);
	itp->it_value.tv_sec = seconds;
	itp->it_value.tv_usec = 0;
	diff.tv_sec = diff.tv_usec = 0;
	setitimer(ITIMER_REAL, itp, &oitv);

	if (timerisset(&oitv.it_value)) {
		if (timercmp(&oitv.it_value, &itp->it_value, >)) {
			oitv.it_value.tv_sec -= itv.it_value.tv_sec;
		} else {
			/* The existing timer was scheduled to fire 
			   before ours, so we compute the time diff
			   so we can add it back in the end. */

			itp->it_value.tv_sec  -= oitv.it_value.tv_sec;
			itp->it_value.tv_usec -= oitv.it_value.tv_usec;
			if (itp->it_value.tv_usec < 0) {
				itp->it_value.tv_sec--;
				itp->it_value.tv_usec += 1000000;
			}
			diff = itp->it_value;

			itp->it_value = oitv.it_value;
			/*
			 * This is a hack, but we must have time to return
			 * from the setitimer after the alarm or else it'll
			 * be restarted.  And, anyway, sleep never did
			 * anything more than this before.
			 */
			oitv.it_value.tv_sec  = 1;
			oitv.it_value.tv_usec = 0;

			setitimer(ITIMER_REAL, &itv, NULL);
		}
	}

 	(void) sigsuspend (&oset);

	sigaction(SIGALRM, &oact, NULL);
	sigprocmask(SIG_SETMASK, &oset, NULL);

	(void) setitimer(ITIMER_REAL, &oitv, itp);

	if (diff.tv_sec != 0 || diff.tv_usec != 0) {
		itp->it_value.tv_sec  += diff.tv_sec;
		itp->it_value.tv_usec += diff.tv_usec;
		if (itp->it_value.tv_usec > 1000000) {
			itp->it_value.tv_usec -= 1000000;
			itp->it_value.tv_sec++;
		}
	}

	return (itp->it_value.tv_sec);
}

static void
sleephandler()
{
}
