/*	$NetBSD: trygetea.c,v 1.5 2007/03/10 00:16:51 hubertf Exp $	*/

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: trygetea.c,v 1.5 2007/03/10 00:16:51 hubertf Exp $");
#endif

/*
 * trygetea.c - test program for getether.c
 */

#include <sys/types.h>
#include <sys/socket.h>

#if defined(SUNOS) || defined(SVR4)
#include <sys/sockio.h>
#endif

#include <net/if.h>				/* for struct ifreq */
#include <netinet/in.h>
#include <arpa/inet.h>			/* inet_ntoa */

#include <netdb.h>
#include <stdio.h>
#include <errno.h>

int
main(int argc, char **argv)
{
	u_char ea[16];				/* Ethernet address */
	int i;

	if (argc < 2) {
		fprintf(stderr, "need interface name\n");
		exit(1);
	}
	if ((i = getether(argv[1], ea)) < 0) {
		fprintf(stderr, "Could not get Ethernet address (rc=%d)\n", i);
		exit(1);
	}
	printf("Ether-addr");
	for (i = 0; i < 6; i++)
		printf(":%x", ea[i] & 0xFF);
	printf("\n");

	return 0;
}
