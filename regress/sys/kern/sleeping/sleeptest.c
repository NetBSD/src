/* $NetBSD: sleeptest.c,v 1.3 2010/03/23 20:32:13 drochner Exp $ */

/*-
 * Copyright (c) 2006 Frank Kardel
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

#include <sys/event.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <sys/signal.h>
#include <string.h>
#include <err.h>
#include <unistd.h>
#include <poll.h>

#define FUZZ		30000000LL	/* scheduling fuzz accepted - 30 ms */
#define MAXSLEEP	32000000000LL	/* 32 seconds */
#define ALARM		11		/* SIGALRM after this time */
#define KEVNT_TIMEOUT	13200

static int sig, sigs;

static char *sleepers[] = {
	"nanosleep",
	"select",
	"poll",
	"sleep",
	"kevent"
};

#define N_SLEEPERS (sizeof(sleepers)/sizeof(sleepers[0]))

void
sigalrm()
{
	sig++;
}

int
main(int argc, char *argv[])
{
	struct timespec tsa, tsb, tslp, tslplast, tremain;
	struct timeval tv;
	int64_t delta1, delta2, delta3, round;
	int errors = 0, i;
	char *rtype;

	printf("Testing sleep/timeout implementations\n");
	printf("accepted scheduling delta %lld ms\n", FUZZ/ 1000000LL);
	printf("testing up to %lld ms for %d interfaces\n", MAXSLEEP / 1000000LL, N_SLEEPERS);
	printf("ALARM interrupt after %d sec once per run\n", ALARM);
	printf("kevent timer will fire after %d ms\n", KEVNT_TIMEOUT);

	signal(SIGALRM, sigalrm);

	for (i = 0; i < N_SLEEPERS; i++) {
		switch (i) {
		case 3: /* sleep has only second resolution */
			round = 1000000000;
			delta3 = round;
			break;
		default:
			round = 1;
			delta3 = FUZZ;
			break;
		}
		sigs += sig;
		sig = 0;
		rtype = "simulated";

		tslp.tv_sec = delta3 / 1000000000;
		tslp.tv_nsec = delta3 % 1000000000;
		tslplast = tslp;

		while (delta3 <= MAXSLEEP) {
			printf("[%s] sleeping %lld nsec ... ", sleepers[i], delta3);
			/*
			 * disturb sleep by signal on purpose
			 */ 
			if (delta3 > ALARM * 1000000000LL && sig == 0)
				alarm(ALARM);

			fflush(stdout);
			clock_gettime(CLOCK_REALTIME, &tsa);
			switch (i) {
			case 0:	/* nanosleep */
				rtype = "returned";
				if (nanosleep(&tslp, &tremain) == -1) {
					if (errno != EINTR)
						errors++;
					printf("- errno=%s (%s) - ", strerror(errno), (errno == EINTR) ? "OK" : "ERROR");
				}
				clock_gettime(CLOCK_REALTIME, &tsb);
				break;

			case 1:	/* select */
				TIMESPEC_TO_TIMEVAL(&tv, &tslp);
				if (select(0, NULL, NULL, NULL, &tv) == -1) {
					if (errno != EINTR)
						errors++;
					printf("- errno=%s (%s) - ", strerror(errno), (errno == EINTR) ? "OK" : "ERROR");
				}
				/* simulate remaining time */
				clock_gettime(CLOCK_REALTIME, &tsb);
				timespecsub(&tsb, &tsa, &tremain);
				timespecsub(&tslp, &tremain, &tremain);
				break;

			case 2:	/* poll */
				TIMESPEC_TO_TIMEVAL(&tv, &tslp);
				if (pollts(NULL, 0, &tslp, NULL) == -1) {
					if (errno != EINTR)
						errors++;
					printf("- errno=%s (%s) - ", strerror(errno), (errno == EINTR) ? "OK" : "ERROR");
				}
				/* simulate remaining time */
				clock_gettime(CLOCK_REALTIME, &tsb);
				timespecsub(&tsb, &tsa, &tremain);
				timespecsub(&tslp, &tremain, &tremain);
				break;

			case 3:	/* sleep */
				rtype = "returned";
				tremain.tv_sec = sleep(tslp.tv_sec);
				tremain.tv_nsec = 0;
				clock_gettime(CLOCK_REALTIME, &tsb);
				break;

			case 4:	/* kevent */
			{
				struct kevent ktimer;
				struct kevent kresult;
				int rtc, timeout = KEVNT_TIMEOUT;
				int kq = kqueue();
				
				if (kq == -1) {
					err(EXIT_FAILURE, "kqueue");
				}

				EV_SET(&ktimer, 1, EVFILT_TIMER, EV_ADD, 0, timeout, 0);

				rtc = kevent(kq, &ktimer, 1, &kresult, 1, &tslp);
				if (rtc == -1) {
					if (errno != EINTR)
						errors++;
					printf("- errno=%s (%s) - ", strerror(errno), (errno == EINTR) ? "OK" : "ERROR");
				}
				clock_gettime(CLOCK_REALTIME, &tsb);
				if (rtc == 0) {
					printf("- not fired");
					if (tslp.tv_sec * 1000000000LL + tslp.tv_nsec -
					    timeout * 1000000LL > 0) {
						printf(" - TIMING ERROR");
						errors++;
					}
					printf(" - ");
				} else if (rtc > 0) {
					printf("- fired - ");
				}
				timespecsub(&tsb, &tsa, &tremain);
				timespecsub(&tslp, &tremain, &tremain);
				(void)close(kq);
			}
			break;

			default:
				errx(EXIT_FAILURE, "programming error - sleep method to test not implemented\n");
			}

			delta1 = ((int64_t)tsb.tv_sec - (int64_t)tsa.tv_sec) * 1000000000LL + (int64_t)tsb.tv_nsec - (int64_t)tsa.tv_nsec;
			delta2 = (((int64_t)tremain.tv_sec * 1000000000LL) + (int64_t)tremain.tv_nsec);
			delta3 = (int64_t)tslp.tv_sec * 1000000000LL + (int64_t)tslp.tv_nsec - delta1 - delta2;
			printf("sig %d, real time sleep %lld nsec, remain(%s) %lld nsec, diff to supposed sleep %lld nsec (rounded %lld nsec)",
			       sigs + sig, delta1, rtype, delta2, delta3, (delta3 / round) * round);
			fflush(stdout);
			delta3 /= round;
			delta3 *= round;
			if (delta3 > FUZZ || delta3 < -FUZZ) {
				errors++;
				printf(" ERROR\n");
				delta3 = ((int64_t)tslp.tv_sec + (int64_t)tslplast.tv_sec) * 1000000000LL + (int64_t)tslplast.tv_nsec 
					+ (int64_t)tslp.tv_nsec;
				delta3 /= 2;
			} else {
				printf(" OK\n");
				tslplast = tslp;
				delta3 = ((int64_t)tslp.tv_sec + (int64_t)tslplast.tv_sec) * 1000000000LL + (int64_t)tslplast.tv_nsec 
					+ (int64_t)tslp.tv_nsec;
			}
			delta3 /= round;
			delta3 *= round;
			if (delta3 < FUZZ)
				break;
			tslp.tv_sec = delta3 / 1000000000LL;
			tslp.tv_nsec = delta3 % 1000000000LL;
		}
	}
	sigs += sig;

	if (errors || sigs != N_SLEEPERS) {
		printf("TEST FAIL (%d errors, %d alarms, %d interfaces)\n", errors, sigs, N_SLEEPERS);
		return 1;
	} else {
		printf("TEST SUCCESSFUL\n");
		return 0;
	}
}
