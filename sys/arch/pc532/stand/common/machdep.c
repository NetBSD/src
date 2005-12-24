/*	$NetBSD: machdep.c,v 1.7 2005/12/24 20:07:24 perry Exp $	*/

/*
 * Copyright (c) 1994 Philip L. Budne.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Philip L. Budne.
 * 4. The name of Philip L. Budne may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
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
 * pc532 standalone machdep code
 * Phil Budne, May 10, 1994
 *
 */

#include <sys/types.h>
#include <machine/cpufunc.h>

#include <lib/libsa/stand.h>

#include <pc532/stand/common/samachdep.h>
#include <pc532/stand/common/so.h>

/* XXX TEMP; would like to use code more like hp300 scsi.c */

int
scsialive(int ctlr)
{

	return 1;		/* controller always alive! */
}

int
scsi_tt_read(int ctlr, int slave, void *buf, u_int len, daddr_t blk, u_int nblk)
{

	if (sc_rdwt(DISK_READ, blk, buf, nblk, 1 << slave, 0) == 0)
		return 0;
	return EIO;
}

int
scsi_tt_write(int ctlr, int slave, void *buf, u_int len, daddr_t blk,
    u_int nblk)
{

	return EIO;
}

void
machdep_start(char *entry, int how_to, char *loadaddr, char *ssym, char *esym)
{
	char *load;
	extern u_int opendev;

	load = (char *)((long)entry & 0x00ffff00);
	/* /sys/lib/libsa/exec.c subtracts loadaddr from esym??? */
	esym += (u_long)loadaddr;

	if (load != loadaddr) {
		memcpy(load, loadaddr, esym - loadaddr);
		ssym += loadaddr - load;
		esym += loadaddr - load;
	}
	entry = (char *)((u_long)entry & 0xffffff);

	printf("\n");

	run_prog((u_long)how_to,	/* r7 */
		 (u_long)opendev,	/* r6 */
		 (u_long)load,		/* r5 */
		 (u_long)esym,		/* r4 */
		 (u_long)3253232532UL,	/* r3 */
		 (u_long)entry);	/* pc */
}

void
_rtt(void)
{

	/* Use monitor scratch area as stack. */
	lprd(sp, 0x2000);

	/* Copy start of ROM. */
	memcpy((void *)0, (void *)0x10000000, 0x1f00);

	/* Jump into ROM copy. */
	__asm volatile("jump @0");

	while (1)
		;
}
