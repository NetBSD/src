/* $NetBSD: signalnumber.c,v 1.3 2020/08/20 22:56:56 kre Exp $ */

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
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

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
#ifndef SMALL
	long offs;
	char *ep;
#endif

	if (strncasecmp(name, "sig", 3) == 0)
		name += 3;

	for (i = 1; i < NSIG; ++i)
		if (sys_signame[i] != NULL &&
		    strcasecmp(name, sys_signame[i]) == 0)
			return i;

#ifndef SMALL
	if (strncasecmp(name, "rtm", 3) == 0) {
		name += 3;
		if (strncasecmp(name, "ax", 2) == 0)
			i = SIGRTMAX;
		else if (strncasecmp(name, "in", 2) == 0)
			i = SIGRTMIN;
		else
			return 0;
		name += 2;
		if (name[0] == '\0')
			return i;
		if (i == SIGRTMAX && name[0] != '-')
			return 0;
		if (i == SIGRTMIN && name[0] != '+')
			return 0;
		if (!isdigit((unsigned char)name[1]))
			return 0;
		offs = strtol(name+1, &ep, 10);
		if (ep == name+1 || *ep != '\0' ||
		    offs < 0 || offs > SIGRTMAX-SIGRTMIN)
			return 0;
		if (name[0] == '+')
			i += (int)offs;
		else
			i -= (int)offs;
		if (i >= SIGRTMIN && i <= SIGRTMAX)
			return i;
	}
#endif
	return 0;
}
