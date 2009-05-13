/*	$NetBSD: util.c,v 1.6.8.1 2009/05/13 19:20:29 jym Exp $	*/

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
 */

#ifndef lint
#include <sys/cdefs.h>
__RCSID("$NetBSD: util.c,v 1.6.8.1 2009/05/13 19:20:29 jym Exp $");
#endif /* not lint */

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/queue.h>

#include <dev/ic/mlxreg.h>
#include <dev/ic/mlxio.h>

#include <dev/scsipi/scsipi_all.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <err.h>

#include "extern.h"

int
mlx_command(struct mlx_usercommand *mu, int bomb)
{
	int rv;

	if ((rv = ioctl(mlxfd, MLX_COMMAND, mu)) != 0 && bomb)
		err(EXIT_FAILURE, "cmd 0x%02x failed",
		    mu->mu_command[0]);

	return (rv);
}

void
mlx_enquiry(struct mlx_enquiry2 *enq)
{
	struct mlx_usercommand mu;
	struct mlx_enquiry_old meo;

	memset(&mu, 0, sizeof(mu));

	mu.mu_datasize = sizeof(*enq);
	mu.mu_buf = enq;
	mu.mu_bufptr = 8;
	mu.mu_bufdir = MU_XFER_IN;
	mu.mu_command[0] = MLX_CMD_ENQUIRY2;

	mlx_command(&mu, 1);

	/*
	 * If we get back a firmware major of 0, this is (probably) an old
	 * controller, so we need to pull the firmware version from the old
	 * enquiry structure.
	 */
	if (enq->me_firmware_id[0] == 0) {	
		memset(&mu, 0, sizeof(mu));

		mu.mu_datasize = sizeof(meo);
		mu.mu_buf = &meo;
		mu.mu_bufptr = 8;
		mu.mu_bufdir = MU_XFER_IN;
		mu.mu_command[0] = MLX_CMD_ENQUIRY_OLD;

		mlx_command(&mu, 1);

		enq->me_firmware_id[0] = meo.me_fwmajor;
		enq->me_firmware_id[1] = meo.me_fwminor;
		enq->me_firmware_id[2] = 0;
		enq->me_firmware_id[3] = '0';
	}
}

void
mlx_configuration(struct mlx_core_cfg *cfg, int wr)
{
	struct mlx_usercommand mu;

	memset(&mu, 0, sizeof(mu));

	mu.mu_datasize = sizeof(*cfg);
	mu.mu_buf = cfg;
	mu.mu_bufptr = 8;
	mu.mu_bufdir = (wr ? MU_XFER_OUT : MU_XFER_IN);
	mu.mu_command[0] = (wr ? MLX_CMD_WRITE_CONFIG : MLX_CMD_READ_CONFIG);

	mlx_command(&mu, 1);
}

int
mlx_get_device_state(int chan, int targ, struct mlx_phys_drv *pd)
{
	struct mlx_usercommand mu;

	memset(&mu, 0, sizeof(mu));

	mu.mu_datasize = sizeof(*pd);
	mu.mu_buf = pd;
	mu.mu_bufptr = 8;
	mu.mu_bufdir = MU_XFER_IN;
	mu.mu_command[0] = MLX_CMD_DEVICE_STATE;
	mu.mu_command[2] = chan;
	mu.mu_command[3] = targ;
	
	return (mlx_command(&mu, 0));
}

int
mlx_scsi_inquiry(int chan, int targ, char **vendor, char **device,
		 char **revision)
{
	struct mlx_usercommand mu;
	static struct {
		struct	mlx_dcdb dcdb;
		struct	scsipi_inquiry_data inq;
	} __packed dcdb_cmd;
	struct scsipi_inquiry *inq_cmd;
	int rv;

	inq_cmd = (struct scsipi_inquiry *)&dcdb_cmd.dcdb.dcdb_cdb[0];

	memset(&mu, 0, sizeof(mu));
	mu.mu_datasize = sizeof(dcdb_cmd);
	mu.mu_buf = &dcdb_cmd;
	mu.mu_command[0] = MLX_CMD_DIRECT_CDB;
	mu.mu_bufdir = MU_XFER_IN | MU_XFER_OUT;

	memset(&dcdb_cmd, 0, sizeof(dcdb_cmd));
	dcdb_cmd.dcdb.dcdb_target = (chan << 4) | targ;
	dcdb_cmd.dcdb.dcdb_flags = MLX_DCDB_DATA_IN | MLX_DCDB_TIMEOUT_10S;
	dcdb_cmd.dcdb.dcdb_datasize = sizeof(dcdb_cmd.inq);
	dcdb_cmd.dcdb.dcdb_length = 6;
	dcdb_cmd.dcdb.dcdb_sense_length = 40;

	inq_cmd->opcode = INQUIRY;
	inq_cmd->length = sizeof(dcdb_cmd.inq);

	if ((rv = mlx_command(&mu, 0)) == 0) {
		*vendor = &dcdb_cmd.inq.vendor[0];
		*device = &dcdb_cmd.inq.product[0];
		*revision = &dcdb_cmd.inq.revision[0];
	}

	return (rv);
}

void
mlx_print_phys_drv(struct mlx_phys_drv *pd, int chn, int targ,
		   const char *prefix)
{
	const char *type;
	char *device, *vendor, *revision;

	switch (pd->pd_flags2 & 0x03) {
		case MLX_PHYS_DRV_DISK:
		type = "disk";
		break;

	case MLX_PHYS_DRV_SEQUENTIAL:
		type = "tape";
		break;

	case MLX_PHYS_DRV_CDROM:
		type= "cdrom";
		break;

	case MLX_PHYS_DRV_OTHER:
	default:
		type = "unknown";
		break;
	}

	printf("%s%s%02d%02d ", prefix, type, chn, targ);

	switch (pd->pd_status) {
	case MLX_PHYS_DRV_DEAD:
		printf(" (dead)	   ");
		break;

	case MLX_PHYS_DRV_WRONLY:
		printf(" (write-only) ");
		break;

	case MLX_PHYS_DRV_ONLINE:
		printf(" (online)	 ");
		break;

	case MLX_PHYS_DRV_STANDBY:
		printf(" (standby)	");
		break;

	default:
		printf(" (0x%02x)   ", pd->pd_status);
		break;
	}

	printf("\n");
	if (verbosity == 0)
		return;

	printf("%s   ", prefix);
	if (!mlx_scsi_inquiry(chn, targ, &vendor, &device, &revision))
		printf("'%8.8s' '%16.16s' '%4.4s'", vendor, device, revision);
	else
		printf("<IDENTIFY FAILED>");

	printf(" %dMB ", pd->pd_config_size / 2048);

	if ((pd->pd_flags2 & MLX_PHYS_DRV_FAST20) != 0)
		printf(" ultra");
	else if ((pd->pd_flags2 & MLX_PHYS_DRV_FAST) != 0)
		printf(" fast");

	if ((pd->pd_flags2 & MLX_PHYS_DRV_WIDE) != 0)
		printf(" wide");

	if ((pd->pd_flags2 & MLX_PHYS_DRV_SYNC) != 0)
		printf(" sync");

	if ((pd->pd_flags2 & MLX_PHYS_DRV_TAG) != 0)
		printf(" tag-enabled");

	printf("\n");
}
