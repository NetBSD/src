/*	$NetBSD: bootsectmain.c,v 1.5 2003/04/02 10:39:33 fvdl Exp $	*/

/*
 * Copyright (c) 1996
 *	Matthias Drochner.  All rights reserved.
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
 *	This product includes software developed for the NetBSD Project
 *	by Matthias Drochner.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 *
 */

/* load remainder of boot program
 (blocks from fraglist),
 start main()
 needs lowlevel parts from biosdisk_ll.c
 */

#include <lib/libsa/stand.h>

#include <biosdisk_ll.h>

#include "bbinfo.h"

int boot_biosdev; /* exported */
int boot_biossector; /* exported */
void bootsectmain(int, int);

extern struct fraglist fraglist64;
extern char edata[], end[];

extern void main(void);

/* bios_sector is a hint..... */

void
bootsectmain(int biosdev, int bios_sector)
{
	struct biosdisk_ll d;
	int i;
	char *buf;

	/*
	 * load sectors from bootdev
	 */
	d.dev = biosdev;
	set_geometry(&d, NULL);

	buf = (char*)(PRIM_LOADSZ * BIOSDISK_SECSIZE);

	for (i = 0; i < fraglist64.numentries; i++) {
		int64_t dblk;
		int num;

		dblk = fraglist64.entries[i].offset;
		num = fraglist64.entries[i].num;

		if (readsects(&d, dblk, num, buf, 1))
			return; /* halts in start_bootsect.S */

		buf += num * BIOSDISK_SECSIZE;
	}

	/* clear BSS */
	buf = edata;
	while(buf < end)
		*buf++ = 0;

	/* call main() */
	boot_biosdev = biosdev;
	boot_biossector = bios_sector;
	main();
}
