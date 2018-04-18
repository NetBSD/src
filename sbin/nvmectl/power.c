/*	$NetBSD: power.c,v 1.4 2018/04/18 10:11:44 nonaka Exp $	*/

/*-
 * Copyright (c) 2016 Netflix, Inc
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: power.c,v 1.4 2018/04/18 10:11:44 nonaka Exp $");
#if 0
__FBSDID("$FreeBSD: head/sbin/nvmecontrol/power.c 329824 2018-02-22 13:32:31Z wma $");
#endif
#endif

#include <sys/param.h>
#include <sys/ioccom.h>

#include <ctype.h>
#include <err.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "nvmectl.h"

__dead static void
power_usage(void)
{
	fprintf(stderr, "usage:\n");
	fprintf(stderr, "\t%s " POWER_USAGE, getprogname());
	exit(1);
}

static void
power_list_one(int i, struct nvm_identify_psd *psd)
{
	int mpower, apower, ipower;

	mpower = psd->mp;
	if (!(psd->flags & NVME_PSD_MPS))
		mpower *= 100;
	ipower = psd->idlp;
	if (__SHIFTOUT(psd->ips, NVME_PSD_IPS_MASK) == 1)
		ipower *= 100;
	apower = psd->actp;
	if (__SHIFTOUT(psd->ap, NVME_PSD_APS_MASK) == 1)
		apower *= 100;
	printf("%2d: %2d.%04dW%c %3d.%03dms %3d.%03dms %2d %2d %2d %2d %2d.%04dW %2d.%04dW %d\n",
	       i, mpower / 10000, mpower % 10000,
	       (psd->flags & NVME_PSD_NOPS) ? '*' : ' ',
	       psd->enlat / 1000, psd->enlat % 1000,
	       psd->exlat / 1000, psd->exlat % 1000,
	       (uint8_t)(psd->rrt & NVME_PSD_RRT_MASK),
	       (uint8_t)(psd->rrl & NVME_PSD_RRL_MASK),
	       (uint8_t)(psd->rwt & NVME_PSD_RWT_MASK),
	       (uint8_t)(psd->rwl & NVME_PSD_RWL_MASK),
	       ipower / 10000, ipower % 10000, apower / 10000, apower % 10000,
	       (uint16_t)__SHIFTOUT(psd->ap, NVME_PSD_APW_MASK));
}

static void
power_list(struct nvm_identify_controller *cdata)
{
	int i;

	printf("\nPower States Supported: %d\n\n", cdata->npss + 1);
	printf(" #   Max pwr  Enter Lat  Exit Lat RT RL WT WL Idle Pwr  Act Pwr Workloadd\n");
	printf("--  --------  --------- --------- -- -- -- -- -------- -------- --\n");
	for (i = 0; i <= cdata->npss; i++)
		power_list_one(i, &cdata->psd[i]);
}

static void
power_set(int fd, int power_val, int workload, int perm)
{
	struct nvme_pt_command	pt;
	uint32_t p;

	p = perm ? (1u << 31) : 0;
	memset(&pt, 0, sizeof(pt));
	pt.cmd.opcode = NVM_ADMIN_SET_FEATURES;
	pt.cmd.cdw10 = NVM_FEAT_POWER_MANAGEMENT | p;
	pt.cmd.cdw11 = power_val | (workload << 5);

	if (ioctl(fd, NVME_PASSTHROUGH_CMD, &pt) < 0)
		err(1, "set feature power mgmt request failed");

	if (nvme_completion_is_error(&pt.cpl))
		errx(1, "set feature power mgmt request returned error");
}

static void
power_show(int fd)
{
	struct nvme_pt_command	pt;

	memset(&pt, 0, sizeof(pt));
	pt.cmd.opcode = NVM_ADMIN_GET_FEATURES;
	pt.cmd.cdw10 = NVM_FEAT_POWER_MANAGEMENT;

	if (ioctl(fd, NVME_PASSTHROUGH_CMD, &pt) < 0)
		err(1, "set feature power mgmt request failed");

	if (nvme_completion_is_error(&pt.cpl))
		errx(1, "set feature power mgmt request returned error");

	printf("Current Power Mode is %d\n", pt.cpl.cdw0);
}

void
power(int argc, char *argv[])
{
	struct nvm_identify_controller	cdata;
	int				ch, listflag = 0, powerflag = 0, power_val = 0, fd;
	int				workload = 0;
	char				*end;

	while ((ch = getopt(argc, argv, "lp:w:")) != -1) {
		switch (ch) {
		case 'l':
			listflag = 1;
			break;
		case 'p':
			powerflag = 1;
			power_val = strtol(optarg, &end, 0);
			if (*end != '\0') {
				fprintf(stderr, "Invalid power state number: %s\n", optarg);
				power_usage();
			}
			break;
		case 'w':
			workload = strtol(optarg, &end, 0);
			if (*end != '\0') {
				fprintf(stderr, "Invalid workload hint: %s\n", optarg);
				power_usage();
			}
			break;
		default:
			power_usage();
		}
	}

	/* Check that a controller was specified. */
	if (optind >= argc)
		power_usage();

	if (listflag && powerflag) {
		fprintf(stderr, "Can't set power and list power states\n");
		power_usage();
	}

	open_dev(argv[optind], &fd, 1, 1);
	read_controller_data(fd, &cdata);

	if (listflag) {
		power_list(&cdata);
		goto out;
	}

	if (powerflag) {
		power_set(fd, power_val, workload, 0);
		goto out;
	}
	power_show(fd);

out:
	close(fd);
	exit(0);
}
