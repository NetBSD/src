/* $NetBSD: signalname.c,v 1.1.2.2 2017/05/11 02:58:32 pgoyette Exp $ */

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
 * signalname()
 *
 *	Converts the signal number "sig" to its
 *	signal name (without the "SIG" prefix).
 *
 * Returns:
 *	NULL on error (invalid signal number)
 *	otherwise the (abbreviated) signal name (no "SIG").
 */

const char *
signalname(int sig)
{

	if (sig <= 0 || sig >= NSIG)
		return NULL;

	return sys_signame[sig];
}
