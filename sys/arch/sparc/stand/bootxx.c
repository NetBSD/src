/*	$NetBSD: bootxx.c,v 1.4 1995/09/16 23:20:27 pk Exp $ */

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

#include <stand.h>
#include "promdev.h"

int debug;
int netif_debug;

/*
 * Boot device is derived from ROM provided information.
 */
struct open_file	io;
char			sblock[SBSIZE];
struct fs		*fs;

#if 0
#define MAXBLOCKNUM	MINBSIZE / sizeof(daddr_t)
#else
#define MAXBLOCKNUM	256
#endif
int			maxblocknum = MAXBLOCKNUM;
daddr_t			blocknum[MAXBLOCKNUM] = { 0 };
const char		progname[] = "bootxx";

void	loadboot __P((struct open_file *, caddr_t));

main()
{
	char	*dummy;
	int	n;
	register void (*entry)__P((caddr_t)) = (void (*)__P((caddr_t)))LOADADDR;

	prom_init();
	io.f_flags = F_RAW;
	if (devopen(&io, 0, &dummy)) {
		panic("%s: can't open device", progname);
	}

	/*
	 * Read superblock.
	 */
	if ((io.f_dev->dv_strategy)(io.f_devdata, F_READ,
				   btodb(SBOFF), SBSIZE,
				   (char *)LOADADDR, &n) || n != SBSIZE) {
		panic("%s: can't read superblock", progname);
	}
	bcopy(LOADADDR, sblock, SBSIZE);
	fs = (struct fs *)sblock;

	(void)loadboot(&io, LOADADDR);
	(*entry)(cputyp == CPU_SUN4 ? LOADADDR : (caddr_t)promvec);
	_rtt();
}

void
loadboot(f, addr)
	register struct open_file	*f;
	register char			*addr;
{
	register int	i;
	register char	*buf;
	int		n;
	daddr_t		blk;

	/*
	 * Allocate a buffer that we can map into DVMA space; only
	 * needed for sun4 architecture, but use it for all machines
	 * to keep code size down as much as possible.
	 */
	buf = alloc(fs->fs_bsize);
	if (buf == NULL)
		panic("%s: alloc failed", progname);

	for (i = 0; i < MAXBLOCKNUM; i++) {
		if ((blk = blocknum[i]) == 0)
			break;
#ifdef DEBUG
		printf("%s: block # %d = %d\n", i, blk);
#endif
		if ((f->f_dev->dv_strategy)(f->f_devdata, F_READ,
					    fsbtodb(fs, blk), fs->fs_bsize,
					    buf, &n)) {
			panic("%s: read failure", progname);
		}
		bcopy(buf, addr, fs->fs_bsize); /* copy over */
		if (n != fs->fs_bsize)
			panic("%s: short read", progname);
		if (i == 0) {
			register int m = N_GETMAGIC(*(struct exec *)addr);
			if (m == ZMAGIC || m == NMAGIC || m == OMAGIC) {
				/* Move exec header out of the way */
				bcopy(addr, addr - sizeof(struct exec), n);
				addr -= sizeof(struct exec);
			}
		}
		addr += n;
	}
	if (blk != 0)
		panic("%s: file too long", progname);

#ifdef DEBUG
	printf("%s: start 0x%x\n", (int)entry);
#endif
}
