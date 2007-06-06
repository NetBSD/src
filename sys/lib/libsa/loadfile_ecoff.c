/* $NetBSD: loadfile_ecoff.c,v 1.8 2007/06/06 07:56:39 martin Exp $ */

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center and by Christos Zoulas.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

#ifdef _STANDALONE
#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>
#else
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <err.h>
#endif

#include <sys/param.h>
#include <sys/exec.h>

#include "loadfile.h"

#ifdef BOOT_ECOFF

int
loadfile_coff(fd, coff, marks, flags)
	int fd;
	struct ecoff_exechdr *coff;
	u_long *marks;
	int flags;
{
	paddr_t offset = marks[MARK_START];
	paddr_t minp = ~0, maxp = 0, pos;

	/* some ports dont use the offset */
	offset = offset;

	/* Read in text. */
	if (lseek(fd, ECOFF_TXTOFF(coff), SEEK_SET) == -1)  {
		WARN(("lseek text"));
		return 1;
	}

	if (coff->a.tsize != 0) {
		if (flags & LOAD_TEXT) {
			PROGRESS(("%lu", coff->a.tsize));
			if (READ(fd, coff->a.text_start, coff->a.tsize) !=
			    coff->a.tsize) {
				return 1;
			}
		}
		else {
			if (lseek(fd, coff->a.tsize, SEEK_CUR) == -1) {
				WARN(("read text"));
				return 1;
			}
		}
		if (flags & (COUNT_TEXT|LOAD_TEXT)) {
			pos = coff->a.text_start;
			if (minp > pos)
				minp = pos;
			pos += coff->a.tsize;
			if (maxp < pos)
				maxp = pos;
		}
	}

	/* Read in data. */
	if (coff->a.dsize != 0) {
		if (flags & LOAD_DATA) {
			PROGRESS(("+%lu", coff->a.dsize));
			if (READ(fd, coff->a.data_start, coff->a.dsize) !=
			    coff->a.dsize) {
				WARN(("read data"));
				return 1;
			}
		}
		if (flags & (COUNT_DATA|LOAD_DATA)) {
			pos = coff->a.data_start;
			if (minp > pos)
				minp = pos;
			pos += coff->a.dsize;
			if (maxp < pos)
				maxp = pos;
		}
	}

	/* Zero out bss. */
	if (coff->a.bsize != 0) {
		if (flags & LOAD_BSS) {
			PROGRESS(("+%lu", coff->a.bsize));
			BZERO(coff->a.bss_start, coff->a.bsize);
		}
		if (flags & (COUNT_BSS|LOAD_BSS)) {
			pos = coff->a.bss_start;
			if (minp > pos)
				minp = pos;
			pos = coff->a.bsize;
			if (maxp < pos)
				maxp = pos;
		}
	}

	marks[MARK_START] = LOADADDR(minp);
	marks[MARK_ENTRY] = LOADADDR(coff->a.entry);
	marks[MARK_NSYM] = 1;	/* XXX: Kernel needs >= 0 */
	marks[MARK_SYM] = LOADADDR(maxp);
	marks[MARK_END] = LOADADDR(maxp);
	marks[MARK_DATA] = LOADADDR(coff->a.data_start);
	return 0;
}

#endif /* BOOT_ECOFF */
