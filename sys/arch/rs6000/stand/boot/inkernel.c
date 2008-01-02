/*	$NetBSD: inkernel.c,v 1.1.4.2 2008/01/02 21:50:04 bouyer Exp $	*/

/*-
 * Copyright (c) 1996, 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Kazuki Sakamoto.
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

#include <lib/libsa/stand.h>
#include <sys/param.h>

#include "boot.h"
#include "magic.h"

#define	KERNENTRY	(RELOC - 0x200000)
/*#define	KERNENTRY	(0x800000)*/
/* Note that the iplcb lives in the 0x700000 range, so we need to avoid that*/

void
init_in(u_long ladr)
{
	extern char _start[], _edata[], _end[];
	/*char *p = (char *)(ladr + _end); for loadaddr 0x0*/
	char *p = (char *)(ladr + (_end - ladr));
	u_int i;

	printf("p=%p start %p edata %p end %p ladr %lx\n", p, _start, _edata, _end, ladr);

	for (i = 0; i < 4096; i++, p++) {
		if (memcmp(p, magic, MAGICSIZE) == 0) {
			kern_len = *(int *)(p + MAGICSIZE);
			printf("Found magic at 0x%x, kernel is size 0x%x\n", i, kern_len);
			memmove((char *)KERNENTRY,
				p + MAGICSIZE + KERNLENSIZE, kern_len);
			return;
		}
	}
	printf("magic is not found.\n");
}

int
inopen(struct open_file *p)
{

	if (kern_len)
		return (0);
	return (EINVAL);
}

int
inclose(struct open_file *p)
{

	return (0);
}

int
instrategy(void *devdata, int func, daddr_t blk, size_t size, void *buf,
	size_t *rsize)
{

	memcpy(buf, (char *)KERNENTRY + ((long)blk * DEV_BSIZE), size);
	*rsize = size;
	return (0);
}
