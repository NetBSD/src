/*	$NetBSD: fdisk.c,v 1.6 1998/02/24 05:39:56 jonathan Exp $	*/

/*
 * Copyright 1997 Piermont Information Systems Inc.
 * All rights reserved.
 *
 * Written by Philip A. Nelson for Piermont Information Systems Inc.
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
 *      This product includes software develooped for the NetBSD Project by
 *      Piermont Information Systems Inc.
 * 4. The name of Piermont Information Systems Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PIERMONT INFORMATION SYSTEMS INC. ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL PIERMONT INFORMATION SYSTEMS INC. BE 
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF 
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/* fdisk.c -- routines to deal with /sbin/fdisk ... */

#include <stdio.h>
#include "defs.h"
#include "md.h"
#include "txtwalk.h"
#include "msg_defs.h"
#include "menu_defs.h"

struct lookfor fdiskbuf[] = {
	{"DLCYL", "DLCYL=%d", "a $0", &dlcyl, NULL},
	{"DLHEAD", "DLHEAD=%d", "a $0", &dlhead, NULL},
	{"DLSEC", "DLSEC=%d", "a $0", &dlsec, NULL},
	{"BCYL", "BCYL=%d", "a $0", &bcyl, NULL},
	{"BHEAD", "BHEAD=%d", "a $0", &bhead, NULL},
	{"BSEC", "BSEC=%d", "a $0", &bsec, NULL},
	{"PART0ID", "PART0ID=%d", "a $0", &part[0][ID], NULL},
	{"PART0SIZE", "PART0SIZE=%d", "a $0", &part[0][SIZE], NULL},
	{"PART0START", "PART0START=%d", "a $0", &part[0][START], NULL},
	{"PART0FLAG", "PART0FLAG=0x%d", "a $0", &part[0][FLAG], NULL},
	{"PART1ID", "PART1ID=%d", "a $0", &part[1][ID], NULL},
	{"PART1SIZE", "PART1SIZE=%d", "a $0", &part[1][SIZE], NULL},
	{"PART1START", "PART1START=%d", "a $0", &part[1][START], NULL},
	{"PART1FLAG", "PART1FLAG=0x%d", "a $0", &part[1][FLAG], NULL},
	{"PART2ID", "PART2ID=%d", "a $0", &part[2][ID], NULL},
	{"PART2SIZE", "PART2SIZE=%d", "a $0", &part[2][SIZE], NULL},
	{"PART2START", "PART2START=%d", "a $0", &part[2][START], NULL},
	{"PART2FLAG", "PART2FLAG=0x%d", "a $0", &part[2][FLAG], NULL},
	{"PART3ID", "PART3ID=%d", "a $0", &part[3][ID], NULL},
	{"PART3SIZE", "PART3SIZE=%d", "a $0", &part[3][SIZE], NULL},
	{"PART3START", "PART3START=%d", "a $0", &part[3][START], NULL},
	{"PART3FLAG", "PART3FLAG=0x%d", "a $0", &part[3][FLAG], NULL}
};

int numfdiskbuf = sizeof(fdiskbuf) / sizeof(struct lookfor);



/*
 * Fetch current MBR from disk into core by parsing /sbin/fdisk output.
 */
void get_fdisk_info (void)
{
	char *textbuf;
	int   textsize;
	int   t1, t2;

	/* Get Fdisk information */
	textsize = collect (T_OUTPUT, &textbuf,
			    "/sbin/fdisk -S /dev/r%sd 2>/dev/null", diskdev);
	if (textsize < 0) {
		endwin();
		(void) fprintf (stderr, "Could not run fdisk.");
		exit (1);
	}
	walk (textbuf, textsize, fdiskbuf, numfdiskbuf);
	free (textbuf);

	/* A common failure of fdisk is to get the number of cylinders
	   wrong and the number of sectors and heads right.  This makes
	   a disk look very big.  In this case, we can just recompute
	   the number of cylinders and things should work just fine.
	   Also, fdisk may correctly indentify the settings to include
	   a cylinder total > 1024, because translation mode is not used.
	   Check for it. */

	if (bcyl > 1024 && disk->geom[1] == bhead && disk->geom[2] == bsec)
		bcyl = 1024;
	else if (bcyl > 1024 && bsec < 64) {
		t1 = disk->geom[0] * disk->geom[1] * disk->geom[2];
		t2 = bhead * bsec;
		if (bcyl * t2 > t1) {
			t2 = t1 / t2;
			if (t2 < 1024)
				bcyl = t2;
		}
	}
}

/*
 * Write incore MBR geometry and partition info to disk
 * using  /sbin/fdisk.
 */
void set_fdisk_info (void)
{
	int i;

	if (bstuffset)
		run_prog ("/sbin/fdisk -i -f -b %d/%d/%d /dev/r%sd",
			  bcyl, bhead, bsec, diskdev);
	
	for (i=0; i<4; i++)
		if (part[i][SET])
			run_prog("/sbin/fdisk -u -f -%d -s %d/%d/%d /dev/r%sd",
				 i, part[i][ID], part[i][START],
				 part[i][SIZE],  diskdev);

	if (activepart >= 0)
		run_prog ("/sbin/fdisk -a -%d -f /dev/r%s",
			  activepart, diskdev);

}
