/*	$NetBSD: scsi_subr.c,v 1.3 2001/04/17 13:31:00 ad Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum; Jason R. Thorpe of the Numerical Aerospace
 * Simulation Facility, NASA Ames Research Center.
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

/*
 * SCSI support subroutines.
 *
 * XXX THESE SHOULD BE IN A LIBRARY!
 */

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/scsiio.h>
#include <err.h> 
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <dev/scsipi/scsi_all.h>

#include "extern.h"

#define	STRVIS_ISWHITE(x) ((x) == ' ' || (x) == '\0' || (x) == (u_char)'\377')

void
scsi_command(fd, cmd, cmdlen, data, datalen, timeout, flags)
	int fd;
	void *cmd;
	size_t cmdlen;
	void *data;
	size_t datalen;
	int timeout, flags;
{
	scsireq_t req;

	memset(&req, 0, sizeof(req));

	memcpy(req.cmd, cmd, cmdlen);
	req.cmdlen = cmdlen;
	req.databuf = data;
	req.datalen = datalen;
	req.timeout = timeout;
	req.flags = flags;
	req.senselen = SENSEBUFLEN;

	if (ioctl(fd, SCIOCCOMMAND, &req) == -1)
		err(1, "SCIOCCOMMAND");

	if (req.retsts == SCCMD_OK)
		return;

	/* Some problem; report it and exit. */
	if (req.retsts & SCCMD_TIMEOUT)
		fprintf(stderr, "%s: SCSI command timed out\n", dvname);
	if (req.retsts & SCCMD_BUSY)
		fprintf(stderr, "%s: device is busy\n", dvname);
	if (req.retsts & SCCMD_SENSE)
		scsi_print_sense(dvname, &req, 1);

	exit(1);
}

void
scsi_mode_sense(fd, pgcode, pctl, buf, len)
	int fd;
	u_int8_t pgcode, pctl;
	void *buf;
	size_t len;
{
	struct scsi_mode_sense cmd;

	memset(&cmd, 0, sizeof(cmd));
	memset(buf, 0, len);

	cmd.opcode = SCSI_MODE_SENSE;
	cmd.page = pgcode | pctl;
	cmd.length = len;

	scsi_command(fd, &cmd, sizeof(cmd), buf, len, 10000, SCCMD_READ);
}

void
scsi_mode_select(fd, pgcode, pctl, buf, len)
	int fd;
	u_int8_t pgcode, pctl;
	void *buf;
	size_t len;
{
	struct scsi_mode_select cmd;

	memset(&cmd, 0, sizeof(cmd));

	cmd.opcode = SCSI_MODE_SELECT;
	cmd.length = len;

	scsi_command(fd, &cmd, sizeof(cmd), buf, len, 10000, SCCMD_WRITE);
}

void
scsi_strvis(sdst, dlen, ssrc, slen)
	char *sdst;
	size_t dlen;
	const char *ssrc;
	size_t slen;
{
	u_char *dst = (u_char *)sdst;
	const u_char *src = (const u_char *)ssrc;

	/* Trim leading and trailing blanks and NULs. */
	while (slen > 0 && STRVIS_ISWHITE(src[0]))
		++src, --slen;
	while (slen > 0 && STRVIS_ISWHITE(src[slen - 1]))
		--slen;

	while (slen > 0) {
		if (*src < 0x20 || *src >= 0x80) {
			/* non-printable characters */ 
			dlen -= 4;
			if (dlen < 1)
				break;
			*dst++ = '\\';
			*dst++ = ((*src & 0300) >> 6) + '0';
			*dst++ = ((*src & 0070) >> 3) + '0';
			*dst++ = ((*src & 0007) >> 0) + '0';
		} else if (*src == '\\') {
			/* quote characters */
			dlen -= 2;
			if (dlen < 1)
				break;
			*dst++ = '\\';
			*dst++ = '\\';
		} else {
			/* normal characters */
			if (--dlen < 1)
				break;
			*dst++ = *src;
		}
		++src, --slen;
	}

	*dst++ = 0;
}
