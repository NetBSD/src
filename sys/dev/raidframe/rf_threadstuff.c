/*	$NetBSD: rf_threadstuff.c,v 1.17 2003/12/29 05:36:19 oster Exp $	*/
/*
 * rf_threadstuff.c
 */
/*
 * Copyright (c) 1995 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Jim Zelenka
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rf_threadstuff.c,v 1.17 2003/12/29 05:36:19 oster Exp $");

#include <dev/raidframe/raidframevar.h>

#include "rf_threadstuff.h"
#include "rf_general.h"
#include "rf_shutdown.h"

/*
 * Shared stuff
 */

#if 0
int 
_rf_create_managed_lkmgr_mutex(listp, m, file, line)
	RF_ShutdownList_t **listp;
RF_DECLARE_LKMGR_MUTEX(*m)
	char   *file;
	int     line;
{
	int     rc, rc1;

	rc = rf_lkmgr_mutex_init(m);
	if (rc)
		return (rc);
	rc = _rf_ShutdownCreate(listp, mutex_destroyer, (void *) m, file, line);
	if (rc) {
		RF_ERRORMSG1("RAIDFRAME: Error %d adding shutdown entry\n", rc);
		rc1 = rf_lkmgr_mutex_destroy(m);
		if (rc1) {
			RF_ERRORMSG1("RAIDFRAME: Error %d destroying mutex\n", rc1);
		}
	}
	return (rc);
}
#endif
int 
_rf_create_managed_cond(listp, c, file, line)
	RF_ShutdownList_t **listp;
RF_DECLARE_COND(*c)
	char   *file;
	int     line;
{

	c = 0;
	return (0);
}

int 
_rf_init_managed_threadgroup(listp, g, file, line)
	RF_ShutdownList_t **listp;
	RF_ThreadGroup_t *g;
	char   *file;
	int     line;
{
	int     rc;

	rf_mutex_init(&g->mutex);
	rc = _rf_create_managed_cond(listp, &g->cond, file, line);
	if (rc)
		return (rc);
	g->created = g->running = g->shutdown = 0;
	return (0);
}

/*
 * Kernel
 */
int
rf_lkmgr_mutex_init(m)
decl_lock_data(, *m)
{
	lockinit(m,PRIBIO,"RAIDframe lock",0,0);
	return(0);
}

#if 0
int
rf_lkmgr_mutex_destroy(m)
decl_lock_data(, *m)
{
	return(0);
}
#endif
