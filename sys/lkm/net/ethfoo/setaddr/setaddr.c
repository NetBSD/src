/*	$NetBSD: setaddr.c,v 1.1.4.4 2004/09/21 13:36:24 skrll Exp $	*/

/*
 *  Copyright (c) 2003, Quentin Garnier.  All rights reserved.
 * 
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 
 *  THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 *  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 *  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/sockio.h>

#include <net/if.h>
#include <net/if_ether.h>

#include <stdio.h>

void
usage()
{
	(void)fprintf(stderr, "usage: %s interface address\n", getprogname());
	(void)fprintf(stderr, "example: %s ethfoo2 01:23:45:67:89:ab\n", getprogname());
	exit(1);
}

int
main(int argc, char *argv[])
{
	int s;
	struct ifaliasreq ifra;
	struct sockaddr *sa;

	setprogname(argv[0]);

	/* Usage: setaddr <iface> <ethaddr> */
	if (argc != 3)
		usage();

	s = socket(AF_INET, SOCK_DGRAM, 0);

	if (s < 0)
		err(1, "opening socket");

	strlcpy(ifra.ifra_name, argv[1], IFNAMSIZ-1);
	ifra.ifra_name[IFNAMSIZ-1] = 0;

	sa = (void *)&ifra.ifra_addr;
	sa->sa_family = AF_LINK;
	sa->sa_len = sizeof(struct sockaddr);

	memcpy(&sa->sa_data, ether_aton(argv[2]), ETHER_ADDR_LEN);

	if (ioctl(s, SIOCSIFPHYADDR, &ifra) < 0)
		err(1, "SIOCSIFPHYADDR");

	close(s);
	exit(0);
}
