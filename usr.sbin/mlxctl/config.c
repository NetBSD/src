/*	$NetBSD: config.c,v 1.1.4.1 2003/07/28 18:14:00 he Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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

/*-
 * Copyright (c) 1999 Michael Smith
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
 *
 * from FreeBSD: config.c,v 1.2 2000/04/11 23:04:17 msmith Exp
 */

#ifndef lint
#include <sys/cdefs.h>
__RCSID("$NetBSD: config.c,v 1.1.4.1 2003/07/28 18:14:00 he Exp $");
#endif /* not lint */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/queue.h>

#include <dev/ic/mlxreg.h>
#include <dev/ic/mlxio.h>

#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <getopt.h>

#include "extern.h"

struct conf_phys_drv {
	TAILQ_ENTRY(conf_phys_drv)	pd_link;
	int				pd_bus;
	int				pd_target;
	struct mlx_phys_drv		pd_drv;
};

struct conf_span {
	TAILQ_ENTRY(conf_span)		s_link;
	struct conf_phys_drv		*s_drvs[8];
	struct mlx_sys_drv_span		s_span;
};

struct conf_sys_drv {
	TAILQ_ENTRY(conf_sys_drv)	sd_link;
	struct conf_span		*sd_spans[4];
	struct mlx_sys_drv		sd_drv;
};

struct conf_config {
	TAILQ_HEAD(,conf_phys_drv)	cc_phys_drvs;
	TAILQ_HEAD(,conf_span)		cc_spans;
	TAILQ_HEAD(,conf_sys_drv)	cc_sys_drvs;
	struct conf_sys_drv		*cc_drives[32];
	struct mlx_core_cfg		cc_cfg;
};

static void	print_span(struct mlx_sys_drv_span *, int);
static void	print_sys_drive(struct conf_config *, int);
static void	print_phys_drive(struct conf_config *, int, int);

/*
 * Get the configuration from the selected controller.
 *
 * config <controller>
 *		Print the configuration for <controller>
 *
 * XXX update to support adding/deleting drives.
 */
int
cmd_config(char **argv) 
{
	char hostname[MAXHOSTNAMELEN];
	struct conf_config conf;
	int i, j;

	if (ci.ci_firmware_id[0] < 3) {
		warnx("action not supported by this firmware version");
		return (1);
	}

	memset(&conf.cc_cfg, 0, sizeof(conf.cc_cfg));
	mlx_configuration(&conf.cc_cfg, 0);

	gethostname(hostname, sizeof(hostname));
	printf("# controller %s on %s\n", mlxname, hostname);

	printf("#\n# physical devices connected:\n");
	for (i = 0; i < 5; i++)
		for (j = 0; j < 16; j++)
			print_phys_drive(&conf, i, j);

	printf("#\n# system drives defined:\n");
	for (i = 0; i < conf.cc_cfg.cc_num_sys_drives; i++)
		print_sys_drive(&conf, i);

	return(0);
}

/*
 * Print details for the system drive (drvno) in a format that we will be
 * able to parse later.
 *
 * drive?? <raidlevel> <writemode>
 *   span? 0x????????-0x???????? ????blks on <disk> [...]
 *   ...
 */
static void
print_span(struct mlx_sys_drv_span *span, int arms)
{
	int i;

	printf("0x%08x-0x%08x %u blks on", span->sp_start_lba,
	    span->sp_start_lba + span->sp_nblks, span->sp_nblks);

	for (i = 0; i < arms; i++)
		printf(" disk%02d%02d", span->sp_arm[i] >> 4,
		    span->sp_arm[i] & 0x0f);

	printf("\n");
}

static void
print_sys_drive(struct conf_config *conf, int drvno)
{
	struct mlx_sys_drv *drv;
	int i;

	drv = &conf->cc_cfg.cc_sys_drives[drvno];

	printf("drive%02d ", drvno);

	switch(drv->sd_raidlevel & 0xf) {
	case MLX_SYS_DRV_RAID0:
		printf("RAID0");
		break;
	 case MLX_SYS_DRV_RAID1:
		printf("RAID1");
		break;
	case MLX_SYS_DRV_RAID3:
		printf("RAID3");
		break;
	case MLX_SYS_DRV_RAID5:
		printf("RAID5");
		break;
	case MLX_SYS_DRV_RAID6:
		printf("RAID6");
		break;
	case MLX_SYS_DRV_JBOD:
		printf("JBOD");
		break;
	default:
		printf("RAID?");
		break;
	}

	printf(" write%s\n",
	    drv->sd_raidlevel & MLX_SYS_DRV_WRITEBACK ? "back" : "through");

	for (i = 0; i < drv->sd_valid_spans; i++) {
		printf("  span%d ", i);
		print_span(&drv->sd_span[i], drv->sd_valid_arms);
	}
}

/*
 * Print details for the physical drive at chn/targ in a format suitable for
 * human consumption.
 */
static void
print_phys_drive(struct conf_config *conf, int chn, int targ)
{
	struct mlx_phys_drv *pd;

	pd = &conf->cc_cfg.cc_phys_drives[chn * 16 + targ];

	/* If the drive isn't present, don't print it. */
	if ((pd->pd_flags1 & MLX_PHYS_DRV_PRESENT) == 0)
		return;

	mlx_print_phys_drv(pd, chn, targ, "# ");
}
