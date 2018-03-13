/*	$NetBSD: uvm_stats_50.h,v 1.1.2.1 2018/03/13 09:10:31 pgoyette Exp $	*/

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

#ifndef _COMPAT_COMMON_UVM_STATS_50_H
#define _COMPAT_COMMON_UVM_STATS_50_H

struct swapent50 {
	int32_t	se50_dev;		/* device id */
	int	se50_flags;		/* flags */
	int	se50_nblks;		/* total blocks */
	int	se50_inuse;		/* blocks in use */
	int	se50_priority;		/* priority of this device */
	char	se50_path[PATH_MAX+1];	/* path name */
};

struct swapdev;

void swapstats_50_init(void);
void swapstats_50_fini(void);

extern size_t swapstats_len_50;
extern void (*vec_swapstats_copy_50)(int, int, struct swapdev *,
    struct swapent50 *);

#endif /* _COMPAT_COMMON_UVM_STATS_50_H */
