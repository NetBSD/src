/*
 *	globals.c:
 *
 *	global variables should be separate, so nothing else
 *	must be included extraneously.
 *
 *	$Id: globals.c,v 1.1 1994/05/08 16:11:23 brezak Exp $
 */

#include <sys/param.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>

#include "stand.h"
#include "net.h"

u_char	bcea[6] = BA;			/* broadcast ethernet address */

char	rootpath[FNAME_SIZE] = "/";	/* root mount path */
char	bootfile[FNAME_SIZE];		/* bootp says to boot this */
char	hostname[FNAME_SIZE];		/* our hostname */
char	domainname[FNAME_SIZE];		/* our DNS domain */
char	ifname[IFNAME_SIZE];		/* name of interface (e.g. "le0") */
n_long	nameip;				/* DNS server ip address */
n_long	rootip;				/* root ip address */
n_long	swapip;				/* swap ip address */
n_long	gateip;				/* swap ip address */
n_long	mask = 0xffffff00;		/* subnet or net mask */
