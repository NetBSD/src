/*	$NetBSD: pvctxctl.c,v 1.3 2001/05/07 14:00:23 kleink Exp $	*/

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
 * ALTQ Id: pvctxctl.c,v 0.4 1999/05/19 11:31:11 kjc Exp
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <err.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include <net/if_atm.h>

static int str2vc(char *str, int *vpi, int *vci);
static void usage(void);

static void 
usage(void)
{
	fprintf(stderr, "usage: pvctxctl interface [vpi:]vci\n");
	fprintf(stderr, "          [-p pcr] [-b pcr_in_bps] [-j [vpi:]vci\n");
	fprintf(stderr, "          [-n] \n");
	exit(1);
}

int vpi = 0;
int vci = 0;
int joint_vpi = 0;
int joint_vci = 0;
int pcr = 0;
int llcsnap = ATM_PH_LLCSNAP;
int getinfo = 1;
int subinterface = 0;
int verbose = 1;

int 
main(int argc, char **argv)
{
	struct pvctxreq pvcreq;
	int s, ch;
	long bandwidth;
	char *if_name, *cp;

	if (argc < 2)
		usage();

	if_name = argv[1];
	if (argc > 2 && isdigit(argv[2][0]))
		str2vc(argv[2], &vpi, &vci);
    
	optind = 3;
	while ((ch = getopt(argc, argv, "p:b:j:snv")) != -1) {
		switch (ch) {
		case 'p':
			pcr = strtol(optarg, NULL, 0);
			getinfo = 0;
			break;
		case 'b':
			cp = NULL;
			bandwidth = strtol(optarg, &cp, 0);
			if (cp != NULL) {
				if (*cp == 'K' || *cp == 'k')
					bandwidth *= 1000;
				if (*cp == 'M' || *cp == 'm')
					bandwidth *= 1000000;
			}
			pcr = bandwidth / 8 / 48;
			getinfo = 0;
			break;
		case 'j':
			str2vc(optarg, &joint_vpi, &joint_vci);
			break;
		case 'n':
			llcsnap = 0;
			break;
		case 'v':
			verbose = 1;
			break;
		default:
			usage();
		}
	}

	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		err(1, "can't open socket");

	pvcreq.pvc_ifname[IFNAMSIZ-1] = '\0';
	strncpy(pvcreq.pvc_ifname, if_name, IFNAMSIZ-1);

	if (strncmp(if_name, "pvc", 3) == 0)
		/* pvc subinterface */
		subinterface = 1;
	
	ATM_PH_FLAGS(&pvcreq.pvc_aph) = ATM_PH_AAL5 | llcsnap;
	ATM_PH_VPI(&pvcreq.pvc_aph) = vpi;
	ATM_PH_SETVCI(&pvcreq.pvc_aph, vci);

	ATM_PH_FLAGS(&pvcreq.pvc_joint) = 0;
	ATM_PH_VPI(&pvcreq.pvc_joint) = joint_vpi;
	ATM_PH_SETVCI(&pvcreq.pvc_joint, joint_vci);
	
	pvcreq.pvc_pcr = pcr;

	if (getinfo) {
		if (ioctl(s, SIOCGPVCTX, &pvcreq) < 0)
			err(1, "SIOCSPVCTX");
	}
	else {
		if (verbose) {
		    printf("setting pvc tx: interface:%s vc:%d:%d ph=0x%x\n",
			   if_name, vpi, vci, ATM_PH_FLAGS(&pvcreq.pvc_aph));
		    printf("  joint:%d:%d, setting pcr:%d\n",
			   joint_vpi, joint_vci, pcr);
	        }

		if (ioctl(s, SIOCSPVCTX, &pvcreq) < 0)
				err(1, "SIOCSPVCTX");
	}

	pcr = pvcreq.pvc_pcr;

	/*
	 * print info
	 */
	printf("  %s", if_name);
	if (subinterface)
		printf(" (bound to %s)", pvcreq.pvc_ifname);
	printf(": vci:[%d:%d] (",
	       ATM_PH_VPI(&pvcreq.pvc_aph), ATM_PH_VCI(&pvcreq.pvc_aph));
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
		printf("pcr:%d(%dbps)\n", pcr, pcr * 48 * 8);
	else if (pcr < 1000000)
		printf("pcr:%d(%dKbps)\n", pcr, pcr * 48 * 8 / 1000);
	else
		printf("pcr:%d(%dMbps)\n", pcr, pcr * 48 * 8 / 1000000);
    
	close(s);

	if (getinfo && pcr < 0) {
		fprintf(stderr, "can't get pvc info for vci:%d\n", vci);
		fprintf(stderr, "to setup a vci, use -p or -b option\n");
	}

	return (0);
}

static int 
str2vc(char *str, int *vpi, int *vci)
{
	char *c;

	if ((c = strchr(str, ':')) != NULL) {
		*c = '\0';
		*vpi = strtol(str, NULL, 0);
		str = c + 1;
	}
	else
		*vpi = 0;

	*vci = strtol(str, NULL, 0);
	return (0);
}

