/*
 * Copyright (c) 1997 Martin Husemann <martin@duskware.de>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
 * Last Edit-Date: [Sat Jan  6 12:30:18 2001]
 *
 * daicctl - maintenance tool and debug utility
 * dnload.c: download on-board processor code to active cards
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <netisdn/i4b_ioctl.h>
#include "daicctl.h"

void
download(fd, controller, filename)
	int fd, controller;
	char *filename;
{
	int i, num_ports = 1;
	struct isdn_download_request dr;
	struct isdn_dr_prot prots[4];
	msg_ctrl_info_req_t info;
	FILE *f;
	struct stat sb;
	u_int8_t *data;
	size_t rlen;

	memset(&info, 0, sizeof info);
	info.controller = controller;
	if (ioctl(fd, I4B_CTRL_INFO_REQ, &info) == -1) {
		perror("ctrl info req");
		exit(1);
	}
	if (info.ctrl_type != CTRL_DAIC) {
		fprintf(stderr, "this is not a Diehl active isdn card...\n");
		exit(1);
	}
	if (info.card_type == CARD_TYPEA_DAIC_QUAD)
		num_ports = 4;

	memset(&dr, 0, sizeof dr);
	dr.controller = controller;
	dr.numprotos = num_ports;
	dr.protocols = prots;
	if (stat(filename, &sb)) {
		perror(filename);
		exit(1);
	}
	f = fopen(filename, "r");
	if (!f) {
		perror(filename);
		exit(1);
	}
	data = (u_int8_t*)malloc(sb.st_size);
	if (!data) {
		fprintf(stderr, "could not allocate memory for microcode!\n");
		exit(1);
	}
	rlen = fread(data, 1, sb.st_size, f);
	if (rlen != sb.st_size) {
		fprintf(stderr, "error reading microcode, read %lu bytes: %s\n",
			(unsigned long)rlen, strerror(errno));
		exit(1);
	}
	fclose(f);
	for (i = 0; i < num_ports; i++) {
		prots[i].bytecount = sb.st_size;
		prots[i].microcode = data;
	} 
	if (ioctl(fd, I4B_CTRL_DOWNLOAD, &dr) == -1) {
		fprintf(stderr, "downloading microcode to controller %d failed: %s\n",
			controller, strerror(errno));
		exit(1);
	}
	printf("download successful\n");
}
