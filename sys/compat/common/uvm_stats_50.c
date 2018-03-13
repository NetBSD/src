/*	$NetBSD: uvm_stats_50.c,v 1.1.2.1 2018/03/13 09:10:31 pgoyette Exp $	*/

/*
 * Copyright (c) 1995, 1996, 1997, 2009 Matthew R. Green
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
 * from: NetBSD: uvm_swap.c,v 1.175 2017/10/28 00:37:13 pgoyette Exp
 * from: NetBSD: vm_swap.c,v 1.52 1997/12/02 13:47:37 pk Exp
 * from: Id: uvm_swap.c,v 1.1.2.42 1998/02/02 20:38:06 chuck Exp
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: uvm_stats_50.c,v 1.1.2.1 2018/03/13 09:10:31 pgoyette Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/swap.h>

#include <uvm/uvm.h>

#include <compat/common/uvm_stats_50.h>

static void (*orig_swapstats_copy_50)(int, int, struct swapdev *,
    struct swapent50 *);

static void swapstats_copy_50(int, int, struct swapdev *, struct swapent50 *);
 
void
swapstats_50_init(void)
{
  
	swapstats_len_50 = sizeof(struct swapent50);
  
	orig_swapstats_copy_50 = vec_swapstats_copy_50;
	vec_swapstats_copy_50 = swapstats_copy_50;
}

void
swapstats_50_fini(void)
{

	swapstats_len_50 = 0;
	vec_swapstats_copy_50 = orig_swapstats_copy_50;
}

static void
swapstats_copy_50(int cmd, int inuse, struct swapdev *sdp,
    struct swapent50 *sep50)
{

	if (cmd == SWAP_STATS50) {
		sep50->se50_dev = sdp->swd_dev;
		sep50->se50_flags = sdp->swd_flags;
		sep50->se50_nblks = sdp->swd_nblks;
		sep50->se50_inuse = inuse;
		sep50->se50_priority = sdp->swd_priority;
		KASSERT(sdp->swd_pathlen < sizeof(sep50->se50_path));
		strcpy(sep50->se50_path, sdp->swd_path);
	}
}
