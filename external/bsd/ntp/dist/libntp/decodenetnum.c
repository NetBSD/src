/*	$NetBSD: decodenetnum.c,v 1.1.1.1 2009/12/13 16:55:02 kardel Exp $	*/

/*
 * decodenetnum - return a net number (this is crude, but careful)
 */
#include <config.h>
#include <sys/types.h>
#include <ctype.h>
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#include "ntp_stdlib.h"
#include "ntp_assert.h"

int
decodenetnum(
	const char *num,
	sockaddr_u *netnum
	)
{
	struct addrinfo hints, *ai = NULL;
	register int err;
	register const char *cp;
	char name[80];
	char *np;

	NTP_REQUIRE(num != NULL);
	NTP_REQUIRE(strlen(num) < sizeof(name));

	if ('[' != num[0]) 
		cp = num;
	else {
		cp = num + 1;
		np = name; 
		while (*cp && ']' != *cp)
			*np++ = *cp++;
		*np = 0;
		cp = name; 
	}
	memset(&hints, 0, sizeof(hints));
	hints.ai_flags = AI_NUMERICHOST;
	err = getaddrinfo(cp, NULL, &hints, &ai);
	if (err != 0)
		return 0;
	memcpy(netnum, ai->ai_addr, ai->ai_addrlen); 
	freeaddrinfo(ai);
	return 1;
}
