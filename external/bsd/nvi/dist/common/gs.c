/*	$NetBSD: gs.c,v 1.4 2013/12/01 02:34:54 christos Exp $ */
/*-
 * Copyright (c) 2000
 *	Sven Verdoolaege.  All rights reserved.
 *
 * See the LICENSE file for redistribution information.
 */

#include "config.h"

#include <sys/types.h>
#include <sys/queue.h>

#include <bitstring.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../common/common.h"
#ifdef USE_PERL_INTERP
#include "perl_api_extern.h"
#endif

__dead static void	   perr __P((char *, char *));

/*
 * gs_init --
 *	Create and partially initialize the GS structure.
 * PUBLIC: GS * gs_init __P((char*));
 */
GS *
gs_init(char *name)
{
	GS *gp;
	char *p;

	/* Figure out what our name is. */
	if ((p = strrchr(name, '/')) != NULL)
		name = p + 1;

	/* Allocate the global structure. */
	CALLOC_NOMSG(NULL, gp, GS *, 1, sizeof(GS));
	if (gp == NULL)
		perr(name, NULL);

	gp->progname = name;

	/* Common global structure initialization. */
	/* others will need to be copied from main.c */
	TAILQ_INIT(&gp->dq);

	TAILQ_INIT(&gp->hq);
	gp->noprint = DEFAULT_NOPRINT;

	/* Structures shared by screens so stored in the GS structure. */
	TAILQ_INIT(&gp->frefq);
	TAILQ_INIT(&gp->exfq);
	LIST_INIT(&gp->seqq);

	thread_init(gp);

	return (gp);
}

/*
 * gs_new_win
 *	Create new window
 * PUBLIC: WIN * gs_new_win __P((GS *gp));
 */

WIN *
gs_new_win(GS *gp)
{
	WIN *wp;

	CALLOC_NOMSG(NULL, wp, WIN *, 1, sizeof(*wp));
	if (!wp)
		return NULL;

	/* Common global structure initialization. */
	LIST_INIT(&wp->ecq);
	LIST_INSERT_HEAD(&wp->ecq, &wp->excmd, q);

	TAILQ_INSERT_TAIL(&gp->dq, wp, q);
	TAILQ_INIT(&wp->scrq);

	TAILQ_INIT(&wp->dcb_store.textq);
	LIST_INIT(&wp->cutq);

	wp->gp = gp;

	return wp;
}

/*
 * win_end --
 *	Remove window.
 *
 * PUBLIC: int win_end __P((WIN *wp));
 */
int
win_end(WIN *wp)
{
	SCR *sp;

	TAILQ_REMOVE(&wp->gp->dq, wp, q);

	if (wp->ccl_sp != NULL) {
		(void)file_end(wp->ccl_sp, NULL, 1);
		(void)screen_end(wp->ccl_sp);
	}
	while ((sp = TAILQ_FIRST(&wp->scrq)) != NULL)
		(void)screen_end(sp);

	/* Free key input queue. */
	if (wp->i_event != NULL)
		free(wp->i_event);

	/* Free cut buffers. */
	cut_close(wp);

	/* Free default buffer storage. */
	(void)text_lfree(&wp->dcb_store.textq);

#if defined(DEBUG) || defined(PURIFY) || defined(LIBRARY)
	/* Free any temporary space. */
	if (wp->tmp_bp != NULL)
		free(wp->tmp_bp);
#endif

	return 0;
}

/*
 * gs_end --
 *	End the program, discarding screens and most of the global area.
 *
 * PUBLIC: void gs_end __P((GS *));
 */
void
gs_end(GS *gp)
{
	MSGS *mp;
	SCR *sp;
	WIN *wp;

	/* If there are any remaining screens, kill them off. */
	while ((wp = TAILQ_FIRST(&gp->dq)) != NULL)
		(void)win_end(wp);
	while ((sp = TAILQ_FIRST(&gp->hq)) != NULL)
		(void)screen_end(sp);

#ifdef HAVE_PERL_INTERP
	perl_end(gp);
#endif

#if defined(DEBUG) || defined(PURIFY) || defined(LIBRARY)
	{ FREF *frp;
		/* Free FREF's. */
		while ((frp = TAILQ_FIRST(&gp->frefq)) != NULL) {
			TAILQ_REMOVE(&gp->frefq, frp, q);
			if (frp->name != NULL)
				free(frp->name);
			if (frp->tname != NULL)
				free(frp->tname);
			free(frp);
		}
	}

	/* Free map sequences. */
	seq_close(gp);

	/* Close message catalogs. */
	msg_close(gp);
#endif

	/* Ring the bell if scheduled. */
	if (F_ISSET(gp, G_BELLSCHED))
		(void)fprintf(stderr, "\07");		/* \a */

	/*
	 * Flush any remaining messages.  If a message is here, it's almost
	 * certainly the message about the event that killed us (although
	 * it's possible that the user is sourcing a file that exits from the
	 * editor).
	 */
	while ((mp = LIST_FIRST(&gp->msgq)) != NULL) {
		(void)fprintf(stderr, "%s%.*s",
		    mp->mtype == M_ERR ? "ex/vi: " : "", (int)mp->len, mp->buf);
		LIST_REMOVE(mp, q);
#if defined(DEBUG) || defined(PURIFY) || defined(LIBRARY)
		free(mp->buf);
		free(mp);
#endif
	}

#if defined(TRACE)
	/* Close tracing file descriptor. */
	vtrace_end();
#endif
}


/*
 * perr --
 *	Print system error.
 */
static void
perr(char *name, char *msg)
{
	(void)fprintf(stderr, "%s:", name);
	if (msg != NULL)
		(void)fprintf(stderr, "%s:", msg);
	(void)fprintf(stderr, "%s\n", strerror(errno));
	exit(1);
}
