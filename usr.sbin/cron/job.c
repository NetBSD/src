/*	$NetBSD: job.c,v 1.5 2003/08/13 03:51:15 atatat Exp $	*/

/* Copyright 1988,1990,1993,1994 by Paul Vixie
 * All rights reserved
 *
 * Distribute freely, except: don't remove my name from the source or
 * documentation (don't take credit for my work), mark your changes (don't
 * get me blamed for your possible bugs), don't alter or remove this
 * notice.  May be sold if buildable source is provided to buyer.  No
 * warrantee of any kind, express or implied, is included with this
 * software; use at your own risk, responsibility for damages (if any) to
 * anyone resulting from the use of this software rests entirely with the
 * user.
 *
 * Send bug reports, bug fixes, enhancements, requests, flames, etc., and
 * I'll try to keep a version up to date.  I can be reached as follows:
 * Paul Vixie          <paul@vix.com>          uunet!decwrl!vixie!paul
 */

#include <sys/cdefs.h>
#include <errno.h>
#if SYS_TIME_H
# include <sys/time.h>
#else
# include <time.h>
#endif
#if !defined(lint) && !defined(LINT)
#if 0
static char rcsid[] = "Id: job.c,v 1.6 1994/01/15 20:43:43 vixie Exp";
#else
__RCSID("$NetBSD: job.c,v 1.5 2003/08/13 03:51:15 atatat Exp $");
#endif
#endif


#include "cron.h"


typedef	struct _job {
	struct _job	*next;
	entry		*e;
	user		*u;
	time_t		t;
} job;


static job	*jhead = NULL, *jtail = NULL;


static int okay_to_go __P((job *));


void
job_add(e, u)
	entry *e;
	user *u;
{
	job *j;

	/* if already on queue, keep going */
	for (j=jhead; j; j=j->next)
		if (j->e == e && j->u == u) {
			j->t = TargetTime;
			return;
		}

	/* build a job queue element */
	j = (job*)malloc(sizeof(job));
	j->next = (job*) NULL;
	j->e = e;
	j->u = u;
	j->t = TargetTime;

	/* add it to the tail */
	if (!jhead) { jhead=j; }
	else { jtail->next=j; }
	jtail = j;
}


int
job_runqueue()
{
	job	*j, *jn;
	int	run = 0;

	for (j=jhead; j; j=jn) {
		if (okay_to_go(j))
			do_command(j->e, j->u);
		else {
			char *x = mkprints((u_char *)j->e->cmd,
			    strlen(j->e->cmd));
			char *usernm = env_get("LOGNAME", j->e->envp);

			log_it(usernm, getpid(), "CMD (skipped)", x);
			free(x);
		}
		jn = j->next;
		free(j);
		run++;
	}
	jhead = jtail = NULL;
	return run;
}


static int
okay_to_go(j)
	job	*j;
{
	char *within, *t;
	int delta;

	if (j->e->flags & WHEN_REBOOT)
		return (1);

	within = env_get("CRON_WITHIN", j->e->envp);
	if (within == NULL)
		return (1);

	/* XXX handle 2m, 4h, etc? */
	errno = 0;
	delta = strtol(within, &t, 10);
	if (errno == ERANGE || *t != '\0' || delta <= 0)
		return (1);

	return ((j->t + delta) > time(NULL));
}
