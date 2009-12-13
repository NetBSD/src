/*	$NetBSD: libntpq_subs.c,v 1.1.1.1 2009/12/13 16:56:26 kardel Exp $	*/

/*****************************************************************************
 *
 *  libntpq_subs.c
 *
 *  This is the second part of the wrapper library for ntpq, the NTP query utility. 
 *  This library reuses the sourcecode from ntpq and exports a number
 *  of useful functions in a library that can be linked against applications
 *  that need to query the status of a running ntpd. The whole 
 *  communcation is based on mode 6 packets.
 *
 *  This source file exports the (private) functions from ntpq-subs.c 
 *
 ****************************************************************************/


#define _LIBNTPQSUBSC
#include "ntpq-subs.c"
#include "libntpq.h"

/* Function Prototypes */
int ntpq_dogetassoc(void);
char ntpq_decodeaddrtype(sockaddr_u *sock);
int ntpq_doquerylist(struct varlist *, int , int , int , u_short *, int *, char **datap );


/* the following functions are required internally by a number of libntpq functions 
 * and since they are defined as static in ntpq-subs.c, they need to be exported here
 */
 
int ntpq_dogetassoc(void)
{
	
	if ( dogetassoc(NULL))
	{
		return numassoc;
	} else {
		return 0;
	}
}

char ntpq_decodeaddrtype(sockaddr_u *sock)
{
	return decodeaddrtype(sock);
}

int ntpq_doquerylist(struct varlist *vlist, int op, int associd, int auth, u_short *rstatus, int *dsize, char **datap )
{
    return doquerylist(vlist, op, associd, auth, rstatus, dsize,  &*datap );
}

