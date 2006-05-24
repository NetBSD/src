/*	$NetBSD: bootxx.c,v 1.12.8.1 2006/05/24 10:56:59 yamt Exp $ */

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
#include <sys/exec_elf.h>
#include <machine/prom.h>

#include <lib/libsa/stand.h>
#include "libsa.h"
#include "bootxx.h"

int copyboot(struct open_file *, u_long *);

/*
 * Boot device is derived from ROM provided information.
 */
#define LOADADDR	0x11000 /* where to load level 2 bootstrap */
				/* (l2 must relocate itself) */

extern int     	block_size;
extern int     	block_count;	/* length of table */
/* XXX ondisk32 */
extern int32_t 	block_table[];

extern		char bootprog_name[], bootprog_rev[];

int main(void);

int
main(void)
{
	struct open_file	f;
	u_long	addr;
	char *foo;
	int error;

	printf("Boot: bug device: ctrl=%d, dev=%d\n",
		bugargs.ctrl_lun, bugargs.dev_lun);
	printf("\nbootxx: %s first level bootstrap program [%s]\n\n",
		bootprog_name, bootprog_rev);

	f.f_flags = F_RAW;
	if (devopen(&f, 0, &foo)) {
		printf("bootxx: open failed\n");
		_rtt();
	}

	addr = LOADADDR;
	error = copyboot(&f, &addr);
	f.f_dev->dv_close(&f);
	if (!error)
		bugexec((void *)addr);

	/* copyboot had a problem... */
	_rtt();
}

int
copyboot(struct open_file *fp, u_long *addr)
{
	int	i, blknum;
	size_t	n;
	char	*laddr = (char *) *addr;
	union boothead {
		struct exec ah;
		Elf32_Ehdr eh;
	} *x;

	x = (union boothead *)laddr;

	if (!block_count) {
		printf("bootxx: no data!?!\n");
		return -1;
	}

	for (i = 0; i < block_count; i++) {
		if ((blknum = block_table[i]) == 0)
			break;
#ifdef DEBUG
		printf("bootxx: read block # %d = %d\n", i, blknum);
#endif
		if ((fp->f_dev->dv_strategy)(fp->f_devdata, F_READ,
					   blknum, block_size, laddr, &n))
		{
			printf("bootxx: read failed\n");
			return -1;
		}
		if (n != block_size) {
			printf("bootxx: short read\n");
			return -1;
		}
		laddr += block_size;
	}

	if (N_GETMAGIC(x->ah) == OMAGIC)
		*addr += sizeof(x->ah);
	else if (memcmp(x->eh.e_ident, ELFMAG, SELFMAG) == 0) {
		Elf32_Phdr *ep = (Elf32_Phdr *)(*addr + x->eh.e_phoff);
		*addr += ep->p_offset;
	} else {
		printf("bootxx: secondary bootstrap isn't an executable\n");
		return -1;
	}

	return 0;
}
