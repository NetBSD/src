/*	$NetBSD: exec.c,v 1.1.10.2 2002/06/23 17:36:15 jdolecek Exp $ */

/*-
 * Copyright (c) 1982, 1986, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)boot.c	8.1 (Berkeley) 6/10/93
 */

#include <sys/param.h>

#include <machine/bootinfo.h>

#include <lib/libsa/loadfile.h>

#include <hp300/stand/common/samachdep.h>

#define	round_to_size(x) \
	(((x) + sizeof(u_long) - 1) & ~(sizeof(u_long) - 1))

void
exec_hp300(file, loadaddr, howto)
	char *file;
	u_long loadaddr;
	int howto;
{
	u_long marks[MARK_MAX];
	struct btinfo_magic *bt;
	int fd;

	marks[MARK_START] = loadaddr;
	if ((fd = loadfile(file, marks, LOAD_KERNEL)) == -1)
		return;

	marks[MARK_END] = round_to_size(marks[MARK_END] - loadaddr);
	printf("Start @ 0x%lx [%ld=0x%lx-0x%lx]...\n",
	    marks[MARK_ENTRY], marks[MARK_NSYM],
	    marks[MARK_SYM], marks[MARK_END]);

	bt = (struct btinfo_magic *)loadaddr;
        bt->common.type = BTINFO_MAGIC;
        bt->magic1 = BOOTINFO_MAGIC1;
        bt->magic2 = BOOTINFO_MAGIC2;

	machdep_start((char *)marks[MARK_ENTRY], howto,
	    (char *)loadaddr, (char *)marks[MARK_SYM],
	    (char *)marks[MARK_END]);
}
