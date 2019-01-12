/*	$NetBSD: iris_parse.c,v 1.1 2019/01/12 16:44:47 tsutsui Exp $	*/

/*
 * Copyright (c) 2018 Naruaki Etomi
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Silicon Graphics "IRIS" series MIPS processors machine bootloader.
 * Parse argv argument vector.
 */

#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>

#include "iris_machdep.h"
#include "iris_scsivar.h"

uint8_t scsi_ctlr = 255, scsi_id = 255, scsi_part = 255;

void
parse(char *argv[], char *kernelname)
{
	char disksetting[1 + 32];
	char bootpath[1 + 64];
	int i;

	char *ep = strcpy(bootpath, argv[1]);
	ep = strrchr(bootpath, '/');
	if (ep == NULL)
		again();

	strcpy(kernelname, ep + 1);

	i =  ep - bootpath;
	bootpath[i - 1] = '\0';

	ep = strchr(bootpath, '(');
	if (ep == NULL)
		/* horrible! */
		again();

	/* ctlr,id,part */
	strcpy(disksetting, ep + 1);

	i = 0;

	while (disksetting[i] != '\0') {
		if (disksetting[i] >= '0' && disksetting[i] <= '9') {
			if (i == 0)
				scsi_ctlr = atoi(&disksetting[i]);
			if (i == 2)
				scsi_id = atoi(&disksetting[i]);
			if (i == 4)
				scsi_part = atoi(&disksetting[i]);
		}
		i++;
	}

	if ((scsi_ctlr == 255) || (scsi_id == 255) || (scsi_part == 255))
		again();
}
