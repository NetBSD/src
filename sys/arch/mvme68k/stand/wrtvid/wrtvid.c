/*	$NetBSD: wrtvid.c,v 1.6.102.1 2008/01/19 12:14:38 bouyer Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Steve C. Woodford.
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* mvme68k's boot block is 512 bytes long */
#define SIZEOF_VID		0x200

/* The first field is effectively the vendor string, in this case NBSD */
#define	VID_ID_OFF		0x000
#define	 VID_ID			"NBSD"
#define	 VID_ID_LEN		4

/* Start block for the 1st stage bootstrap code */
#define	VID_OSS_OFF		0x014
#define	 VID_OSS_TAPE		1
#define	 VID_OSS_DISK		2

/* Length, in 256-byte logical blocks, of the 1st stage bootstrap */
#define	VID_OSL_OFF		0x018

/* Start address of the bootstrap */
#define VID_OSA_OFF		0x01e
#define  VID_OSA		0x003f0000

/* Block number of config area */
#define VID_CAS_OFF		0x093
#define  VID_CAS		1

/* Length, in 256-byte logical blocks, of config area */
#define VID_CAL_OFF		0x094
#define  VID_CAL		1

/* Magic `MOTOROLA' string */
#define VID_MOT_OFF		0x0f8
#define  VID_MOT		"MOTOROLA"
#define  VID_MOT_LEN		8

/* Logical block size, in bytes */
#define	CFG_REC_OFF		0x10a
#define	 CFG_REC		0x100

/* Physical sector size, in bytes */
#define	CFG_PSM_OFF		0x11e
#define	 CFG_PSM		0x200


/*
 * Write a big-endian 32-bit value at the specified address
 */
static void
write32(uint8_t *vp, uint32_t value)
{

	*vp++ = (uint8_t)((value >> 24) & 0xff);
	*vp++ = (uint8_t)((value >> 16) & 0xff);
	*vp++ = (uint8_t)((value >> 8) & 0xff);
	*vp = (uint8_t)(value & 0xff);
}

/*
 * Write a big-endian 16-bit value at the specified address
 */
static void
write16(uint8_t *vp, uint16_t value)
{

	*vp++ = (uint8_t)((value >> 8) & 0xff);
	*vp = (uint8_t)(value & 0xff);
}

int
main(int argc, char **argv)
{
	struct stat st;
	uint16_t len;
	uint8_t *vid;
	char *fn;
	int is_disk;
	int fd;

	if (argc != 2) {
usage:
		fprintf(stderr, "%s: <bootsd|bootst>\n", argv[0]);
		exit(1);
	}

	if (strcmp(argv[1], "bootsd") == 0) {
		is_disk = 1;
		fn = "sdboot";
	} else
	if (strcmp(argv[1], "bootst") == 0) {
		is_disk = 0;
		fn = "stboot";
	} else
		goto usage;

	if (stat(argv[1], &st) < 0) {
		perror(argv[1]);
		exit(1);
	}

	/* How many 256-byte logical blocks (rounded up) */
	len = (uint16_t)((st.st_size + 255) / 256);

	/* For tapes, round up to 8k */
	if (is_disk == 0) {
		len += (8192 / 256) - 1;
		len &= ~((8192 / 256) -1);
	}

	if ((vid = malloc(SIZEOF_VID)) == NULL) {
		perror(argv[0]);
		exit(1);
	}

	/* Generate the VID and CFG data */
	memset(vid, 0, SIZEOF_VID);
	memcpy(&vid[VID_ID_OFF], VID_ID, VID_ID_LEN);
	write32(&vid[VID_OSS_OFF], is_disk ? VID_OSS_DISK : VID_OSS_TAPE);
	write16(&vid[VID_OSL_OFF], len);
	write32(&vid[VID_OSA_OFF], VID_OSA);
	vid[VID_CAS_OFF] = VID_CAS;
	vid[VID_CAL_OFF] = VID_CAL;
	memcpy(&vid[VID_MOT_OFF], VID_MOT, VID_MOT_LEN);
	write16(&vid[CFG_REC_OFF], CFG_REC);
	write16(&vid[CFG_PSM_OFF], CFG_PSM);

	/* Create and write it out */
	if ((fd = open(fn, O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0) {
		perror(fn);
		exit(1);
	}

	if (write(fd, vid, SIZEOF_VID) != SIZEOF_VID) {
		perror(fn);
		exit(1);
	}

	close(fd);
	free(vid);

	return 0;
}
