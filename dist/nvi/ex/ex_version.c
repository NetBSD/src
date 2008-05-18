/*-
 * Copyright (c) 1991, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 * Copyright (c) 1991, 1993, 1994, 1995, 1996
 *	Keith Bostic.  All rights reserved.
 *
 * See the LICENSE file for redistribution information.
 */

#include "config.h"

#ifndef lint
static const char sccsid[] = "$Id: ex_version.c,v 1.1.1.1.2.2 2008/05/18 12:29:29 yamt Exp $ (Berkeley) $Date: 2008/05/18 12:29:29 $";
#endif /* not lint */

#include <sys/types.h>
#include <sys/queue.h>

#include <bitstring.h>
#include <limits.h>
#include <stdio.h>

#include "../common/common.h"
#include "version.h"

/*
 * ex_version -- :version
 *	Display the program version.
 *
 * PUBLIC: int ex_version __P((SCR *, EXCMD *));
 */
int
ex_version(SCR *sp, EXCMD *cmdp)
{
	msgq(sp, M_INFO, "Version "VI_VERSION
			 " The CSRG, University of California, Berkeley.");
	return (0);
}
