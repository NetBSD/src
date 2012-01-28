
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004, 2005
 *
 */


#if (defined (__FreeBSD__) || defined (__OpenBSD__) || defined(__NetBSD__))
#include <sys/param.h>
#include <sys/sysctl.h>
#include <err.h>
#elif (defined (__linux) || defined (linux) || defined(__GLIBC__))
#include <utmp.h>
#elif (defined (SOLARIS))
#include <utmpx.h>
#endif

#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "trousers/tss.h"
#include "trousers_types.h"
#include "tcs_tsp.h"
#include "tcs_int_literals.h"
#include "capabilities.h"
#include "tcsps.h"
#include "tcslog.h"


#if (defined (__linux) || defined (linux) || defined(__GLIBC__))
MUTEX_DECLARE_INIT(utmp_lock);

char
platform_get_runlevel()
{
	char runlevel;
	struct utmp ut, save, *next = NULL;
	struct timeval tv;
	int flag = 0, counter = 0;

	MUTEX_LOCK(utmp_lock);

	memset(&ut, 0, sizeof(struct utmp));
	memset(&save, 0, sizeof(struct utmp));
	memset(&tv, 0, sizeof(struct timeval));

	ut.ut_type = RUN_LVL;

	next = getutid(&ut);

	while (next != NULL) {
		if (next->ut_tv.tv_sec > tv.tv_sec) {
			memcpy(&save, next, sizeof(*next));
			flag = 1;
		} else if (next->ut_tv.tv_sec == tv.tv_sec) {
			if (next->ut_tv.tv_usec > tv.tv_usec) {
				memcpy(&save, next, sizeof(*next));
				flag = 1;
			}
		}

		counter++;
		next = getutid(&ut);
	}

	if (flag) {
		//printf("prev_runlevel=%c, runlevel=%c\n", save.ut_pid / 256, save.ut_pid % 256);
		runlevel = save.ut_pid % 256;
	} else {
		//printf("unknown\n");
		runlevel = 'u';
	}

	MUTEX_UNLOCK(utmp_lock);

	return runlevel;
}
#elif (defined (__FreeBSD__) || defined (__OpenBSD__) || defined(__NetBSD__))

char
platform_get_runlevel()
{
	int mib[2], rlevel = -1;
	size_t len;

	mib[0] = CTL_KERN;
	mib[1] = KERN_SECURELVL;
	
	len = sizeof(rlevel);
	if (sysctl(mib,2,&rlevel,&len, NULL,0) == -1) {
		err(1,"Could not get runlevel");
		return 'u';
	}
#if defined (__OpenBSD__)
	if (rlevel == 0)
#else
	if (rlevel == -1)
#endif
		return 's';	

	return rlevel + '0';
}
#elif (defined (SOLARIS))

MUTEX_DECLARE_INIT(utmp_lock);
char
platform_get_runlevel()
{
	char runlevel;
	struct utmpx ut, *utp = NULL;

	MUTEX_LOCK(utmp_lock);

	memset(&ut, 0, sizeof(ut));
	ut.ut_type = RUN_LVL;

	setutxent();
	utp = getutxid(&ut);
	if (utp->ut_type == RUN_LVL &&
	    sscanf(utp->ut_line, "run-level %c", &runlevel) != 1)
			runlevel = 'u';
	endutxent();

	MUTEX_UNLOCK(utmp_lock);

	return runlevel;
}
#endif
