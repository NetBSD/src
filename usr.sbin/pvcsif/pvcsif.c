/*	$NetBSD: pvcsif.c,v 1.3 2001/05/07 14:00:23 kleink Exp $	*/

/*
 * Copyright (C) 1998
 *	Sony Computer Science Laboratory Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY SONY CSL AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL SONY CSL OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * ALTQ Id: pvcsif.c,v 0.3 1999/05/19 11:31:11 kjc Exp
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <err.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include <net/if_atm.h>

void usage __P((void));
void list_all __P((void));

void usage(void)
{
	fprintf(stderr, "usage: pvcsif -a\n");
	fprintf(stderr, "       pvcsif interface [-s]\n");
	exit(1);
}

int 
main(int argc, char **argv)
{
	struct ifreq ifr;
	int shell_mode = 0;
	int s, ch;

	if (argc < 2)
		usage();

	if (strncmp(argv[1], "-a", 2) == 0) {
		list_all();
		return (0);
	}

	ifr.ifr_name[IFNAMSIZ-1] = '\0';
	strncpy(ifr.ifr_name, argv[1], IFNAMSIZ-1);

	optind = 2;
	while ((ch = getopt(argc, argv, "s")) != -1) {
		switch (ch) {
		case 's':
			shell_mode = 1;
			break;
		}
	}

	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		err(1, "can't open socket");

	if (ioctl(s, SIOCSPVCSIF, &ifr) < 0)
		err(1, "SIOCSPVCSIF");

	close(s);

	if (shell_mode)
		printf("%s", ifr.ifr_name);
	else
		printf("created a pvc subinterface %s (bound to %s)\n",
		       ifr.ifr_name, argv[1]);
    
	return (0);
}

void
list_all(void)
{
	struct if_nameindex *ifn_list, *ifnp;
	struct pvctxreq pvcreq;
	int pcr, s;

	if ((ifn_list = if_nameindex()) == NULL)
		err(1, "if_nameindex failed");

	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		err(1, "can't open socket");

	for (ifnp = ifn_list; ifnp->if_name != NULL; ifnp++) {
		if (strncmp(ifnp->if_name, "pvc", 3) == 0) {

			bzero(&pvcreq, sizeof(pvcreq));
			strncpy(pvcreq.pvc_ifname, ifnp->if_name, IFNAMSIZ-1);
			if (ioctl(s, SIOCGPVCTX, &pvcreq) < 0)
				err(1, "SIOCSPVCTX");

			/*
			 * print info
			 */
			pcr = pvcreq.pvc_pcr;
			printf("  %s (bound to %s): vci:[%d:%d] (",
			       ifnp->if_name, pvcreq.pvc_ifname,
			       ATM_PH_VPI(&pvcreq.pvc_aph),
			       ATM_PH_VCI(&pvcreq.pvc_aph));
			if (ATM_PH_FLAGS(&pvcreq.pvc_aph) & ATM_PH_AAL5)
				printf("AAL5");
			if (ATM_PH_FLAGS(&pvcreq.pvc_aph) & ATM_PH_LLCSNAP)
				printf("/LLCSNAP");
			printf(") ");
			if (pcr < 0)
				printf("(invalid)\n");
			else if (pcr == 0)
				printf("pcr:%d(full speed)\n", pcr);
			else if (pcr < 1000)
				printf("pcr:%d(%dbps)\n",
				       pcr, pcr * 48 * 8);
			else if (pcr < 1000000)
				printf("pcr:%d(%dKbps)\n",
				       pcr, pcr * 48 * 8 / 1000);
			else
				printf("pcr:%d(%dMbps)\n",
				       pcr, pcr * 48 * 8 / 1000000);
		}
	}

	close(s);
	if_freenameindex(ifn_list);
}
