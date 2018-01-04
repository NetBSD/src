/* $NetBSD: signalnumber.c,v 1.2 2018/01/04 20:57:29 kamil Exp $ */

/*
 * Software available to all and sundry without limitations
 * but without warranty of fitness for any purpose whatever.
 *
 * Licensed for any use, including redistribution in source
 * and binary forms, with or without modifications, subject
 * the following agreement:
 *
 * Licensee agrees to indemnify licensor, and distributor, for
 * the full amount of any any claim made by the licensee against
 * the licensor or distributor, for any action that results from
 * any use or redistribution of this software, plus any costs
 * incurred by licensor or distributor resulting from that claim.
 *
 * This licence must be retained with the software.
 */

#include "namespace.h"

#include <signal.h>
#include <string.h>

/*
 * signalnumber()
 *
 *	Converts the signal named "name" to its
 *	signal number.
 *
 *	"name" can be the signal name with or without
 *	the leading "SIG" prefix, and character comparisons
 *	are done in a case independent manner.
 *	(ie: SIGINT == INT == int == Int == SiGiNt == (etc).)
 *
 * Returns:
 *	0 on error (invalid signal name)
 *	otherwise the signal number that corresponds to "name"
 */

int
signalnumber(const char *name)
{
	int i;

	if (strncasecmp(name, "sig", 3) == 0)
		name += 3;

	for (i = 1; i < NSIG; ++i)
		if (sys_signame[i] != NULL &&
		    strcasecmp(name, sys_signame[i]) == 0)
			return i;
	return 0;
}
