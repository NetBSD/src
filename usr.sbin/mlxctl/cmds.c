/*	$NetBSD: cmds.c,v 1.7 2003/07/13 12:30:17 itojun Exp $	*/

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
 * from FreeBSD: command.c,v 1.2 2000/04/11 23:04:17 msmith Exp
 */

#ifndef lint
#include <sys/cdefs.h>
__RCSID("$NetBSD: cmds.c,v 1.7 2003/07/13 12:30:17 itojun Exp $");
#endif /* not lint */

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/queue.h>
#include <sys/endian.h>

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

static void	cmd_status0(struct mlx_disk *);
static void	cmd_check0(struct mlx_disk *);
static void	cmd_detach0(struct mlx_disk *);

static struct	mlx_rebuild_status rs;
static int	rstatus;

struct {
	int	hwid;
	const char	*name;
} static const mlx_ctlr_names[] = {
	{ 0x00, "960E/960M" },
	{ 0x01,	"960P/PD" },
	{ 0x02,	"960PL" },
	{ 0x10,	"960PG" },
	{ 0x11,	"960PJ" },
	{ 0x12,	"960PR" },
	{ 0x13,	"960PT" }, 
	{ 0x14,	"960PTL0" },
	{ 0x15,	"960PRL" } ,
	{ 0x16,	"960PTL1" },
	{ 0x20,	"1100PVX" },
	{ -1,	NULL },
};

/*
 * Status output
 */
static void
cmd_status0(struct mlx_disk *md)
{
	int result;
 
	result = md->hwunit;
	if (ioctl(mlxfd, MLXD_STATUS, &result) < 0)
		err(EXIT_FAILURE, "ioctl(MLXD_STATUS)");

	switch(result) {
	case MLX_SYSD_ONLINE:
		printf("%s: online\n", md->name);
		break;

	case MLX_SYSD_CRITICAL:
		printf("%s: critical\n", md->name);
		if (!rstatus)
			rstatus = 1;
		break;

	case MLX_SYSD_OFFLINE:
		printf("%s: offline\n", md->name);
		rstatus = 2;
		break;

	default:
		printf("%s: unknown status 0x%02x\n", md->name, result);
		break;
	}

	/* Rebuild/check in progress on this drive? */
	if (rs.rs_drive == md->hwunit &&
	    rs.rs_code != MLX_REBUILDSTAT_IDLE) {
		switch(rs.rs_code) {
		case MLX_REBUILDSTAT_REBUILDCHECK:
			printf(" [consistency check");
			break;

		case MLX_REBUILDSTAT_ADDCAPACITY:
			printf(" [add capacity");
			break;

		case MLX_REBUILDSTAT_ADDCAPACITYINIT:
			printf(" [add capacity init");
			break;

		default:
			printf(" [unknown operation");
			break;
		}

		printf(": %d/%d, %d%% complete]\n", rs.rs_remaining, rs.rs_size,
		   ((rs.rs_size - rs.rs_remaining) / (rs.rs_size / 100)));
	}
}

int
cmd_status(char **argv)
{

	if (ioctl(mlxfd, MLX_REBUILDSTAT, &rs) < 0)
		err(EXIT_FAILURE, "ioctl(MLX_REBUILDSTAT)");

	mlx_disk_iterate(cmd_status0);
	return (rstatus);
}

/*
 * Display controller status.
 */
int
cmd_cstatus(char **argv)
{
	struct mlx_enquiry2 enq;
	struct mlx_phys_drv pd;
	static char buf[80];
	const char *model;
	int i, channel, target;

	for (i = 0; i < sizeof(mlx_ctlr_names) / sizeof(mlx_ctlr_names[0]); i++)
		if (ci.ci_hardware_id == mlx_ctlr_names[i].hwid) {
			model = mlx_ctlr_names[i].name;
			break;
		}

	if (i == sizeof(mlx_ctlr_names) / sizeof(mlx_ctlr_names[0])) {
		snprintf(buf, sizeof(buf), " model 0x%x", ci.ci_hardware_id);
		model = buf;
	}

	printf("DAC%s, %d channel%s, firmware %d.%02d-%c-%02d",
	    model, ci.ci_nchan,
	    ci.ci_nchan > 1 ? "s" : "",
	    ci.ci_firmware_id[0], ci.ci_firmware_id[1],
	    ci.ci_firmware_id[3], ci.ci_firmware_id[2]);
	if (ci.ci_mem_size != 0)
		printf(", %dMB RAM", ci.ci_mem_size >> 20);
	printf("\n");

	if (verbosity > 0 && ci.ci_iftype > 1) {
		mlx_enquiry(&enq);

		printf("  Hardware ID\t\t\t0x%08x\n",
		    le32toh(*(u_int32_t *)enq.me_hardware_id));
		printf("  Firmware ID\t\t\t0x%08x\n",
		    le32toh(*(u_int32_t *)enq.me_firmware_id));
		printf("  Configured/Actual channels\t%d/%d\n",
		    enq.me_configured_channels, enq.me_actual_channels);
		printf("  Max Targets\t\t\t%d\n", enq.me_max_targets);
		printf("  Max Tags\t\t\t%d\n", enq.me_max_tags);
		printf("  Max System Drives\t\t%d\n", enq.me_max_sys_drives);
		printf("  Max Arms\t\t\t%d\n", enq.me_max_arms);
		printf("  Max Spans\t\t\t%d\n", enq.me_max_spans);
		printf("  DRAM/cache/flash/NVRAM size\t%d/%d/%d/%d\n",
		    le32toh(enq.me_mem_size), le32toh(enq.me_cache_size),
		    le32toh(enq.me_flash_size), le32toh(enq.me_nvram_size));
		printf("  DRAM type\t\t\t%d\n", le16toh(enq.me_mem_type));
		printf("  Clock Speed\t\t\t%dns\n",
		    le16toh(enq.me_clock_speed));
		printf("  Hardware Speed\t\t%dns\n",
		    le16toh(enq.me_hardware_speed));
		printf("  Max Commands\t\t\t%d\n",
		    le16toh(enq.me_max_commands));
		printf("  Max SG Entries\t\t%d\n", le16toh(enq.me_max_sg));
		printf("  Max DP\t\t\t%d\n", le16toh(enq.me_max_dp));
		printf("  Max IOD\t\t\t%d\n", le16toh(enq.me_max_iod));
		printf("  Max Comb\t\t\t%d\n", le16toh(enq.me_max_comb));
		printf("  Latency\t\t\t%ds\n", enq.me_latency);
		printf("  SCSI Timeout\t\t\t%ds\n", enq.me_scsi_timeout);
		printf("  Min Free Lines\t\t%d\n",
		    le16toh(enq.me_min_freelines));
		printf("  Rate Constant\t\t\t%d\n", enq.me_rate_const);
		printf("  MAXBLK\t\t\t%d\n", le16toh(enq.me_maxblk));
		printf("  Blocking Factor\t\t%d sectors\n",
		    le16toh(enq.me_blocking_factor));
		printf("  Cache Line Size\t\t%d blocks\n",
		    le16toh(enq.me_cacheline));
		printf("  SCSI Capability\t\t%s%dMHz, %d bit\n", 
		      enq.me_scsi_cap & (1<<4) ? "differential " : "",
		      (1 << ((enq.me_scsi_cap >> 2) & 3)) * 10,
		      8 << (enq.me_scsi_cap & 0x3));
		printf("  Firmware Build Number\t\t%d\n",
		    le16toh(enq.me_firmware_build));
		printf("  Fault Management Type\t\t%d\n",
		    enq.me_fault_mgmt_type);
#if 0
		printf("  Features\t\t\t%b\n", enq.me_firmware_features,
		      "\20\4Background Init\3Read Ahead\2MORE\1Cluster\n");
#endif
	} else if (verbosity > 0 && ci.ci_iftype == 1)
		warnx("can't be verbose for this firmware level");

	fflush(stdout);

	if (ci.ci_firmware_id[0] < 3) {
		warnx("can't display physical drives for this firmware level");
		return (0);
	}

	/*
	 * Fetch and print physical drive data.
	 */
	for (channel = 0; channel < enq.me_configured_channels; channel++) {
		for (target = 0; target < enq.me_max_targets; target++)
			if (mlx_get_device_state(channel, target, &pd) == 0 &&
			    (pd.pd_flags1 & MLX_PHYS_DRV_PRESENT) != 0)
				mlx_print_phys_drv(&pd, channel, target, "  ");
	}

	return (0);
}

/*
 * Recscan for new or changed system drives.
 */
int
cmd_rescan(char **argv)
{

	if (ioctl(mlxfd, MLX_RESCAN_DRIVES) < 0)
		err(EXIT_FAILURE, "rescan failed");
	return (0);
}

/*
 * Detach one or more system drives from a controller.
 */
static void
cmd_detach0(struct mlx_disk *md)
{

	if (ioctl(mlxfd, MLXD_DETACH, &md->hwunit) < 0)
		warn("can't detach %s", md->name);
}

int
cmd_detach(char **argv) 
{

	mlx_disk_iterate(cmd_detach0);
	return (0);
}

/*
 * Initiate a consistency check on a system drive.
 */
static void
cmd_check0(struct mlx_disk *md)
{
	int result;

	if (ioctl(mlxfd, MLXD_CHECKASYNC, &result) == 0)
		return;

	switch (result) {
	case 0x0002:
		warnx("one or more of the SCSI disks on which %s", md->name);
		warnx("depends is DEAD.");
		break;

	case 0x0105:
		warnx("drive %s is invalid, or not a drive which ", md->name);
		warnx("can be checked.");
		break;

	case 0x0106:
		warnx("drive rebuild or consistency check is already ");
		warnx("in progress on this controller.");
		break;

	default:
		err(EXIT_FAILURE, "ioctl(MLXD_CHECKASYNC)");
		/* NOTREACHED */
	}
}

int
cmd_check(char **argv)
{

	if (ci.ci_firmware_id[0] < 3) {
		warnx("action not supported by this firmware version");
		return (1);
	}

	mlx_disk_iterate(cmd_check0);
	return (0);
}

/*
 * Initiate a physical drive rebuild.
 */
int
cmd_rebuild(char **argv)
{
	struct mlx_rebuild_request rb;
	char *p;

	if (ci.ci_firmware_id[0] < 3) {
		warnx("action not supported by this firmware version");
		return (1);
	}

	if (argv[0] == NULL || argv[1] != NULL)
		usage();

	rb.rr_channel = (int)strtol(*argv, &p, 0);
	if (p[0] != ':' || p[1] == '\0')
		usage();

	rb.rr_target = (int)strtol(*argv, &p, 0);
	if (p[0] != '\0')
		usage();

	if (ioctl(mlxfd, MLX_REBUILDASYNC, &rb) == 0)
		return (0);

	switch (rb.rr_status) {
	case 0x0002:
		warnx("the drive at %d:%d is already ONLINE", rb.rr_channel,
		    rb.rr_target);
		break;

	case 0x0004:
		warnx("drive failed during rebuild");
		break;

	case 0x0105:
		warnx("there is no drive at %d:%d", rb.rr_channel,
		    rb.rr_target);
		break;

	case 0x0106:
		warnx("drive rebuild or consistency check is already in ");
		warnx("progress on this controller");
		break;

	default:
		err(EXIT_FAILURE, "ioctl(MLXD_CHECKASYNC)");
		/* NOTREACHED */
	}

	return(0);
}
