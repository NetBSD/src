/*	$NetBSD: uvm.h,v 1.2 2018/03/15 03:22:23 christos Exp $	*/

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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

#ifndef _COMPAT_SYS_UVM_H_
#define _COMPAT_SYS_UVM_H_

/*
 * NetBSD 1.3 swapctl(SWAP_STATS, ...) swapent structure; uses 32 bit
 * dev_t and has no se_path[] member.
 */
struct swapent13 {
	int32_t	se13_dev;		/* device id */
	int	se13_flags;		/* flags */
	int	se13_nblks;		/* total blocks */
	int	se13_inuse;		/* blocks in use */
	int	se13_priority;		/* priority of this device */
};

/*
 * NetBSD 5.0 swapctl(SWAP_STATS, ...) swapent structure; uses 32 bit
 * dev_t.
 */
struct swapent50 {
	int32_t	se50_dev;		/* device id */
	int	se50_flags;		/* flags */
	int	se50_nblks;		/* total blocks */
	int	se50_inuse;		/* blocks in use */
	int	se50_priority;		/* priority of this device */
	char	se50_path[PATH_MAX+1];	/* path name */
};

__BEGIN_DECLS

void uvm_13_init(void);
void uvm_50_init(void);
void uvm_13_fini(void);
void uvm_50_fini(void);

struct sys_swapctl_args;

extern int (*uvm_swap_stats13)(const struct sys_swapctl_args *, register_t *);
extern int (*uvm_swap_stats50)(const struct sys_swapctl_args *, register_t *);

__END_DECLS

#endif /* _COMPAT_SYS_UVM_H_ */
