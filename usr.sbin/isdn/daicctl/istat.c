/* $NetBSD: istat.c,v 1.3 2002/04/14 11:41:43 martin Exp $ */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Martin Husemann <martin@netbsd.org>.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <netisdn/i4b_ioctl.h>

#define LOG                     1
#define MEMR                    2
#define MEMW                    3
#define IOR                     4
#define IOW                     5
#define B1TEST                  6
#define B2TEST                  7
#define BTESTOFF                8
#define DSIG_STATS              9
#define B_CH_STATS              10
#define D_CH_STATS              11
#define BL1_STATS               12
#define BL1_STATS_C             13
#define DVERSION                14

#define OK			0xff
#define MORE_EVENTS		0xfe
#define NO_EVENT		1

struct DSigStruc
{
  u_int8_t rc;
  u_int8_t Id;
  u_int8_t u;
  u_int8_t listen;
  u_int8_t active;
  u_int8_t sin[3];
  u_int8_t bc[6];
  u_int8_t llc[6];
  u_int8_t hlc[6];
  u_int8_t oad[20];
};

struct BL1Struc {
  u_int8_t rc;
  u_int32_t cx_b1;
  u_int32_t cx_b2;
  u_int32_t cr_b1;
  u_int32_t cr_b2;
  u_int32_t px_b1;
  u_int32_t px_b2;
  u_int32_t pr_b1;
  u_int32_t pr_b2;
  u_int16_t er_b1;
  u_int16_t er_b2;
};

struct L2Struc {
  u_int8_t rc;
  u_int32_t XTotal;
  u_int32_t RTotal;
  u_int16_t XError;
  u_int16_t RError;
};

static void printIE(u_int8_t *ie);
void istat(int fd, int controller);
void xversion(int fd, int controller);

void
istat(int fd, int controller)
{
	struct isdn_diagnostic_request req;
	printf("istat:\n");
	memset(&req, 0, sizeof(req));

	req.controller = controller;

	/* dump d-channel signaling tasks */
	{
		struct DSigStruc r;
		int ok = 1;
		req.cmd = DSIG_STATS;
		req.out_param_len = sizeof(r);
		req.out_param = &r;

		printf("D-Channel Signaling Entities\n");
		printf("============================\n");

		while (ok) {
			if (ioctl(fd, I4B_ACTIVE_DIAGNOSTIC, &req) == -1) {
				perror("ioctl(I4B_ACTIVE_DIAGNOSTIC)");
				return;
			}
			if (r.rc == OK)	/* last entry */
				ok = 0;

			if (r.Id) {
				switch(r.sin[1]) {
				case 0:
				  printf("Any Service Task");
				  break;
				case 1:
				  printf("Voice Task");
				  break;
				case 2:
				  printf("a/b Task");
				  break;
				case 3:
				  printf("X.21 Task");
				  break;
				case 4:
				  printf("Fax G4 Task");
				  break;
				case 5:
				  printf("Videotex Task");
				  break;
				case 7:
				  printf("Transparent Data Task");
				  break;
				case 9:
				  printf("Teletex 64 Task");
				  break;
				}
				printf(", Id = %02X, State = %i\n",
				       r.Id,r.u);
				printf("\t");
				printf(" BC =");
				printIE(r.bc);
				printf(" LLC =");
				printIE(r.llc);
				printf(" HLC =");
				printIE(r.hlc);
				printf("\n");
			}
		}
	}
}

static void
printIE(ie)
	u_int8_t *ie;
{
	int i;

	for(i = 0; i < ie[0]; i++)
		printf(" %02X", ie[i+1]);
}

void
xversion(int fd, int controller)
{
	struct isdn_diagnostic_request req;
	char rcversion[50];
	memset(&req, 0, sizeof(req));

	req.controller = controller;
	req.cmd = DVERSION;
	req.out_param_len = 49;
	req.out_param = &rcversion;

	if (ioctl(fd, I4B_ACTIVE_DIAGNOSTIC, &req) == -1) {
		perror("");
		return;
	}
	rcversion[49] = '\0';
	printf("controller %d is running '%s'\n", controller, rcversion+1);
}
