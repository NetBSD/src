/*	$NetBSD: bootxx.c,v 1.3 1995/02/22 08:18:19 mycroft Exp $ */

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

#include <sys/param.h>
#include <sys/time.h>
#include <a.out.h>
#include <ufs/ufs/dinode.h>
#include <ufs/ffs/fs.h>

#include "defs.h"

int debug;
int netif_debug;

/*
 * Boot device is derived from ROM provided information.
 */
#define LOADADDR	0x4000
extern struct promvec	*promvec;
struct open_file	io;
char			sblock[SBSIZE];
struct fs		*fs;

#if 0
#define MAXBLOCKNUM	MINBSIZE / sizeof(daddr_t)
#else
#define MAXBLOCKNUM	512
#endif
int			maxblocknum = MAXBLOCKNUM;
daddr_t			blocknum[MAXBLOCKNUM] = { 0 };

main(pp)
	struct promvec *pp;
{
	char *dummy;
	int n;

	io.f_flags = F_RAW;
	if (devopen(&io, 0, &dummy)) {
		printf("Can't open device\n");
		_rtt();
	}

	if ((io.f_dev->dv_strategy)(io.f_devdata, F_READ,
				   btodb(SBOFF), SBSIZE,
				   sblock, &n) || n != SBSIZE) {
		printf("Can't read superblock\n");
		_rtt();
	}
	fs = (struct fs *)sblock;

	(void)copyboot(&io, LOADADDR);
	_rtt();
}

int
copyboot(f, addr)
	register struct open_file	*f;
	register char			*addr;
{
	int	n, i;
	daddr_t	blk;
	void	(*entry)() = (void (*)())addr;

	addr -= sizeof(struct exec); /* XXX */

	for (i = 0; i < MAXBLOCKNUM; i++) {
		if ((blk = blocknum[i]) == 0)
			break;
#ifdef DEBUG
		printf("bootxx: block # %d = %d\n", i, blk);
#endif
		if ((f->f_dev->dv_strategy)(f->f_devdata, F_READ,
					   fsbtodb(fs, blk), fs->fs_bsize,
					   addr, &n)) {
			printf("Read failure\n");
			return -1;
		}
		if (n != fs->fs_bsize) {
			printf("Short read\n");
			return -1;
		}
		addr += fs->fs_bsize;
	}
	if (blk != 0) {
		printf("File too long\n");
		return -1;
	}

#ifdef DEBUG
	printf("bootxx: start 0x%x\n", (int)entry);
#endif
	(*entry)(promvec);
	return 0;
}

_rtt()
{
	promvec->pv_halt();
}
