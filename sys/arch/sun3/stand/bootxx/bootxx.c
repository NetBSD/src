/*	$NetBSD: bootxx.c,v 1.3 1995/06/01 21:03:07 gwr Exp $ */

/*
 * Copyright (c) 1994 Paul Kranenburg
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
 *      This product includes software developed by Paul Kranenburg.
 * 4. The name of the author may not be used to endorse or promote products
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
 * This is a generic "first-stage" boot program.
 *
 * Note that this program has absolutely no filesystem knowledge!
 *
 * Instead, this uses a table of disk block numbers that are
 * filled in by the installboot program such that this program
 * can load the "second-stage" boot program.
 */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/exec.h>

#include <machine/mon.h>
#include "stand.h"

int debug;
int netif_debug;

/*
 * Boot device is derived from ROM provided information.
 */
#define LOADADDR	0x4000
struct open_file	io;

/* This determines the largest boot program we can load. */
#define MAXBLOCKNUM	64

/*
 * These three names are known by installboot.
 * The block_table contains starting block numbers,
 * in terms of 512-byte blocks.  Each non-zero value
 * will result in a read of block_size bytes.
 */
int     	block_size = 512;	/* default */
int     	block_count = MAXBLOCKNUM;	/* length of table */
daddr_t 	block_table[MAXBLOCKNUM] = { 0 };


main()
{
	char *dummy;
	int n;

#ifdef DEBUG
	printf("bootxx: open...\n");
#endif
	io.f_flags = F_RAW;
	if (devopen(&io, 0, &dummy)) {
		printf("bootxx: open failed\n");
		exit();
	}

	(void)copyboot(&io, LOADADDR);
	exit();
}

int
copyboot(f, addr)
	register struct open_file	*f;
	register char			*addr;
{
	int	n, i, bsize;
	daddr_t	blk;
	void	(*entry)() = (void (*)())addr;

#ifdef	sparc
	/*
	 * On the sparc, the 2nd stage boot has an a.out header.
	 * On the sun3, (by tradition) the 2nd stage boot programs
	 * have the a.out header stripped off.  (1st stage boot
	 * programs have the header stripped by installboot.)
	 */
	/* XXX - This assumes OMAGIC format! */
	addr -= sizeof(struct exec); /* XXX */
#endif

	for (i = 0; i < block_count; i++) {

		/* XXX - This is FS knowledge, actually. */
		if ((blk = block_table[i]) == 0)
			break;

#ifdef DEBUG
		printf("bootxx: block # %d = %d\n", i, blk);
#endif
		if ((f->f_dev->dv_strategy)(f->f_devdata, F_READ,
					   blk, block_size, addr, &n))
		{
			printf("bootxx: read failed\n");
			return -1;
		}
		if (n != block_size) {
			printf("bootxx: short read\n");
			return -1;
		}
		addr += block_size;
	}

#ifdef DEBUG
	printf("bootxx: start 0x%x\n", (int)entry);
#endif
	(*entry)();
	return 0;
}

