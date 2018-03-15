/*	$NetBSD: uvm_swap.h,v 1.22.16.2 2018/03/15 09:12:07 pgoyette Exp $	*/

/*
 * Copyright (c) 1997 Matthew R. Green
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
 *
 * from: Id: uvm_swap.h,v 1.1.2.6 1997/12/15 05:39:31 mrg Exp
 */

#ifndef _UVM_UVM_SWAP_H_
#define _UVM_UVM_SWAP_H_

#define	SWSLOT_BAD	(-1)

#if defined(_KERNEL) || defined(_MODULE)
#if defined(_KERNEL_OPT)
#include "opt_vmswap.h"
#endif

struct lwp;

/*
 * swapdev: describes a single swap partition/file
 *
 * note the following should be true:
 * swd_inuse <= swd_nblks  [number of blocks in use is <= total blocks]
 * swd_nblks <= swd_mapsize [because mapsize includes miniroot+disklabel]
 */   
struct swapdev {
	dev_t			swd_dev;	/* device id */
	int			swd_flags;	/* flags:inuse/enable/fake */
	int			swd_priority;	/* our priority */ 
	int			swd_nblks;	/* blocks in this device */
	char			*swd_path;	/* saved pathname of device */
	int			swd_pathlen;	/* length of pathname */
	int			swd_npages;	/* #pages we can use */
	int			swd_npginuse;	/* #pages in use */
	int			swd_npgbad;	/* #pages bad */
	int			swd_drumoffset;	/* page0 offset in drum */
	int			swd_drumsize;	/* #pages in drum */
	blist_t			swd_blist;	/* blist for this swapdev */
	struct vnode		*swd_vp;	/* backing vnode */
	TAILQ_ENTRY(swapdev)	swd_next;	/* priority tailq */
 
	int			swd_bsize;	/* blocksize (bytes) */ 
	int			swd_maxactive;	/* max active i/o reqs */
	struct bufq_state	*swd_tab;	/* buffer list */
	int			swd_active;	/* number of active buffers */
};      

#if defined(VMSWAP)

struct swapent;

int	uvm_swap_get(struct vm_page *, int, int);
int	uvm_swap_put(int, struct vm_page **, int, int);
int	uvm_swap_alloc(int *, bool);
void	uvm_swap_free(int, int);
void	uvm_swap_markbad(int, int);
bool	uvm_swapisfull(void);
void	swapsys_lock(krw_t);
void	swapsys_unlock(void);
int	uvm_swap_stats(char *, int,
    void (*)(void *, const struct swapent *), size_t, register_t *);

#else /* defined(VMSWAP) */
#define	uvm_swapisfull()	true
#define uvm_swap_stats(c, l, f, count, retval) (*retval = 0, ENOSYS)
#endif /* defined(VMSWAP) */

void	uvm_swap_shutdown(struct lwp *);

#endif /* _KERNEL || _MODULE */

#endif /* _UVM_UVM_SWAP_H_ */
