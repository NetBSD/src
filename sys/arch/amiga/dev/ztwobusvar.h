/*
 * Copyright (c) 1994 Christian E. Hopps
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
 *      This product includes software developed by Christian E. Hopps.
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
 *
 *	$Id: ztwobusvar.h,v 1.2 1994/06/04 11:59:20 chopps Exp $
 */
#ifndef _ZTWOBUSVAR_H_
#define _ZTWOBUSVAR_H_

struct ztwobus_args {
	void *pa;
	void *va;
	int size;
	int slot;
	int manid;
	int prodid;
	int serno;
};
vm_offset_t	ZTWOROMADDR;
vm_offset_t	ZTWOMEMADDR;
u_int		NZTWOMEMPG;
#define ZTWOROMBASE	(0x00D80000)
#define ZTWOROMTOP	(0x00F80000)
#define NZTWOROMPG	btoc(ZTWOROMTOP-ZTWOROMBASE)

/* 
 * maps a ztwo and/or A3000 builtin address into the mapped kva address
 */
#define ztwomap(pa) \
    ((volatile void *)((u_int)ZTWOROMADDR - ZTWOROMBASE + (u_int)(pa)))
#define ztwopa(va) ((caddr_t)(ZTWOROMBASE + (u_int)(va) - (u_int)ZTWOROMADDR))

/*
 * tests whether the address lies in our zorro2 rom space
 */
#define isztwokva(kva) \
    ((u_int)(kva) >= ZTWOROMADDR && \
    (u_int)(kva) < \
    (ZTWOROMADDR + ZTWOROMTOP - ZTWOROMBASE))
#define isztwopa(pa) ((u_int)(pa) >= ZTWOROMBASE && (u_int)(pa) <= ZTWOROMTOP)
#define isztwomem(kva) \
    (ZTWOMEMADDR && (u_int)(kva) >= ZTWOMEMADDR && \
    (u_int)(kva) < (ZTWOMEMADDR + NZTWOMEMPG * NBPG))
#endif /* _ZTWOBUS_H_ */
