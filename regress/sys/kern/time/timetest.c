/* $NetBSD: timetest.c,v 1.1 2006/09/10 11:37:04 kardel Exp $ */

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

#include <sys/param.h>
#include <machine/int_limits.h>
#include <sys/sysctl.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define MINPOSDIFF 15000000	/* 15 ms for now */

#define TC_HARDWARE "kern.timecounter.hardware"
#define TC_CHOICE "kern.timecounter.choice"

static char **ctrs;	/* NULL terminate list of timecounter names */
static int cnt;			/* last index of time counter name array */

static long long
check_time(time_t endlimit, int exitonerror, int verbose)
{
  struct timespec tsa;
  struct timespec tsb;
  struct timespec tsl;
  long long mindiff = INTMAX_MAX;
  time_t startsec;

  if (clock_gettime(CLOCK_REALTIME, &tsa) == -1)
    perror("clock_gettime(CLOCK_REALTIME, &tsa)");
  tsl = tsa;
  startsec = tsl.tv_sec;

  while (endlimit == 0 || (time_t)tsa.tv_sec < endlimit) {
    long long diff;

    if (clock_gettime(CLOCK_REALTIME, &tsb) == -1)
      perror("clock_gettime(CLOCK_REALTIME, &tsb)");
    diff = 1000000000LL * (tsb.tv_sec - tsa.tv_sec) + tsb.tv_nsec - tsa.tv_nsec;

    if (diff > 0 && mindiff > diff)
	    mindiff = diff;

    if (diff < 0 || (verbose && diff > MINPOSDIFF)) {
	    printf("%stime TSA: 0x%x.%08x, TSB: 0x%x.%08x, diff = %lld nsec, ", (diff < 0) ? "BAD " : "", tsa.tv_sec, tsa.tv_nsec, tsb.tv_sec, tsb.tv_nsec, diff);
      diff = 1000000000LL * (tsb.tv_sec - tsl.tv_sec) + tsb.tv_nsec - tsl.tv_nsec;
      printf("%lld nsec\n", diff);
      tsl = tsb;
      if (exitonerror && diff < 0)
	      return -1LL;
    }
    if (tsa.tv_sec != tsb.tv_sec && verbose > 1) {
	    printf("%06d\r", (int)(tsb.tv_sec - startsec));
	    fflush(stdout);
    }
    tsa.tv_sec = tsb.tv_sec;
    tsa.tv_nsec = tsb.tv_nsec;
  }

  return mindiff;
}

static void
usage(const char *name)
{
	printf("Usage: %s [-c] [-v] [-t <timeout>] [-A]\n", name);
}

static void
add_counter(const char *ctr)
{
	if (ctrs == NULL) {
		ctrs = malloc(2*sizeof(char *));
	} else {
		ctrs = realloc(ctrs, (cnt+2)*sizeof(char *));
	}
	ctrs[cnt++] = strdup(ctr);
	ctrs[cnt] = NULL;
}

static void
split_counters(const char *choice)
{
	const char *s = choice;
	const char *e = choice + strlen(choice);
	char name[128];
	int quality;

	while (s < e) {
		int items = sscanf(s, "%127[^(](q=%d, f=%*u Hz)", name, &quality);

		if (2 != items)
			return;

		if (quality >= 0)
			add_counter(name);

		s = strchr(s, ')');
		if (s) {
			s++;
			while (*s == ' ')
				s++;
		}
		else
			return;
	}
}

int
main(int argc, char **argv)
{
	extern char *optarg;
	extern int optind;
	char buf[512];
	char cbuf[512];
	size_t cbufsiz = sizeof cbuf;
	char ctrbuf[10240];
	size_t ctrbufsiz = sizeof ctrbuf;
	char *defaultcounter = NULL;
	long long mindiff;
	time_t endtime;
	time_t timeout;
	int ch, index;
	int verbose, exitonerror, allcounters;

	verbose = 0;
	exitonerror = 0;
	endtime = 0;
	allcounters = 0;

	while ((ch = getopt(argc, argv, "cvAt:")) != -1) {
		switch (ch) {
		case 'A':
			allcounters = 1;
			break;

		case 'c':
			exitonerror = 1;
			break;

		case 'v':
			if (verbose < 2)
				verbose++;
			break;

		case 't':
		{
			endtime = atoi(optarg);
			if (endtime <= 0) {
				printf("timeout must be positive\n");
				return 1;
			}
		}
		break;

		case '?':
		default:
			usage(argv[0]);
		}
	}
	argc -= optind;
	argv += optind;

	if (endtime) {
		snprintf(buf, sizeof buf, "%d seconds", (int)endtime);
	} else {
		timeout = endtime;
		strncpy(buf, "forever", sizeof(buf));
		if (allcounters) {
			printf("testing all counters requires -c <timeout> arguments\n");
			return 1;
		}
	}

	if (sysctlbyname(TC_HARDWARE, cbuf, &cbufsiz, NULL, 0) != 0) {
		strncpy(cbuf, "legacy time implementation", sizeof cbuf);
		allcounters = 0;
	} else {
		defaultcounter = strdup(cbuf);
		snprintf(cbuf, sizeof cbuf, "timecounter \"%s\"", defaultcounter);

		if (sysctlbyname(TC_CHOICE, ctrbuf, &ctrbufsiz, NULL, 0) != 0) {
			perror("sysctlbyname(\"" TC_CHOICE "\", ...)");
			return 3;
		}
		split_counters(ctrbuf);
		if (allcounters) {
			printf("Will test active counter and counters with positive quality from %s\n", ctrbuf);
		}
	}

	index = 0;

	do {
		if (endtime) {
			time(&timeout);
			timeout += endtime + 1;
		}
		printf("Testing time for monotonicity of %s for %s...\n", cbuf, buf);
		mindiff = check_time(timeout, exitonerror, verbose);
		if (mindiff < 0) {
			printf("TEST FAILED\n");
			if (allcounters)
				(void)sysctlbyname(TC_HARDWARE, NULL, 0,
					     defaultcounter, strlen(defaultcounter));
			return 2;
		} else {
			struct timespec res;
			
			if (clock_getres(CLOCK_REALTIME, &res) == 0) {
				long long resolution = res.tv_sec * 1000000000 + res.tv_nsec;
				
				printf("claimed resolution %lld nsec (%f Hz) or better, observed minimum non zero delta %lld nsec\n",
				       resolution, 1.0 / resolution * 1e9, mindiff);
			}
		}
		if (allcounters && index < cnt) {
			if (sysctlbyname(TC_HARDWARE, NULL, 0,
					 ctrs[index], strlen(ctrs[index])) != 0) {
				perror("selecting new timecounter");
				return 3;
			} else {
				struct timespec ts;

				/* wait a bit to select new counter in clockinterrupt */
				ts.tv_sec = 0;
				ts.tv_nsec = 100000000;
				printf("switching to timecounter \"%s\"...\n", ctrs[index]);
				nanosleep(&ts, NULL);
			}
			snprintf(cbuf, sizeof cbuf, "timecounter \"%s\"", ctrs[index]);
		}
	} while (allcounters && ++index <= cnt);
	/* restore the counter we started with */
	if (allcounters)
		(void)sysctlbyname(TC_HARDWARE, NULL, 0,
				   defaultcounter, strlen(defaultcounter));
	printf("TEST SUCCESSFUL\n");
	return 0;
}
