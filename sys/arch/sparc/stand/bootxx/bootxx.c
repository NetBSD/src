/*	$NetBSD: bootxx.c,v 1.8.12.2 2002/06/20 03:41:14 nathanw Exp $ */

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg.
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

#include <sys/param.h>
#include <sys/time.h>
#include <sys/exec.h>
#include <sys/bootblock.h>

#include <lib/libsa/stand.h>

#include <machine/promlib.h>
#include <sparc/stand/common/promdev.h>

int debug;
int netif_debug;

/*
 * Boot device is derived from ROM provided information.
 */
const char		progname[] = "bootxx";
struct open_file	io;

/*
 * The contents of the bbinfo below are set by installboot(8)
 * to hold the filesystem data of the second-stage boot program
 * (typically `/boot'): filesystem block size, # of filesystem
 * blocks and the block numbers themselves.
 */
struct shared_bbinfo bbinfo = {
	{ SPARC_BBINFO_MAGIC },
	0,
	SHARED_BBINFO_MAXBLOCKS,
	{ 0 }
};

int	main __P((void));
void	loadboot __P((struct open_file *, caddr_t));

int
main()
{
	char	*dummy;
	void (*entry)__P((void *)) = (void (*)__P((void *)))PROM_LOADADDR;
	void	*arg;

#ifdef HEAP_VARIABLE
	{
		extern char end[];
		setheap((void *)ALIGN(end), (void *)0xffffffff);
	}
#endif
	prom_init();
	prom_bootdevice = prom_getbootpath();
	io.f_flags = F_RAW;
	if (devopen(&io, 0, &dummy)) {
		panic("%s: can't open device `%s'", progname,
			prom_bootdevice != NULL ? prom_bootdevice : "unknown");
	}

	(void)loadboot(&io, (caddr_t)PROM_LOADADDR);
	(io.f_dev->dv_close)(&io);

	arg = (prom_version() == PROM_OLDMON) ? (caddr_t)PROM_LOADADDR : romp;
	(*entry)(arg);
	_rtt();
}

void
loadboot(f, addr)
	struct open_file	*f;
	char			*addr;
{
	int	i;
	char	*buf;
	size_t		n;
	daddr_t		blk;

	/*
	 * Allocate a buffer that we can map into DVMA space; only
	 * needed for sun4 architecture, but use it for all machines
	 * to keep code size down as much as possible.
	 */
	buf = alloc(bbinfo.bbi_block_size);
	if (buf == NULL)
		panic("%s: alloc failed", progname);

	for (i = 0; i < bbinfo.bbi_block_count; i++) {
		if ((blk = bbinfo.bbi_block_table[i]) == 0)
			panic("%s: block table corrupt", progname);

#ifdef DEBUG
		printf("%s: block # %d = %d\n", progname, i, blk);
#endif
		if ((f->f_dev->dv_strategy)(f->f_devdata, F_READ, blk,
		    bbinfo.bbi_block_size, buf, &n)) {
			printf("%s: read failure", progname);
			_rtt();
		}
		bcopy(buf, addr, bbinfo.bbi_block_size);
		if (n != bbinfo.bbi_block_size)
			panic("%s: short read", progname);
		if (i == 0) {
			int m = N_GETMAGIC(*(struct exec *)addr);
			if (m == ZMAGIC || m == NMAGIC || m == OMAGIC) {
				/* Move exec header out of the way */
				bcopy(addr, addr - sizeof(struct exec), n);
				addr -= sizeof(struct exec);
			}
		}
		addr += n;
	}

}

/*
 * We don't need the overlap handling feature that the libkern version
 * of bcopy() provides. We DO need code compactness..
 */
void
bcopy(src, dst, n)
	const void *src;
	void *dst;
	size_t n;
{
	const char *p = src;
	char *q = dst;

	while (n-- > 0)
		*q++ = *p++;
}
