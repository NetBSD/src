/*	$NetBSD: SRT1.c,v 1.2 2002/06/03 00:18:27 fredette Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Gordon W. Ross.
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

/* SRT1.c - Stand-alone Run-time startup code, part 1 */

#include <sys/types.h>
#include <machine/mon.h>

#include "libsa.h"
#include "dvma.h"

int _is2 = 0;
int _is3x = 0;
struct sunromvec *_romvec = 0;
void *chain_to_func = 0;

/*
 * These are the function pointers for sun2 vs sun3 vs sun3x stuff.
 */
char *	(*dev_mapin_p) __P((int, u_long, int));
char *	(*dvma_alloc_p) __P((int len));
void	(*dvma_free_p) __P((char *dvma, int len));
char *	(*dvma_mapin_p) __P((char *pkt, int len));
void	(*dvma_mapout_p) __P((char *dmabuf, int len));

/*
 * This is called by SRT0.S
 * to do final prep for main
 */
void
_start()
{
	void **vbr;
	int x;

	/*
	 * Determine sun2 vs sun3 vs sun3x by looking where the
	 * vector base register points.  The PROM always
	 * points that somewhere into [MONSTART..MONEND]
	 * which is a different range on each.
	 */

	vbr = getvbr();
	x = (int)vbr & 0xFFF00000;
	if (x == SUN3X_MONSTART)
		_is3x = 1;
	else if (x == 0)
		_is2 = 1;

	/* Find the PROM vector. */
	if (_is3x)
		x = SUN3X_PROM_BASE;
	else if (_is2)
		x = SUN2_PROM_BASE;
	else
		x = SUN3_PROM_BASE;
	_romvec = ((struct sunromvec *) x);

	/* Setup trap 14 for use as a breakpoint. */
	vbr[32+14] = _romvec->abortEntry;

	/* Initialize sun3 vs sun3x function pointers. */
	if (_is3x)
		sun3x_init();
	else if (_is2)
		sun2_init();
	else
		sun3_init();

	main(0);
	exit(0);
}

void
breakpoint()
{
	__asm __volatile ("trap #14");
}

void
chain_to(func)
	void *func;
{

	/*
	 * If set, this pointer is jumped-to by exit
	 * after carefully restoring the PROM stack.
	 */
	chain_to_func = func;
	ICIA();
	exit(0);
}

/*
 * Boot programs in C++ ?  Not likely!
 */
void
__main() {}
