/*	$NetBSD: mboot.c,v 1.10 2012/04/05 12:01:36 isaki Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Minoura Makoto.
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

#include <sys/types.h>
#include <machine/disklabel.h>

#include "iocs.h"

int bootmain(int);

#define PART_BOOTABLE	0
#define PART_UNUSED	1
#define PART_INUSE	2

int
bootmain(int scsiid)
{
	struct iocs_readcap cap;
	int size;

	if (IOCS_BITSNS(0) & 1)	/* ESC key */
		return 0;

	if (IOCS_S_READCAP(scsiid, &cap) < 0) {
		IOCS_B_PRINT("Error in reading.\r\n");
		return 0;
	}
	size = cap.size >> 9;

	{
		long *label = (void*) 0x3000;
		if (IOCS_S_READ(0, 1, scsiid, size, label) < 0) {
			IOCS_B_PRINT("Error in reading.\r\n");
			return 0;
		}
		if (label[0] != 0x58363853 ||
		    label[1] != 0x43534931) {
			IOCS_B_PRINT("Invalid disk.\r\n");
			return 0;
		}
	}

	{
		struct cpu_disklabel *label = (void*) 0x3000;
		int i, firstinuse=-1;
		unsigned char *t;

		if (IOCS_S_READ(2<<(2-size), size?2:1, scsiid, size, label) < 0) {
			IOCS_B_PRINT("Error in reading.\r\n");
			return 0;
		}
		t = label->dosparts[0].dp_typname;
		if (t[0] != 'X' || t[1] != '6' || t[2] != '8' || t[3] != 'K') {
			IOCS_B_PRINT("Invalid disk.\r\n");
			return 0;
		}

		for (i = 1; i < NDOSPART; i++) {
			if (label->dosparts[i].dp_flag == PART_BOOTABLE)
				break;
			else if (label->dosparts[i].dp_flag == PART_INUSE)
				firstinuse = i;
		}
		if (i >= NDOSPART && firstinuse >= 0)
			i = firstinuse;
		if (i < NDOSPART) {
			unsigned int start = label->dosparts[i].dp_start;
			unsigned int start1 = start << (2-size);
			int r;
			if ((start1 & 0x1fffff) == 0x1fffff)
				r = IOCS_S_READ(start1,
						8>>size,
						scsiid,
						size,
						(void*) 0x2400);
			else
				r = IOCS_S_READEXT(start1,
						   8>>size,
						   scsiid,
						   size,
						   (void*) 0x2400);
			if (r < 0) {
				IOCS_B_PRINT ("Error in reading.\r\n");
				return 0;
			}
			if (*((char*) 0x2400) != 0x60) {
				IOCS_B_PRINT("Invalid disk.\r\n");
				return 0;
			}
			__asm volatile ("movl %0,%%d4\n\t"
				      "movl %1,%%d2\n\t"
				      "jsr 0x2400"
				      :
				      : "g" (scsiid), "g"(start)
				      : "d4");
			return 0;
		}
		IOCS_B_PRINT ("No bootable partition.\r\n");
		return 0;
	}

	return 0;
}
