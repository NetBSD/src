/* $NetBSD: signalnext.c,v 1.1 2017/05/09 11:14:16 kre Exp $ */

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

#include <sys/types.h>

#include <signal.h>
#include <string.h>

/*
 * signalnext()
 *
 *	Returns the next signal number after the one given.
 *	Giving 0 as 'sig' requests the lowest available signal number.
 *
 * Returns:
 *	-1 on error (invalid 'sig' arg)
 *	0  when there is no next.
 *	otherwise the next greater implemented signal number after 'sig'
 */

int
signalnext(int sig)
{

	if (sig < 0 || sig >= NSIG || (sig > 0 && sys_signame[sig] == NULL))
		return -1;

	while (++sig < NSIG)
		if (sys_signame[sig] != NULL)
			return sig;

	return 0;
}
