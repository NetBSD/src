/*
 *	globals.c:
 *
 *	global variables should be separate, so nothing else
 *	must be included extraneously.
 *
 *	$Id: globals.c,v 1.2 1993/10/13 21:22:28 cgd Exp $
 */

#include <sys/param.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>

#include "netboot.h"

u_char	bcea[6] = BA;			/* broadcast ethernet address */
char	rootpath[FNAME_SIZE];		/* root mount path */
char	swappath[FNAME_SIZE];		/* swap mount path */
char	ifname[IFNAME_SIZE];		/* name of interface (e.g. "le0") */
n_long	rootip;				/* root ip address */
n_long	swapip;				/* swap ip address */
n_long	gateip;				/* swap ip address */
n_long	smask;				/* subnet mask */
n_long	nmask;				/* net mask */
n_long	mask;				/* subnet or net mask */
time_t	bot;				/* beginning of time in seconds */
