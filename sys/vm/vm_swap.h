/*	$NetBSD: vm_swap.h,v 1.1.2.4.2.3 1997/05/09 10:45:42 mrg Exp $	*/

/*
 * Copyright (c) 1995, 1996 Matthew R. Green
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
 *      This product includes software developed by Matthew R. Green for
 *      The NetBSD Foundation.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _VM_VM_SWAP_H_
#define _VM_VM_SWAP_H_

/* These structures are used to return swap information for userland */
struct swapent {
	dev_t	se_dev;
	int	se_flags;
	int	se_nblks;
	int	se_inuse;
	int	se_priority;
};

#define SWAP_ON		1
#define SWAP_OFF	2
#define SWAP_NSWAP	3
#define SWAP_STATS	4
#define	SWAP_CTL	5

#define SWF_INUSE	0x00000001
#define SWF_ENABLE	0x00000002

#ifdef _KERNEL
int sys_swap __P((struct proc *, void *, register_t *));
daddr_t swap_alloc __P((int size));
void swap_free __P((int size, daddr_t addr));
void swapinit __P((void));
#endif

#endif /* _VM_VM_SWAP_H_ */
