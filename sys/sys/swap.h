/*	$NetBSD: swap.h,v 1.3 1999/01/31 09:26:28 mrg Exp $	*/

/*
 * Copyright (c) 1995, 1996, 1998 Matthew R. Green
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
 * 3. The name of the author may not be used to endorse or promote products
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

#ifndef _SYS_SWAP_H_
#define _SYS_SWAP_H_

#include <sys/syslimits.h>

#if defined(_KERNEL) && !defined(_LKM)
#include "opt_uvm.h"
#endif

/* These structures are used to return swap information for userland */

/*
 * NetBSD 1.3 swapctl(SWAP_STATS, ...) swapent structure; now used as an
 * overlay for both the new swapent structure, and the hidden swapdev
 * structure (see sys/uvm/uvm_swap.c).
 */
struct oswapent {
	dev_t	ose_dev;		/* device id */
	int	ose_flags;		/* flags */
	int	ose_nblks;		/* total blocks */
	int	ose_inuse;		/* blocks in use */
	int	ose_priority;		/* priority of this device */
};

struct swapent {
	struct oswapent se_ose;
#define	se_dev		se_ose.ose_dev
#define	se_flags	se_ose.ose_flags
#define	se_nblks	se_ose.ose_nblks
#define	se_inuse	se_ose.ose_inuse
#define	se_priority	se_ose.ose_priority
	char	se_path[PATH_MAX+1];	/* path name */
};

#define SWAP_ON		1		/* begin swapping on device */
#define SWAP_OFF	2		/* (stop swapping on device) */
#define SWAP_NSWAP	3		/* how many swap devices ? */
#define SWAP_OSTATS	4		/* old SWAP_STATS, no se_path */
#define SWAP_CTL	5		/* change priority on device */
#define SWAP_STATS	6		/* get device info */
#define SWAP_DUMPDEV	7		/* use this device as dump device */

#define SWF_INUSE	0x00000001	/* in use: we have swapped here */
#define SWF_ENABLE	0x00000002	/* enabled: we can swap here */
#define SWF_BUSY	0x00000004	/* busy: I/O happening here */
#define SWF_FAKE	0x00000008	/* fake: still being built */

#if defined(_KERNEL) && !defined(UVM)
daddr_t swap_alloc __P((int size));
void swap_free __P((int size, daddr_t addr));
void swapinit __P((void));
#endif

#endif /* _SYS_SWAP_H_ */
