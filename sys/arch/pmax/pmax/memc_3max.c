/*	$NetBSD: memc_3max.c,v 1.8 1999/05/26 04:23:59 nisimura Exp $	*/

/*
 * Copyright (c) 1998 Jonathan Stone.  All rights reserved.
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
 *	This product includes software developed by Jonathan Stone for
 *      the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: memc_3max.c,v 1.8 1999/05/26 04:23:59 nisimura Exp $");

#include <sys/types.h>
#include <sys/systm.h>

#include <machine/locore.h>		/* wbflush() */

#include <pmax/pmax/kn02.h>	/* error-register defs copied into kn03.h */
#include <pmax/pmax/memc.h>

/*
 * the 3max and 3maxplus have compatible memory subsystems;
 * we handle them both here.
 */


void
dec_mtasic_err(erradr, errsyn)
	u_int erradr, errsyn;
{
	u_int physadr;

	if (!(erradr & KN02_ERR_VALID))
		return;
	/* extract the physical word address and compensate for pipelining */
	physadr = erradr & KN02_ERR_ADDRESS;
	if (!(erradr & KN02_ERR_WRITE))
		physadr = (physadr & ~0xfff) | ((physadr & 0xfff) - 5);
	physadr <<= 2;
	printf("%s memory %s %s error at 0x%08x\n",
		(erradr & KN02_ERR_CPU) ? "CPU" : "DMA",
		(erradr & KN02_ERR_WRITE) ? "write" : "read",
		(erradr & KN02_ERR_ECCERR) ? "ECC" : "timeout",
		physadr);
	if (erradr & KN02_ERR_ECCERR) {
		u_int errsyn_value = *(u_int *)errsyn;
		*(u_int *)errsyn = 0;
		wbflush();
		printf("   ECC 0x%08x\n", errsyn_value);

		/* check for a corrected, single bit, read error */
		if (!(erradr & KN02_ERR_WRITE)) {
			if (physadr & 0x4) {
				/* check high word */
				if (errsyn_value & KN02_ECC_SNGHI)
					return;
			} else {
				/* check low word */
				if (errsyn_value & KN02_ECC_SNGLO)
					return;
			}
		}
		printf("\n");
	}
	else
		printf("\n");
	panic("panic(\"Mem error interrupt\");\n");
}

/*
 * Xxx noncontig memory probing with mixed-sized memory  boards
 * XXX on 3max (kn02) or 3maxplus (kn03) belongs here.
 */

/*
 * Xxx any support for NVRAM (PrestoServe) modules in
 * XXX on 3max (kn02) or 3maxplus (kn03) memory slots probably belongs here,
 * since we need to not probe it as normal RAM.
 */
